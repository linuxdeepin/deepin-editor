// SPDX-FileCopyrightText: 2011-2026 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include "environments.h"
#include "utils.h"
#include "window.h"
#include "urlinfo.h"
#include "editorapplication.h"
#include "performancemonitor.h"
#include "eventlogutils.h"
#include "common/utils.h"

#include <DApplication>
#include <DMainWindow>
#include <DWidgetUtil>
#include <DLog>
#include <DSettingsOption>

#include <QApplication>
#include <QCommandLineParser>
#include <QDBusConnection>
#include <QDBusInterface>
#include <QScreen>
#include <QDebug>

#include <iostream>

DWIDGET_USE_NAMESPACE

int main(int argc, char *argv[])
{
    qDebug() << "Application starting with arguments:" << QCoreApplication::arguments();
    DCORE_USE_NAMESPACE
    PerformanceMonitor::initializeAppStart();

    // —— QtWebEngine 架构适配（参考 deepin-manual dman.cpp，须在 QApplication 构造前设置）——
    // 注意：qputenv 是整体覆盖而非追加，后续设置会覆盖先前值，因此各架构只在此处
    // 一次性设置完整 flags，不要在别处重复设置。
    const bool isWaylandSession = []() {
        const auto env = QProcessEnvironment::systemEnvironment();
        return env.value(QStringLiteral("XDG_SESSION_TYPE")) == QLatin1String("wayland")
               || env.value(QStringLiteral("WAYLAND_DISPLAY")).contains(QLatin1String("wayland"), Qt::CaseInsensitive);
    }();
#ifdef __sw_64__
    // sw_64（申威）：V8 JIT 不支持，必须 jitless 纯解释执行，否则渲染进程崩溃；
    // 合并 --disable-gpu 一次性设置，避免被分段设置覆盖（参考 deepin-manual bug-347387）
    qputenv("QTWEBENGINE_CHROMIUM_FLAGS", "--disable-gpu --single-process --js-flags=--jitless");
    qDebug() << "Set QTWEBENGINE_CHROMIUM_FLAGS for sw_64: --disable-gpu --single-process --js-flags=--jitless";
#else
    // 通用（含 x86/arm64/loongarch64/mips）：禁用 GPU 合成走软件渲染，
    // 规避国产平台显卡驱动兼容问题（参考 deepin-manual 全架构处理）
    qputenv("QTWEBENGINE_CHROMIUM_FLAGS", "--disable-gpu");
    qDebug() << "Set QTWEBENGINE_CHROMIUM_FLAGS=--disable-gpu";
#  ifndef __mips__
    // 非 Wayland 会话追加 --single-process（渲染进程并入主进程，加快首屏并规避
    // 欧拉 root 下 no-sandbox 报错）；mips 上实测有问题须禁用（deepin-manual de2df4a4）
    if (!isWaylandSession) {
        qputenv("QTWEBENGINE_CHROMIUM_FLAGS", "--disable-gpu --single-process");
        qDebug() << "Non-Wayland session, set --single-process additionally";
    }
#  endif
#endif
    // 龙芯（mips 内核）平台 DTK 若强制 raster widgets 会导致 WebEngine GL 窗口异常，
    // 显式关闭（deepin-manual 同款处理，无副作用）
    qputenv("DTK_FORCE_RASTER_WIDGETS", "FALSE");

    if (!QString(qgetenv("XDG_CURRENT_DESKTOP")).toLower().startsWith("deepin")) {
        qDebug() << "XDG_CURRENT_DESKTOP not set to Deepin, setting environment variable";
        setenv("XDG_CURRENT_DESKTOP", "Deepin", 1);
    } else {
        qDebug() << "XDG_CURRENT_DESKTOP already set to:" << qgetenv("XDG_CURRENT_DESKTOP");
    }
    qDebug() << "XDG_CURRENT_DESKTOP set to:" << qgetenv("XDG_CURRENT_DESKTOP");
    QCoreApplication::setAttribute(Qt::AA_UseOpenGLES);
    // QWebEngineView 硬性前提：共享 GL 上下文必须在 QApplication 构造前设置，
    // 否则 Markdown 渲染视图无法创建共享上下文导致白屏（Qt6 文档要求）
    QCoreApplication::setAttribute(Qt::AA_ShareOpenGLContexts);
    qDebug() << "Qt::AA_UseOpenGLES / AA_ShareOpenGLContexts attributes set";

    EditorApplication app(argc, argv);
    qInfo() << "Application instance created, version:" << app.applicationVersion();
    // save theme
    // DApplicationSettings savetheme;

    // 需在App构造后初始化日志设置
    DLogManager::registerConsoleAppender();
    DLogManager::registerFileAppender();
    qDebug() << "Log system initialized";
    // Parser input arguments.
    QCommandLineParser parser;
    const QCommandLineOption newWindowOption("w", "Open file in new window");
    const QCommandLineOption helpOption = parser.addHelpOption();
    parser.addOption(newWindowOption);
    parser.process(app);
    qDebug() << "Command line arguments processed, positional args:" << parser.positionalArguments().size();

    qInfo() << qPrintable(QString("App start, pid: %1, version: %2").arg(app.applicationPid()).arg(app.applicationVersion()));

    QStringList urls;
    QStringList arguments = parser.positionalArguments();
    qDebug() << "Processing" << arguments.size() << "file arguments";

    for (const QString &path : arguments) {
        UrlInfo info(path);
        urls << info.url.toLocalFile();

        QFileInfo fileInfo(path);
        qInfo() << qPrintable(QString("Open file, isFile: %1, suffix: %2, size: %3, permssion: %4").arg(fileInfo.isFile())
                              .arg(fileInfo.suffix()).arg(fileInfo.size()).arg(static_cast<int>(fileInfo.permissions())));
    }

    bool hasWindowFlag = parser.isSet(newWindowOption);
    qDebug() << "New window flag set:" << hasWindowFlag;

    QDBusConnection dbus = QDBusConnection::sessionBus();
    // Start editor process if not found any editor use DBus.
    qDebug() << "Attempting to register DBus service";
    if (dbus.registerService("com.deepin.Editor")) {
        qInfo() << "DBus service registered successfully";
#ifdef DTKWIDGET_CLASS_DSizeMode
        qDebug() << "DTKWIDGET_CLASS_DSizeMode defined, setting up size mode handler";
        // 不同模式下的基础字体像素大小不同，系统级别为 T6 的字体大小, 默认是 14px ；在紧凑模式下 T6 为 12px
        QObject::connect(DGuiApplicationHelper::instance(), &DGuiApplicationHelper::sizeModeChanged, []() {
            const int genericPixelSize = 14;
            const int genericPixelSizeCompact = 12;
            bool isCompact = DGuiApplicationHelper::isCompactMode();
            qDebug() << "Size mode changed, compact mode:" << isCompact;
            DFontSizeManager::instance()->setFontGenericPixelSize(
                DGuiApplicationHelper::isCompactMode() ? genericPixelSizeCompact : genericPixelSize);
        });
#else
        qDebug() << "DTKWIDGET_CLASS_DSizeMode not defined, skipping size mode handler";
#endif

        StartManager *startManager = StartManager::instance();

        //埋点记录启动数据
        QJsonObject objStartEvent{
            {"tid", Eventlogutils::StartUp},
            {"version", VERSION},
            {"mode", 1},
        };
        Eventlogutils::GetInstance()->writeLogs(objStartEvent);
        qDebug() << "Startup event logged with version:" << VERSION;

        bool save_tab_before_close =
            Settings::instance()->settings->option("advance.startup.save_tab_before_close")->value().toBool();
        if (!save_tab_before_close && urls.isEmpty()) {
            qDebug() << "No files to open and save_tab_before_close is false, creating window with blank tab";
            auto window = startManager->createWindow(true);
            window->addBlankTab();
        } else {
            if (hasWindowFlag) {
                qDebug() << "Opening" << urls.size() << "files in new window";
                startManager->openFilesInWindow(urls);
            } else {
                qDebug() << "Opening" << urls.size() << "files in current window";
                startManager->openFilesInTab(urls);
            }
        }
#if _ZPD_
        qDebug() << "ZPD customization enabled, loading custom DLL";
        // 解析ZPD定制需求提供的库libzpdcallback.so
        Utils::loadCustomDLL();
#endif
        dbus.registerObject("/com/deepin/Editor", startManager, QDBusConnection::ExportScriptableSlots);
        qDebug() << "DBus object registered at /com/deepin/Editor";

        PerformanceMonitor::initializAppFinish();
        qDebug() << "Entering main event loop";
        return app.exec();
    }
    // Just send dbus message to exist editor process.
    else {
        qInfo() << "Another editor instance detected, sending commands via DBus";
        QDBusInterface notification(
            "com.deepin.Editor", "/com/deepin/Editor", "com.deepin.Editor", QDBusConnection::sessionBus());

        QList<QVariant> args;
        args << urls;

        if (hasWindowFlag) {
            qDebug() << "Calling openFilesInWindow via DBus with" << urls.size() << "files";
            notification.callWithArgumentList(QDBus::AutoDetect, "openFilesInWindow", args);
        } else {
            qDebug() << "Calling openFilesInTab via DBus with" << urls.size() << "files";
            notification.callWithArgumentList(QDBus::AutoDetect, "openFilesInTab", args);
        }
    }

    qDebug() << "Application exiting with code 0";
    return 0;
}
