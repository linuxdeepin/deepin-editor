/* -*- Mode: C++; indent-tabs-mode: nil; tab-width: 4 -*-
 * -*- coding: utf-8 -*-
 *
 * Copyright (C) 2011 ~ 2018 Deepin, Inc.
 *
 * Author:     Wang Yong <wangyong@deepin.com>
 * Maintainer: Rekols    <rekols@foxmail.com>
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include "startmanager.h"
//#include <settings.h>

#include <DApplication>
#include <QDesktopWidget>
#include <DWidgetUtil>
#include <QDebug>
#include <QScreen>
#include <QPropertyAnimation>
#include <DSettingsOption>
//#include <DSettings>

DWIDGET_USE_NAMESPACE

StartManager *StartManager::m_instance = nullptr;

StartManager *StartManager::instance()
{
    if (m_instance == nullptr) {
        m_instance = new StartManager;
    }

    return m_instance;
}

StartManager::StartManager(QObject *parent)
    : QObject(parent)
{
//    m_bIsDragEnter = false;
    // Create blank directory if it not exist.
    initBlockShutdown();
    QString blankFileDir = QDir(QStandardPaths::standardLocations(QStandardPaths::DataLocation).first()).filePath("blank-files");

    if (!QFileInfo(blankFileDir).exists()) {
        QDir().mkpath(blankFileDir);
    }
}


bool StartManager::checkPath(const QString &file)
{
    for (int i = 0;i < m_windows.count();i++) {
        if (m_windows.value(i)->wrapper(file) != nullptr) {
            m_windows.value(i)->activateWindow();
            return false;
        }
    }
    return true;
}

bool StartManager::ifKlu()
{
    auto e = QProcessEnvironment::systemEnvironment();
    QString XDG_SESSION_TYPE = e.value(QStringLiteral("XDG_SESSION_TYPE"));
    QString WAYLAND_DISPLAY = e.value(QStringLiteral("WAYLAND_DISPLAY"));

    if (XDG_SESSION_TYPE == QLatin1String("wayland") || WAYLAND_DISPLAY.contains(QLatin1String("wayland"), Qt::CaseInsensitive)){
        return true;
    }
    else {
        return false;
    }
}

bool StartManager::isMultiWindow()
{
    if (m_windows.count() > 1) {
        return true;
    }

    return false;
}

void StartManager::openFilesInWindow(QStringList files)
{
    // Open window with blank tab if no files need open.
    if (files.isEmpty()) {
        if (m_windows.count() >= 20) return;
        Window *window = createWindow();
        window->showCenterWindow(true);
        window->addBlankTab();
        window->activateWindow();
    } else {
        for (const QString &file : files) {
            FileTabInfo info = getFileTabInfo(file);

            // Open exist tab if file has opened.
            if (info.windowIndex != -1) {
                popupExistTabs(info);
            }
            // Add new tab in current window.
            else {
                Window* window = createWindow();
                window->showCenterWindow(true);
                window->addTab(file);
            }
        }
    }
}

void StartManager::openFilesInTab(QStringList files)
{
    if (files.isEmpty()) {
        QDir blankDirectory = QDir(QDir(QStandardPaths::standardLocations(QStandardPaths::DataLocation).first()).filePath("blank-files"));
        QStringList blankFiles = blankDirectory.entryList(QStringList(), QDir::Files);

        if (m_windows.isEmpty()) {

            Window *window = createWindow(true);
            window->showCenterWindow(true);
            bool bIsEmpty = false;
            m_qlistTemFile = Settings::instance()->settings->option("advance.editor.browsing_history_temfile")->value().toStringList();

            for (auto temFile : m_qlistTemFile) {
                if (temFile.isEmpty()) {
                    bIsEmpty = true;
                }
            }
           // bIsEmpty = true;
            if (!bIsEmpty) {
                QFileInfo fileInfo;
                QFileInfo fileInfoTem;
                bool bIsTemFile = false;
                for (int i = 0;i < m_qlistTemFile.count();i++) {
                    QJsonParseError jsonError;
                    // 转化为 JSON 文档
                    QJsonDocument doucment = QJsonDocument::fromJson(m_qlistTemFile.value(i).toUtf8(), &jsonError);
                    // 解析未发生错误
                    if (!doucment.isNull() && (jsonError.error == QJsonParseError::NoError)) {
                        if (doucment.isObject()) {
                            QString temFilePath;
                            QString localPath;
                            // JSON 文档为对象
                            QJsonObject object = doucment.object();  // 转化为对象

                            if (object.contains("temFilePath")) {  // 包含指定的 key
                                QJsonValue value = object.value("temFilePath");  // 获取指定 key 对应的 value

                                if (value.isString()) {  // 判断 value 是否为字符串
                                    temFilePath = value.toString();  // 将 value 转化为字符串
                                }
                            }

                            if (object.contains("modify")) {  // 包含指定的 key
                                QJsonValue value = object.value("modify");  // 获取指定 key 对应的 value

                                if (value.isBool()) {  // 判断 value 是否为字符串
                                    bIsTemFile = value.toBool();
                                }
                            }

                            if (object.contains("localPath")) {  // 包含指定的 key
                                QJsonValue value = object.value("localPath");  // 获取指定 key 对应的 value

                                if (value.isString()) {  // 判断 value 是否为字符串
                                    localPath = value.toString();  // 将 value 转化为字符串
                                    fileInfo.setFile(localPath);
                                }
                            }

                            if (!temFilePath.isEmpty()) {
                                if (temFilePath == localPath) {
                                    int index = blankFiles.indexOf(fileInfo.fileName());

                                    if (index >= 0) {
                                        QString fileName = tr("Untitled %1").arg(index + 1);
                                        window->addTemFileTab(temFilePath,fileName,localPath,bIsTemFile);
                                    }
                                } else {
                                        window->addTemFileTab(temFilePath,fileInfo.fileName(),localPath,bIsTemFile);

                                }
                            } else {
                                if (!localPath.isEmpty()) {
                                    window->addTemFileTab(localPath,fileInfo.fileName(),localPath,bIsTemFile);
                                }
                            }
                        }
                    }
                }
            } else {
                // Open blank files of last session.
                if (!blankFiles.isEmpty()) {
                    QTimer::singleShot(50, [=] {
                        for (const QString &blankFile : blankFiles) {
                            window->addBlankTab(QDir(blankDirectory).filePath(blankFile));
                        }
                    });
                }
                // Just open new window with blank tab if no blank files in last session.
                else {
                    window->addBlankTab();
                }
            }
        }
        // Just active first window if no file is need opened.
        else {
            Window *window = createWindow();
            window->show();
            window->addBlankTab();
        }
    } else {
        for (const QString &file : files) {
            FileTabInfo info = getFileTabInfo(file);

            // Open exist tab if file has opened.
            if (info.windowIndex != -1) {
                popupExistTabs(info);

                //qDebug() << "Open " << file << " in exist tab";
            }
            // Create new window with file if haven't window exist.
            else if (m_windows.size() == 0) {
                Window *window = createWindow(true);
                window->showCenterWindow(true);
                QTimer::singleShot(50, [=] { window->addTab(file); });

                //qDebug() << "Open " << file << " with new window";
            }
            // Open file tab in first window of window list.
            else {
                Window *window = m_windows[0];
                window->addTab(file);
                window->setWindowState(Qt::WindowActive);
                window->activateWindow();

//                int indexid=0;
//                uint winid=0;
//                QDBusInterface dock("com.deepin.dde.daemon.Dock",
//                                    "/com/deepin/dde/daemon/Dock",
//                                    "com.deepin.dde.daemon.Dock",
//                                    QDBusConnection::sessionBus()
//                                    );
//                QDBusReply<QStringList> rep = dock.call("GetEntryIDs");
//                for(auto name:rep.value())
//                {
//                    if(name=="deepin-editor")
//                    {
//                        indexid=rep.value().indexOf(name);
//                    }
//                }

//                m_pDock.reset(new Dock("com.deepin.dde.daemon.Dock",
//                                       "/com/deepin/dde/daemon/Dock",
//                                       QDBusConnection::sessionBus(),
//                                       this
//                                       )
//                              );
//                QList<QDBusObjectPath> list = m_pDock->entries();

//                m_pEntry.reset(new Entry("com.deepin.dde.daemon.Dock",
//                                         list[indexid].path(),
//                                         QDBusConnection::sessionBus(),
//                                         this));
//                winid= m_pEntry->currentWindow() ;

//                QDBusMessage active = QDBusMessage::createMethodCall("com.deepin.dde.daemon.Dock",
//                                                                     "/com/deepin/dde/daemon/Dock",
//                                                                     "com.deepin.dde.daemon.Dock",
//                                                                     "ActivateWindow");
//                active<<winid;
//                QDBusConnection::sessionBus().call(active, QDBus::BlockWithGui);
            }

        }
    }
}


void StartManager::createWindowFromWrapper(const QString &tabName, const QString &filePath, const QString &qstrTruePath, EditWrapper *buffer, bool isModifyed)
{

    Window *window = createWindow();
    //window->showCenterWindow();
    window->addTabWithWrapper(buffer, filePath, qstrTruePath, tabName);
    window->currentWrapper()->updateModifyStatus(isModifyed);
    window->setMinimumSize(Tabbar::sm_pDragPixmap->rect().size());


    QRect rect = window->rect();
    QPoint pos = QCursor::pos() ;/*- window->topLevelWidget()->pos();*/
    QRect desktopRect = QApplication::desktop()->rect();
    QPoint startPos = pos;

    if ((pos.x() + rect.width()) > desktopRect.width()) {
        startPos.setX(desktopRect.width() - rect.width());
    } else if (pos.x() < 0) {
        startPos.setX(0);
    }

    if ((pos.y() + rect.height()) > desktopRect.height()) {
        startPos.setY(desktopRect.height() - rect.height());
    } else if (pos.y() < 0) {
        startPos.setY(0);
    }

    QRect startRect(startPos, Tabbar::sm_pDragPixmap->rect().size());
    QRect endRect(startPos,window->rect().size());
    window->move(startPos);
    window->show();
