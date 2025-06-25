// SPDX-FileCopyrightText: 2011-2024 UnionTech Software Technology Co., Ltd.
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
    if (!QString(qgetenv("XDG_CURRENT_DESKTOP")).toLower().startsWith("deepin")) {
        qDebug() << "XDG_CURRENT_DESKTOP not set to Deepin, setting environment variable";
        setenv("XDG_CURRENT_DESKTOP", "Deepin", 1);
    } else {
        qDebug() << "XDG_CURRENT_DESKTOP already set to:" << qgetenv("XDG_CURRENT_DESKTOP");
    }
    qDebug() << "XDG_CURRENT_DESKTOP set to:" << qgetenv("XDG_CURRENT_DESKTOP");
    QCoreApplication::setAttribute(Qt::AA_UseOpenGLES);
    qDebug() << "Qt::AA_UseOpenGLES attribute set";

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
                              .arg(fileInfo.suffix()).arg(fileInfo.size()).arg(fileInfo.permissions()));
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