#if 0
    // window->setFixedSize(Tabbar::sm_pDragPixmap->rect().size());
    QLabel *pLab = new QLabel();
    //pLab->resize(Tabbar::sm_pDragPixmap->rect().size());
    pLab->move(pos);
    pLab->setPixmap(*Tabbar::sm_pDragPixmap);
    pLab->show();
#endif
    //添加编辑窗口drop动态显示效果　梁卫东　２０２０－０８－２５　０９：５４：５７
    QPropertyAnimation *geometry = new QPropertyAnimation(window, "geometry");
    geometry->setDuration(200);
    geometry->setStartValue(startRect);
    geometry->setEndValue(endRect);
    geometry->setEasingCurve(QEasingCurve::InCubic);
    //OutCubic InCubic
//    QPropertyAnimation *Opacity = new QPropertyAnimation(this, "windowOpacity");
//    connect(Opacity,&QPropertyAnimation::finished,Opacity,&QPropertyAnimation::deleteLater);
//    Opacity->setDuration(200);
//    Opacity->setStartValue(1.0);
//    Opacity->setEndValue(0);
//    Opacity->setEasingCurve(QEasingCurve::InCirc);

    QParallelAnimationGroup *group = new QParallelAnimationGroup;
    connect(group,&QParallelAnimationGroup::finished,geometry,[window,geometry,/*Opacity,*/group](){
        // window minimum size.
        //window->setMinimumSize(1000, 600);
        window->showCenterWindow(false);
        geometry->deleteLater();
       // Opacity->deleteLater();
        group->deleteLater();
    });

    group->addAnimation(geometry);
   // group->addAnimation(Opacity);

    group->start();
}

void StartManager::loadTheme(const QString &themeName)
{
    for (Window *window : m_windows) {
        window->loadTheme(themeName);
    }
}

Window* StartManager::createWindow(bool alwaysCenter)
{
    // Create window.
    Window *window = new Window;
    connect(window, &Window::themeChanged, this, &StartManager::loadTheme, Qt::QueuedConnection);
    connect(window, &Window::sigJudgeBlockShutdown, this, &StartManager::slotCheckUnsaveTab, Qt::QueuedConnection);

    // Quit application if close last window.
    connect(window, &Window::close, this, [=] {
        int windowIndex = m_windows.indexOf(window);
        //qDebug() << "Close window " << windowIndex;

        if (windowIndex >= 0) {
            m_windows.takeAt(windowIndex);
        }

        if (m_windows.isEmpty()) {
            QDir path = QDir::currentPath();
            if (!path.exists()) {
                return ;
            }
            path.setFilter(QDir::Files);
            QStringList nameList = path.entryList();
            foreach (auto name, nameList) {
                if (name.contains("tabPaths.txt")) {
                    QFile file(name);
                    file.remove();
                }
            }
            QApplication::quit();
            PerformanceMonitor::closeAPPFinish();
        }
    });

    // Init window position.
    initWindowPosition(window, alwaysCenter);

    connect(window, &Window::newWindow, this, [=] {
        openFilesInWindow(QStringList());
    });

    // Append window in window list.
    m_windows << window;

    return window;
}

void StartManager::initWindowPosition(Window *window, bool alwaysCenter)
{
    if (m_windows.isEmpty() || alwaysCenter) {
        //Dtk::Widget::moveToCenter(window);
    } else {
        // Add window offset to avoid all editor window popup at same coordinate.
        int windowOffset = m_windows.size() * 50;
        QRect screenGeometry = qApp->desktop()->screenGeometry(QCursor::pos());
        window->move(screenGeometry.x() + windowOffset, screenGeometry.y() + windowOffset);
    }
}

void StartManager::popupExistTabs(FileTabInfo info)
{
    Window *window = m_windows[info.windowIndex];
    window->showNormal();
    window->activeTab(info.tabIndex);
    window->setWindowState(Qt::WindowActive);
    window->activateWindow();

//    int indexid=0;
//    uint winid=0;
//    QDBusInterface dock("com.deepin.dde.daemon.Dock",
//                        "/com/deepin/dde/daemon/Dock",
//                        "com.deepin.dde.daemon.Dock",
//                        QDBusConnection::sessionBus()
//                        );
//    QDBusReply<QStringList> rep = dock.call("GetEntryIDs");

//    for(auto name:rep.value())
//    {
//        if(name=="deepin-editor")
//        {
//            indexid=rep.value().indexOf(name);
//        }
//    }

//    m_pDock.reset(new Dock("com.deepin.dde.daemon.Dock",
//                           "/com/deepin/dde/daemon/Dock",
//                           QDBusConnection::sessionBus(),
//                           this
//                           )
//                  );
//    QList<QDBusObjectPath> list = m_pDock->entries();

//    m_pEntry.reset(new Entry("com.deepin.dde.daemon.Dock",
//                             list[indexid].path(),
//                             QDBusConnection::sessionBus(),
//                             this));
//    winid= m_pEntry->currentWindow() ;


//    QDBusMessage active = QDBusMessage::createMethodCall("com.deepin.dde.daemon.Dock",
//                                                         "/com/deepin/dde/daemon/Dock",
//                                                         "com.deepin.dde.daemon.Dock",
//                                                         "ActivateWindow");
//    active<<winid;
//    QDBusConnection::sessionBus().call(active, QDBus::BlockWithGui);
}

StartManager::FileTabInfo StartManager::getFileTabInfo(QString file)
{
    FileTabInfo info = {-1, -1};

    //qDebug() << "Windows size: " << m_windows.size();

    for (Window *window : m_windows) {
        int tabIndex = window->getTabIndex(file);
        if (tabIndex >= 0) {
            info.windowIndex = m_windows.indexOf(window);
            info.tabIndex = tabIndex;
            break;
        }
    }

    return info;
}


void StartManager::initBlockShutdown()
{
    if (m_reply.value().isValid()) {
        //qDebug() << "m_reply.value().isValid():" << m_reply.value().isValid();
        return;
    }

    m_pLoginManager = new QDBusInterface("org.freedesktop.login1",
                                         "/org/freedesktop/login1",
                                         "org.freedesktop.login1.Manager",
                                         QDBusConnection::systemBus());

    m_arg << QString("shutdown")             // what
          << qApp->applicationDisplayName()           // who
          << QObject::tr("File not saved") // why
          << QString("delay");                        // mode

    int fd = -1;
    m_reply = m_pLoginManager->callWithArgumentList(QDBus::Block, "Inhibit", m_arg);
    if (m_reply.isValid()) {
        fd = m_reply.value().fileDescriptor();
    }
}

void StartManager::slotCheckUnsaveTab() {
    for (Window *pWindow : m_windows) {
        //如果返回true，则表示有未保存的tab项，则阻塞系统关机
        bool bRet = pWindow->checkBlockShutdown();
        if (bRet == true) {
            m_reply = m_pLoginManager->callWithArgumentList(QDBus::Block, "Inhibit", m_arg);
            if (m_reply.isValid()) {
                //qDebug() << "Block shutdown.";
            }

            return;
        }
    }

    //如果for结束则表示没有发现未保存的tab项，则放开阻塞关机
    if (m_reply.isValid()) {
        QDBusReply<QDBusUnixFileDescriptor> tmp = m_reply;
        m_reply = QDBusReply<QDBusUnixFileDescriptor>();
        //m_pLoginManager->callWithArgumentList(QDBus::NoBlock, "Inhibit", m_arg);
        //qDebug() << "Nublock shutdown.";
    }
}
