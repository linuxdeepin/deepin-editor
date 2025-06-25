// SPDX-FileCopyrightText: 2011-2022 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include "startmanager.h"
//#include <settings.h>

#include <DApplication>
#include <DWidgetUtil>
#include <QDebug>
#include <QScreen>
#include <QPropertyAnimation>
#include <DSettingsOption>
#include <DAboutDialog>
#if (QT_VERSION < QT_VERSION_CHECK(6, 0, 0))
#include <QDesktopWidget>
#endif

#include "common/iflytek_ai_assistant.h"

DWIDGET_USE_NAMESPACE

// 备份定时器间隔
enum BackupInterval {
    EAutoBackupInterval = 5 * 60 * 1000,        ///< 周期自动备份定时 5分钟
    EDelayBackupInterval = 20,                  ///< 延迟备份间隔 20ms
};

// 用于配置文件书签的标识
static const QString s_bookMarkKey = "advance.editor.bookmark";

StartManager *StartManager::m_instance = nullptr;

StartManager *StartManager::instance()
{
    if (m_instance == nullptr) {
        qDebug() << "StartManager instance is null, creating new instance";
        m_instance = new StartManager;
    }

    return m_instance;
}

StartManager::StartManager(QObject *parent)
    : QObject(parent)
{
    qDebug() << "Enter StartManager constructor";
    //m_bIsDragEnter = false;
    //Create blank directory if it not exist.

    initBlockShutdown();
    qDebug() << "inited block shutdown";

    m_blankFileDir = QDir(Utils::cleanPath(QStandardPaths::standardLocations(QStandardPaths::AppDataLocation)).first()).filePath("blank-files");
    m_backupDir = QDir(Utils::cleanPath(QStandardPaths::standardLocations(QStandardPaths::AppDataLocation)).first()).filePath("backup-files");
    m_autoBackupDir = QDir(Utils::cleanPath(QStandardPaths::standardLocations(QStandardPaths::AppDataLocation)).first()).filePath("autoBackup-files");

    if (!QFileInfo(m_blankFileDir).exists()) {
        qDebug() << "blank file dir not exist, create it";
        QDir().mkpath(m_blankFileDir);
    }

    if (!QFileInfo(m_backupDir).exists()) {
        qDebug() << "backup file dir not exist, create it";
        QDir().mkpath(m_backupDir);
    }

    m_qlistTemFile = Settings::instance()->settings->option("advance.editor.browsing_history_temfile")->value().toStringList();
    // 初始化书签信息记录表
    initBookmark();
    qDebug() << "inited bookmark";

    //按时间自动备份（5分钟）
    m_pTimer = new QTimer;
    connect(m_pTimer, &QTimer::timeout, this, &StartManager::autoBackupFile);
    m_pTimer->start(EAutoBackupInterval);

    // init flytek backend, check service valid
    IflytekAiAssistant::instance()->checkAiExists();
    qInfo() << "StartManager initialized with backup interval:" << EAutoBackupInterval << "ms";
    qDebug() << "Exit StartManager constructor";
}

bool StartManager::checkPath(const QString &file)
{
    qDebug() << "Enter checkPath, file:" << file;
    for (int i = 0; i < m_windows.count(); i++) {
        EditWrapper *wrapper = m_windows.value(i)->wrapper(file);

        if (wrapper != nullptr) {
            FileTabInfo info = getFileTabInfo(wrapper->textEditor()->getFilePath());
            // Open exist tab if file has opened.
            popupExistTabs(info);

            qDebug() << "Exit checkPath, return false";
            return false;
        }
    }

    qDebug() << "Exit checkPath, return true";

    return true;
}

bool StartManager::ifKlu()
{
    qDebug() << "Enter ifKlu";
    auto e = QProcessEnvironment::systemEnvironment();
    QString XDG_SESSION_TYPE = e.value(QStringLiteral("XDG_SESSION_TYPE"));
    QString WAYLAND_DISPLAY = e.value(QStringLiteral("WAYLAND_DISPLAY"));

    if (XDG_SESSION_TYPE == QLatin1String("wayland") || WAYLAND_DISPLAY.contains(QLatin1String("wayland"), Qt::CaseInsensitive)) {
        qDebug() << "Exit ifKlu, return true";
        return true;
    } else {
        qDebug() << "Exit ifKlu, return false";
        return false;
    }
}

bool StartManager::isMultiWindow()
{
    qDebug() << "Enter isMultiWindow";
    if (m_windows.count() > 1) {
        qDebug() << "Exit isMultiWindow, return true";
        return true;
    }

    qDebug() << "Exit isMultiWindow, return false";
    return false;
}

bool StartManager::isTemFilesEmpty()
{
    qDebug() << "Enter isTemFilesEmpty";
    bool bIsEmpty = false;

    for (auto temFile : m_qlistTemFile) {
        if (temFile.isEmpty()) {
            bIsEmpty = true;
        }
    }

    qDebug() << "Exit isTemFilesEmpty, return" << bIsEmpty;
    return bIsEmpty;
}

void StartManager::autoBackupFile()
{
    qDebug() << "Enter autoBackupFile";
    // 标签页在拖拽状态时不执行备份
    if (m_bIsTagDragging) {
        qDebug() << "Exit autoBackupFile, return";
        return;
    }

    //如果自动备份文件夹不存在，创建自动备份文件夹
    if (!QFileInfo(m_autoBackupDir).exists()) {
        qDebug() << "Auto backup dir not exist, create it";
        QDir().mkpath(m_autoBackupDir);
    } else {
        qDebug() << "Auto backup dir exist, remove user backup";
        //有用户备份时删除用户备份
        if (!QDir(m_backupDir).isEmpty()) {
            qDebug() << "User backup exist, remove it";
            QDir(m_backupDir).removeRecursively();
        }
    }

    QMap<QString, EditWrapper *> wrappers;
    QStringList listBackupInfo;
    QString filePath, localPath, curPos;
    QFileInfo fileInfo;
    m_qlistTemFile.clear();
    listBackupInfo = Settings::instance()->settings->option("advance.editor.browsing_history_temfile")->value().toStringList();

    //记录所有的文件信息
    for (int var = 0; var < m_windows.count(); ++var) {
        wrappers = m_windows.value(var)->getWrappers();
        QStringList list = wrappers.keys();

        for (EditWrapper *wrapper : wrappers) {
            //大文件加载时不备份
            if (wrapper->getFileLoading()) {
                qDebug() << "File loading, skip auto backup, continue";
                continue;
            }

            filePath = wrapper->textEditor()->getFilePath();
            localPath = wrapper->textEditor()->getTruePath();

            StartManager::FileTabInfo tabInfo = StartManager::instance()->getFileTabInfo(filePath);
            curPos = QString::number(wrapper->textEditor()->textCursor().position());
            fileInfo.setFile(localPath);

            //json格式记录文件信息
            QJsonObject jsonObject;
            QJsonDocument document;
            jsonObject.insert("localPath", localPath); //记录源文件路径
            jsonObject.insert("cursorPosition", curPos); //记录鼠标位置
            jsonObject.insert("modify", wrapper->isModified()); //记录修改状态
            jsonObject.insert("window", var); //记录窗口Index

            //记录书签
            QList<int> bookmarkList = wrapper->textEditor()->getBookmarkInfo();

            if (!bookmarkList.isEmpty()) {
                qDebug() << "File has bookmark, record it";
                QString bookmarkInfo;

                for (int i = 0; i < bookmarkList.count(); i++) {
                    if (i == bookmarkList.count() - 1) {
                        bookmarkInfo.append(QString::number(bookmarkList.value(i)));
                    } else {
                        bookmarkInfo.append(QString::number(bookmarkList.value(i)) + ",");
                    }
                }

                jsonObject.insert("bookMark", bookmarkInfo);

                // 更新记录全局书签信息
                m_bookmarkTable.insert(localPath, bookmarkList);
            } else {
                qDebug() << "File has no bookmark, remove it";
                // 无书签，移除
                m_bookmarkTable.remove(localPath);
            }

            //记录活动页
            if (m_windows.value(var)->isActiveWindow()) {
                if (wrapper == m_windows.value(var)->currentWrapper()) {
                    jsonObject.insert("focus", true);
                }
            }

            //保存备份文件
            if (Utils::isDraftFile(filePath)) {
                qDebug() << "File is draft, save it as temporary file";
                wrapper->saveTemFile(filePath);
            } else {
                qDebug() << "File is not draft, save it as backup file";
                if (wrapper->isModified()) {
                    qDebug() << "File is modified, save it as backup file";
                    QString name = fileInfo.absolutePath().replace("/", "_");
                    QString qstrFilePath = m_autoBackupDir + "/" + Utils::getStringMD5Hash(fileInfo.baseName()) + "." + name + "." + fileInfo.suffix();
                    jsonObject.insert("temFilePath", qstrFilePath);
                    wrapper->saveTemFile(qstrFilePath);
                }
            }

            //使用json串形式保存
            document.setObject(jsonObject);
            QByteArray byteArray = document.toJson(QJsonDocument::Compact);
            list.replace(tabInfo.tabIndex, byteArray);
            qInfo() << "Auto backup completed, files backed up:" << m_qlistTemFile.size();
            qDebug() << "Exit autoBackupFile";
        }

        m_qlistTemFile.append(list);
    }

    //将json串列表写入配置文件
    Settings::instance()->settings->option("advance.editor.browsing_history_temfile")->setValue(m_qlistTemFile);
    // 备份书签信息
    saveBookmark();
    qDebug() << "Exit autoBackupFile";
}

int StartManager::recoverFile(Window *window)
{
    qDebug() << "Enter recoverFile";
    Window *pFocusWindow = nullptr;
    QString focusPath;
    bool bIsTemFile = false;
    QStringList blankFiles = QDir(m_blankFileDir).entryList(QStringList(), QDir::Files);
    qDebug() << "Found" << blankFiles.size() << "blank files in directory:" << m_blankFileDir;
    int recFilesSum = 0;
    QStringList files = blankFiles;
    QFileInfo fileInfo;
    QString lastmodifiedtime;
    //去除非新建文件
    for (auto file : blankFiles) {
        if (!file.contains("blank_file")) {
            files.removeOne(file);
            qDebug() << "Removed non-blank file from list:" << file;
        }
    }
    qDebug() << "After filtering, found" << files.size() << "blank files";

    int windowIndex = -1;

    //根据备份信息恢复文件
    qDebug() << "Processing" << m_qlistTemFile.count() << "temporary files for recovery";
    for (int i = 0; i < m_qlistTemFile.count(); i++) {
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
                qDebug() << "Processing JSON object for recovery, item" << (i+1) << "of" << m_qlistTemFile.count();

                //得到恢复文件对应的窗口
                if (object.contains("window")) {  // 包含指定的 key
                    QJsonValue value = object.value("window");  // 获取指定 key 对应的 value

                    if (value.isDouble()) {
                        if (windowIndex == -1) {
                            windowIndex = static_cast<int>(value.toDouble());
                            qDebug() << "First window index found:" << windowIndex;
                        } else {
                            if (windowIndex != static_cast<int>(value.toDouble())) {
                                windowIndex = static_cast<int>(value.toDouble());
                                qDebug() << "New window index found:" << windowIndex << ", creating new window";
                                window = createWindow(true);
                                window->showCenterWindow(false);
                            }
                        }
                    }
                }

                //得到备份文件路径
                if (object.contains("temFilePath")) {  // 包含指定的 key
                    QJsonValue value = object.value("temFilePath");  // 获取指定 key 对应的 value

                    if (value.isString()) {  // 判断 value 是否为字符串
                        temFilePath = value.toString();  // 将 value 转化为字符串
                        qDebug() << "Found temporary file path:" << temFilePath;
                    }
                }

                //得到文件修改状态
                if (object.contains("modify")) {  // 包含指定的 key
                    QJsonValue value = object.value("modify");  // 获取指定 key 对应的 value

                    if (value.isBool()) {  // 判断 value 是否为字符串
                        bIsTemFile = value.toBool();
                        qDebug() << "File modification status:" << (bIsTemFile ? "modified" : "unmodified");
                    }
                }
                if (object.contains("lastModifiedTime")) {
                    auto v = object.value("lastModifiedTime");
                    if (v.isString()) {
                        lastmodifiedtime = v.toString();
                        qDebug() << "Last modified time:" << lastmodifiedtime;
                    }
                }

                //得到真实文件路径
                if (object.contains("localPath")) {  // 包含指定的 key
                    QJsonValue value = object.value("localPath");  // 获取指定 key 对应的 value

                    if (value.isString()) {  // 判断 value 是否为字符串
                        localPath = value.toString();  // 将 value 转化为字符串
                        fileInfo.setFile(localPath);
                        qDebug() << "Local file path:" << localPath << "exists:" << fileInfo.exists();
                        if (!fileInfo.exists()) {
                            qWarning() << "Local file does not exist, skipping recovery for:" << localPath;
                            continue;
                        }
                    }
                }

                //打开文件
                if (!temFilePath.isEmpty()) {
                    if (Utils::fileExists(temFilePath)) {
                        qDebug() << "Opening temporary file:" << temFilePath << "for" << fileInfo.fileName();
                        window->addTemFileTab(temFilePath, fileInfo.fileName(), localPath, lastmodifiedtime, bIsTemFile);

                        //打开文件后设置书签
                        if (object.contains("bookMark")) {  // 包含指定的 key
                            QJsonValue value = object.value("bookMark");  // 获取指定 key 对应的 value

                            if (value.isString()) {
                                QList<int> bookmarkList;
                                bookmarkList = analyzeBookmakeInfo(value.toString());
                                qDebug() << "Setting bookmarks from file config:" << bookmarkList;
                                window->wrapper(temFilePath)->textEditor()->setBookMarkList(bookmarkList);
                            }
                        } else if (m_bookmarkTable.contains(temFilePath)) {
                            // 若文件已有配置，则以文件为准，否则以全局配置为准
                            qDebug() << "Setting bookmarks from global config for:" << temFilePath;
                            window->wrapper(localPath)->textEditor()->setBookMarkList(m_bookmarkTable.value(temFilePath));
                        }

                        if (object.contains("focus")) {  // 包含指定的 key
                            QJsonValue value = object.value("focus");  // 获取指定 key 对应的 value

                            if (value.isBool() && value.toBool()) {
                                qDebug() << "Setting focus to file:" << temFilePath;
                                pFocusWindow = window;
                                focusPath = temFilePath;
                            }
                        }

                        recFilesSum++;
                    }
                } else {
                    if (!localPath.isEmpty() && Utils::fileExists(localPath)) {
                        qDebug() << "Opening local file:" << localPath;
                        // 若为草稿文件或不支持的MIMETYPE文件，显示默认名称标签
                        if (Utils::isDraftFile(localPath) || !Utils::isMimeTypeSupport(localPath)) {
                            //得到新建文件名称
                            int index = files.indexOf(QFileInfo(localPath).fileName());

                            if (index >= 0) {
                                QString fileName = tr("Untitled %1").arg(index + 1);
                                qDebug() << "Using untitled name for draft file:" << fileName;
                                window->addTemFileTab(localPath, fileName, localPath, lastmodifiedtime, bIsTemFile);

                            }
                        } else {
                            qDebug() << "Using file name for regular file:" << fileInfo.fileName();
                            window->addTemFileTab(localPath, fileInfo.fileName(), localPath, lastmodifiedtime, bIsTemFile);
                        }

                        //打开文件后设置书签
                        if (object.contains("bookMark")) {  // 包含指定的 key
                            QJsonValue value = object.value("bookMark");  // 获取指定 key 对应的 value

                            if (value.isString()) {
                                QList<int> bookmarkList;
                                bookmarkList = analyzeBookmakeInfo(value.toString());
                                qDebug() << "Setting bookmarks from file config:" << bookmarkList;
                                window->wrapper(localPath)->textEditor()->setBookMarkList(bookmarkList);
                            }
                        } else if (m_bookmarkTable.contains(localPath)) {
                            // 若文件已有配置，则以文件为准，否则以全局配置为准
                            qDebug() << "Setting bookmarks from global config for:" << localPath;
                            window->wrapper(localPath)->textEditor()->setBookMarkList(m_bookmarkTable.value(localPath));
                        }

                        if (object.contains("focus")) {  // 包含指定的 key
                            QJsonValue value = object.value("focus");  // 获取指定 key 对应的 value

                            if (value.isBool() && value.toBool()) {
                                qDebug() << "Setting focus to file:" << localPath;
                                pFocusWindow = window;
                                focusPath = localPath;
                            }
                        }

                        recFilesSum++;
                    }
                }
                qInfo() << "Recovered files count:" << recFilesSum;
                qDebug() << "Exit recoverFile";
                return recFilesSum;
            }
        }
    }

    //当前活动页
    if (pFocusWindow != nullptr && !focusPath.isNull()) {
        qDebug() << "Setting focus to window and tab for path:" << focusPath;
        FileTabInfo info;
        info.windowIndex = m_windows.indexOf(pFocusWindow);
        info.tabIndex = pFocusWindow->getTabIndex(focusPath);
        popupExistTabs(info);
    }

    qInfo() << "Recovered files count:" << recFilesSum;
    qDebug() << "Exit recoverFile";
    return recFilesSum;
}

void StartManager::openFilesInWindow(QStringList files)
{
    qDebug() << "Enter openFilesInWindow, file count:" << files.size();
    // Open window with blank tab if no files need open.
    if (files.isEmpty()) {
        if (m_windows.count() >= 20) {
            qWarning() << "Maximum window limit (20) reached, cannot create new window";
            return;
        }
        Window *window = createWindow();
        qDebug() << "Created new window with no files";

        if (m_windows.count() > 0) {
            qDebug() << "Multiple windows exist, showing new window without centering";
            window->showCenterWindow(false);
        } else {
            qDebug() << "First window, showing centered";
            window->showCenterWindow(true);
        }

        window->addBlankTab();
        window->activateWindow();
        qDebug() << "Added blank tab and activated window";
    } else {
        qDebug() << "Processing files to open in window:" << files;
        for (const QString &file : files) {
            QString canonicalFile = QFileInfo(file).canonicalFilePath();
            qDebug() << "Processing file:" << file << "canonical path:" << canonicalFile;
            FileTabInfo info = getFileTabInfo(canonicalFile);

            // Open exist tab if file has opened.
            if (info.windowIndex != -1) {
                qDebug() << "File already open in window" << info.windowIndex << "tab" << info.tabIndex << ", activating";
                popupExistTabs(info);
            }
            // Add new tab in current window.
            else {
                qDebug() << "File not open, creating new window and tab";
                Window *window = createWindow();
                window->showCenterWindow(true);
                window->addTab(canonicalFile);
                qDebug() << "Added tab for file:" << canonicalFile;
            }
        }
    }
    qDebug() << "Exit openFilesInWindow";
}

void StartManager::openFilesInTab(QStringList files)
{
    qDebug() << "Enter openFilesInTab, file count:" << files.size();
    if (files.isEmpty()) {
        qDebug() << "No files to open, checking blank files directory";
        QDir blankDirectory = QDir(QDir(Utils::cleanPath(QStandardPaths::standardLocations(QStandardPaths::AppDataLocation)).first()).filePath("blank-files"));
        QStringList blankFiles = blankDirectory.entryList(QStringList(), QDir::Files);
        qDebug() << "Found" << blankFiles.size() << "blank files";

        if (m_windows.isEmpty()) {
            qDebug() << "No windows exist, creating first window";
            Window *window = createWindow(true);
            window->showCenterWindow(true);

            if (!isTemFilesEmpty()) {
                qDebug() << "Temporary files exist, attempting to recover";
                int recoveredCount = recoverFile(window);
                qDebug() << "Recovered" << recoveredCount << "files";
                if (recoveredCount == 0) {
                    qDebug() << "No files recovered, adding blank tab";
                    window->addBlankTab();
                }
            } else {
                qDebug() << "No temporary files, checking blank files";
                if (blankFiles.isEmpty()) {
                    qDebug() << "No blank files, adding new blank tab";
                    window->addBlankTab();
                } else {
                    qDebug() << "Cleaning up blank files and adding new blank tab";
                    for (auto file : blankFiles) {
                        if (file.contains("blank_file")) {
                            blankDirectory.remove(file);
                            qDebug() << "Removed blank file:" << file;
                        }
                    }

                    window->addBlankTab();
                }
            }
        }
        // Just active first window if no file is need opened.
        else {
            qDebug() << "Windows already exist, creating new window with blank tab";
            Window *window = createWindow();
            window->show();
            window->addBlankTab();
        }
    } else {
        qDebug() << "Processing" << files.size() << "files to open in tabs";
        for (const QString &file : files) {
            QString canonicalFile = QFileInfo(file).canonicalFilePath();
            qDebug() << "Processing file:" << file << "canonical path:" << canonicalFile;
            
            if (!checkPath(canonicalFile)) {
                qDebug() << "File already open, skipping:" << canonicalFile;
                // 存在已打开文件时，进行下一文件判断
                continue;
            }

            FileTabInfo info = getFileTabInfo(canonicalFile);

            // Open exist tab if file has opened.
            if (info.windowIndex != -1) {
                qDebug() << "File found in existing window" << info.windowIndex << ", activating tab" << info.tabIndex;
                popupExistTabs(info);
            }
            // Create new window with file if haven't window exist.
            else if (m_windows.size() == 0) {
                qDebug() << "No windows exist, creating first window for file";
                Window *window = createWindow(true);
                window->showCenterWindow(true);
                QTimer::singleShot(50, [ = ] {
                    qDebug() << "Delayed file opening for:" << canonicalFile;
                    recoverFile(window);
                    window->addTab(canonicalFile);
                });
            }
            // Open file tab in first window of window list.
            else {
                qDebug() << "Opening file in first existing window";
                Window *window = m_windows[0];
                window->addTab(canonicalFile);
                qDebug() << "Added tab for file:" << canonicalFile << "in existing window";
                //window->setWindowState(Qt::WindowActive);
                //通过dbus接口从任务栏激活窗口
                if (!Q_LIKELY(Utils::activeWindowFromDock(window->winId()))) {
                    qDebug() << "Activating window via Qt (DBus activation failed)";
                    window->activateWindow();
                } else {
                    qDebug() << "Window activated via DBus";
                }
            }
        }
    }
    qDebug() << "Exit openFilesInTab";
}

void StartManager::createWindowFromWrapper(const QString &tabName, const QString &filePath, const QString &qstrTruePath, EditWrapper *buffer, bool isModifyed)
{
    qDebug() << "Enter createWindowFromWrapper";
    Window *pWindow = createWindow();
    //window->showCenterWindow();
    QRect rect = pWindow->rect();
    QPoint pos = QCursor::pos() ;/*- window->topLevelWidget()->pos();*/
#if (QT_VERSION < QT_VERSION_CHECK(6, 0, 0))
    QRect desktopRect = QApplication::desktop()->rect();
#else
    QRect desktopRect = QGuiApplication::primaryScreen()->availableGeometry();
#endif
    QPoint startPos = pos;

    if ((pos.x() + rect.width()) > desktopRect.width()) {
        startPos.setX(desktopRect.width() - rect.width());
        qDebug() << "Window position out of desktop, adjusting x";
    } else if (pos.x() < 0) {
        startPos.setX(0);
        qDebug() << "Window position out of desktop, adjusting x";
    }

    if ((pos.y() + rect.height()) > desktopRect.height()) {
        startPos.setY(desktopRect.height() - rect.height());
        qDebug() << "Window position out of desktop, adjusting y";
    } else if (pos.y() < 0) {
        startPos.setY(0);
        qDebug() << "Window position out of desktop, adjusting y";
    }

    QRect startRect(startPos, Tabbar::sm_pDragPixmap->rect().size());
    //QRect startRect(startPos, QSize(0,0));
    QRect endRect(startPos, pWindow->rect().size());
    pWindow->move(startPos);
#if 0
    // window->setFixedSize(Tabbar::sm_pDragPixmap->rect().size());
    QLabel *pLab = new QLabel();
    //pLab->resize(Tabbar::sm_pDragPixmap->rect().size());
    pLab->move(pos);
    pLab->setPixmap(*Tabbar::sm_pDragPixmap);
    pLab->show();
#endif
    //添加编辑窗口drop动态显示效果　梁卫东　２０２０－０８－２５　０９：５４：５７
    QPropertyAnimation *geometry = new QPropertyAnimation(pWindow, "geometry");
    geometry->setDuration(200);
    geometry->setStartValue(startRect);
    geometry->setEndValue(endRect);
    geometry->setEasingCurve(QEasingCurve::InCubic);

    //OutCubic InCubic
#if 0
    QPropertyAnimation *Opacity = new QPropertyAnimation(this, "windowOpacity");
    connect(Opacity, &QPropertyAnimation::finished, Opacity, &QPropertyAnimation::deleteLater);
    Opacity->setDuration(200);
    Opacity->setStartValue(1.0);
    Opacity->setEndValue(0);
    Opacity->setEasingCurve(QEasingCurve::InCirc);
#endif

    QParallelAnimationGroup *group = new QParallelAnimationGroup;
    connect(group, &QParallelAnimationGroup::finished, this, [/*window,geometry,Opacity,group,*/ = ]() {
        pWindow->show();
        pWindow->showCenterWindow(false);
        geometry->deleteLater();
        group->deleteLater();

        pWindow->addTabWithWrapper(buffer, filePath, qstrTruePath, tabName);
        pWindow->currentWrapper()->updateModifyStatus(isModifyed);
        pWindow->currentWrapper()->OnUpdateHighlighter();
        pWindow->setFocus();
        pWindow->setFontSizeWithConfig(pWindow->currentWrapper());
    });

    group->addAnimation(geometry);
    // group->addAnimation(Opacity);
    group->start();
    qDebug() << "Exit createWindowFromWrapper";
}

void StartManager::loadTheme(const QString &themeName)
{
    qDebug() << "Enter loadTheme, themeName:" << themeName;
    for (Window *window : m_windows) {
        window->loadTheme(themeName);
    }
    qDebug() << "Exit loadTheme";
}

Window *StartManager::createWindow(bool alwaysCenter)
{
    qDebug() << "Enter createWindow, alwaysCenter:" << alwaysCenter;
    // Create window.
    Window *window = new Window;
    qDebug() << "Window object created, connecting signals";
    connect(window, &Window::themeChanged, this, &StartManager::loadTheme, Qt::QueuedConnection);
    connect(window, &Window::sigJudgeBlockShutdown, this, &StartManager::slotCheckUnsaveTab, Qt::QueuedConnection);
    connect(window, &Window::tabChanged, this, &StartManager::slotDelayBackupFile, Qt::QueuedConnection);

    // Quit application if close last window.
    connect(window, &Window::closeWindow, this, &StartManager::slotCloseWindow);
    qDebug() << "Window signals connected";

    // Init window position.
    initWindowPosition(window, alwaysCenter);
    qDebug() << "Window position initialized, alwaysCenter:" << alwaysCenter;

    connect(window, &Window::newWindow, this, &StartManager::slotCreatNewwindow);

    // 标签页拖拽状态变更时触发，防止在拖拽过程中备份导致获取的标签状态异常
    connect(window->getTabbar(), &DTabBar::dragStarted, this, [this](){
        qDebug() << "Tab drag started, pausing backup";
        m_bIsTagDragging = true;
    });
    connect(window->getTabbar(), &DTabBar::dragEnd, this, [this](){
        qDebug() << "Tab drag ended, resuming backup";
        m_bIsTagDragging = false;
        slotDelayBackupFile();
    });

    // Append window in window list.
    m_windows << window;

    qInfo() << "Window created, total windows:" << m_windows.size();
    qDebug() << "Exit createWindow";
    return window;
}


void StartManager::slotCloseWindow()
{
    qDebug() << "Enter slotCloseWindow";
    Window *pWindow = static_cast<Window *>(sender());
    int windowIndex = m_windows.indexOf(pWindow);

    if (windowIndex >= 0) {
        qDebug() << "Window closed, index:" << windowIndex;
        m_windows.takeAt(windowIndex);
    }

    if (m_windows.isEmpty()) {
        qDebug() << "No remaining windows, saving bookmark";
        // 保存书签信息
        saveBookmark();

        QDir path = QDir::currentPath();
        if (!path.exists()) {
            return ;
            qInfo() << "Window closed, remaining windows:" << m_windows.size();
            qDebug() << "Exit slotCloseWindow";
        }
        path.setFilter(QDir::Files);
        QStringList nameList = path.entryList();
        foreach (auto name, nameList) {
            if (name.contains("tabPaths.txt")) {
                qDebug() << "Delete tabPaths.txt";
                QFile file(name);
                file.remove();
            }
        }

        // 在程序退出前（最后一个窗口关闭后），手动注销DBus服务，防止重启文本编辑器时判断应用仍在占用总线
        // 导致被附加到上一文本编辑器进程
        QDBusConnection::sessionBus().unregisterService("com.deepin.Editor");

        QTimer::singleShot(1000, []() {
            QApplication::quit();
        });

        PerformanceMonitor::closeAPPFinish();
    }
    qDebug() << "Exit slotCloseWindow";
}

void StartManager::slotDelayBackupFile()
{
    qDebug() << "Enter slotDelayBackupFile";
    // 判断定时器是否已在触发状态，防止短时间内的多次触发，标签页拖拽状态不触发
    if (!m_DelayTimer.isActive() && !m_bIsTagDragging) {
        qDebug() << "Start timer for delay backup";
        m_DelayTimer.start(EDelayBackupInterval, this);
    }
    qDebug() << "Exit slotDelayBackupFile";
}

void StartManager::timerEvent(QTimerEvent *e)
{
    qDebug() << "Enter timerEvent";
    // 判断是否为延迟备份
    if (e->timerId() == m_DelayTimer.timerId()) {
        qDebug() << "Timer expired, backup file";
        m_DelayTimer.stop();
        // 执行配置文件备份
        autoBackupFile();

        // 重启周期定时备份
        m_pTimer->start(EAutoBackupInterval);
    }
    qDebug() << "Exit timerEvent";
}

void StartManager::slotCreatNewwindow()
{
    qDebug() << "Enter slotCreatNewwindow";
    openFilesInWindow(QStringList());
}

void StartManager::initWindowPosition(Window *window, bool alwaysCenter)
{
    qDebug() << "Enter initWindowPosition";
    if (m_windows.isEmpty() || alwaysCenter) {
        qDebug() << "No existing windows or alwaysCenter is true, centering window";
        //Dtk::Widget::moveToCenter(window);
    } else {
        qDebug() << "Adding window offset to avoid all editor window popup at same coordinate";
        // Add window offset to avoid all editor window popup at same coordinate.
        int windowOffset = m_windows.size() * 50;
#if (QT_VERSION < QT_VERSION_CHECK(6, 0, 0))
        QRect screenGeometry = qApp->desktop()->screenGeometry(QCursor::pos());
#else
        QRect screenGeometry = QGuiApplication::primaryScreen()->availableGeometry();
#endif
        window->move(screenGeometry.x() + windowOffset, screenGeometry.y() + windowOffset);
    }
    qDebug() << "Exit initWindowPosition";
}

void StartManager::popupExistTabs(FileTabInfo info)
{
    qDebug() << "Enter popupExistTabs";
    Window *window = m_windows[info.windowIndex];
    //window->showNormal();
    window->activeTab(info.tabIndex);
    //window->setWindowState(Qt::WindowActive);
    //通过dbus接口从任务栏激活窗口
    if (!Q_LIKELY(Utils::activeWindowFromDock(window->winId()))) {
        qDebug() << "Window not active from Dock, activating manually";
        window->activateWindow();
    }

#if 0
    int indexid = 0;
    uint winid = 0;
    QDBusInterface dock("com.deepin.dde.daemon.Dock",
                        "/com/deepin/dde/daemon/Dock",
                        "com.deepin.dde.daemon.Dock",
                        QDBusConnection::sessionBus()
                       );
    QDBusReply<QStringList> rep = dock.call("GetEntryIDs");

    for (auto name : rep.value()) {
        if (name == "deepin-editor") {
            indexid = rep.value().indexOf(name);
        }
    }

    m_pDock.reset(new Dock("com.deepin.dde.daemon.Dock",
                           "/com/deepin/dde/daemon/Dock",
                           QDBusConnection::sessionBus(),
                           this
                          )
                 );
    QList<QDBusObjectPath> list = m_pDock->entries();

    m_pEntry.reset(new Entry("com.deepin.dde.daemon.Dock",
                             list[indexid].path(),
                             QDBusConnection::sessionBus(),
                             this));
    winid = m_pEntry->currentWindow() ;


    QDBusMessage active = QDBusMessage::createMethodCall("com.deepin.dde.daemon.Dock",
                                                         "/com/deepin/dde/daemon/Dock",
                                                         "com.deepin.dde.daemon.Dock",
                                                         "ActivateWindow");
    active << winid;
    QDBusConnection::sessionBus().call(active, QDBus::BlockWithGui);
#endif
    qDebug() << "Exit popupExistTabs";
}

StartManager::FileTabInfo StartManager::getFileTabInfo(QString file)
{
    qDebug() << "Enter getFileTabInfo";
    FileTabInfo info = {-1, -1};

    //qDebug() << "Windows size: " << m_windows.size();

    for (Window *window : m_windows) {
        int tabIndex = window->getTabIndex(file);
        if (tabIndex >= 0) {
            info.windowIndex = m_windows.indexOf(window);
            info.tabIndex = tabIndex;
            qDebug() << "Break, File found in window " << info.windowIndex << " tab " << tabIndex;
            break;
        }
    }

    qDebug() << "Exit getFileTabInfo";
    return info;
}

QList<int> StartManager::analyzeBookmakeInfo(QString bookmarkInfo)
{
    qDebug() << "Enter analyzeBookmakeInfo";
    QList<int> bookmarkList;
    int nLeftPosition = 0;
    int nRightPosition = bookmarkInfo.indexOf(",");
    bookmarkList << bookmarkInfo.mid(nLeftPosition, nRightPosition - nLeftPosition).toInt();
    nLeftPosition = nRightPosition;

    while (nRightPosition != -1) {
        nRightPosition = bookmarkInfo.indexOf(",", nRightPosition + 1);
        bookmarkList << bookmarkInfo.mid(nLeftPosition + 1, nRightPosition - nLeftPosition - 1).toInt();
        nLeftPosition = nRightPosition;
    }

    qDebug() << "Exit analyzeBookmakeInfo";
    return bookmarkList;
}

/**
 * @brief 主动更新记录书签信息
 * @param localPath 文件路径
 * @param bookmark  书签信息
 */
void StartManager::recordBookmark(const QString &localPath, const QList<int> &bookmark)
{
    qDebug() << "Enter recordBookmark";
    m_bookmarkTable.insert(localPath, bookmark);
    qDebug() << "Exit recordBookmark";
}

/**
 * @return 返回文件 \a localPath 的书签记录
 */
QList<int> StartManager::findBookmark(const QString &localPath)
{
    qDebug() << "Enter findBookmark";
    return m_bookmarkTable.value(localPath);
}

void StartManager::initBlockShutdown()
{
    qDebug() << "Enter initBlockShutdown";
    if (m_reply.value().isValid()) {
        qDebug() << "reply is valid, block shutdown";
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

    m_reply = m_pLoginManager->callWithArgumentList(QDBus::Block, "Inhibit", m_arg);
    if (m_reply.isValid()) {
        qDebug() << "Inhibit reply is valid";
        //m_reply.value().fileDescriptor();
    }
    qDebug() << "Exit initBlockShutdown";
}

/**
 * @brief 从配置文件中获取全局的书签信息，包括已关闭的所有文件书签，
 *      会遍历每个书签记录并判断文件是否存在，若文件被删除或移动，则不再记录对应的书签信息。
 */
void StartManager::initBookmark()
{
    qDebug() << "Enter initBookmark";
    // 遍历书签信息列表
    QStringList bookmarkInfoList = Settings::instance()->settings->value(s_bookMarkKey).toStringList();
    for (const QString &bookmarkInfo : bookmarkInfoList) {
        QJsonParseError readError;
        QJsonDocument doc = QJsonDocument::fromJson(bookmarkInfo.toUtf8(), &readError);
        if (QJsonParseError::NoError == readError.error
                && !doc.isNull()) {
            QJsonObject obj = doc.object();
            QString filePath = obj.value("localPath").toString();

            // 判断文件是否仍存在，若不存在，则不保留书签信息
            if (!filePath.isEmpty()
                    && QFileInfo::exists(filePath)) {
                QString bookmarkStr = obj.value("bookmark").toString();
                if (!bookmarkStr.isEmpty()) {
                    qDebug() << "bookmarkStr:" << bookmarkStr;
                    QList<int> bookmarkList = analyzeBookmakeInfo(bookmarkStr);
                    if (!bookmarkList.isEmpty()) {
                        qDebug() << "bookmarkList:" << bookmarkList;
                        // 文件存在且书签标记非空，缓存书签信息
                        m_bookmarkTable.insert(filePath, bookmarkList);
                    }
                }
            }
        }
    }
    qDebug() << "Exit initBookmark";
}

/**
 * @brief 将当前记录的所有文件的书签信息转换为Json数据列表记录到配置信息中，
 *      被删除或移动文件的书签将被销毁。
 */
void StartManager::saveBookmark()
{
    qDebug() << "Enter saveBookmark";
    QStringList recordInfo;
    // 遍历记录
    for (auto itr = m_bookmarkTable.begin(); itr != m_bookmarkTable.end();) {
        if (!QFileInfo::exists(itr.key())
                || itr.value().isEmpty()) {
            // 文件不存在则销毁记录
            itr = m_bookmarkTable.erase(itr);
            qDebug() << "File not exist, remove bookmark record";
        } else {
            qDebug() << "File exist, save bookmark record";
            QJsonObject obj;
            obj.insert("localPath", itr.key());

            // 遍历书签信息并组合
            QString bookmarkInfo;
            const auto &markList = itr.value();
            for (int i = 0; i < markList.count(); i++) {
                if (i == itr.value().count() - 1) {
                    bookmarkInfo.append(QString::number(markList.value(i)));
                } else {
                    bookmarkInfo.append(QString::number(markList.value(i)) + ",");
                }
            }
            obj.insert("bookmark", bookmarkInfo);

            QJsonDocument doc(obj);
            recordInfo.append(doc.toJson(QJsonDocument::Compact));

            ++itr;
            qDebug() << "Exit slotCheckUnsaveTab";
        }
    }

    // 将书签信息保存至配置文件
    Settings::instance()->settings->option(s_bookMarkKey)->setValue(recordInfo);
    qDebug() << "Exit saveBookmark";
}

void StartManager::slotCheckUnsaveTab()
{
    qDebug() << "Enter slotCheckUnsaveTab";
    for (Window *pWindow : m_windows) {
        //如果返回true，则表示有未保存的tab项，则阻塞系统关机
        bool bRet = pWindow->checkBlockShutdown();
        if (bRet == true) {
            m_reply = m_pLoginManager->callWithArgumentList(QDBus::Block, "Inhibit", m_arg);
            if (m_reply.isValid()) {
                qDebug() << "Inhibit reply is valid";
            }

            qDebug() << "Find unsaved tab, block shutdown";
            return;
        }
    }

    //如果for结束则表示没有发现未保存的tab项，则放开阻塞关机
    if (m_reply.isValid()) {
        qDebug() << "No unsaved tab, unblock shutdown";
        m_reply = QDBusReply<QDBusUnixFileDescriptor>();
    }
    qDebug() << "Exit slotCheckUnsaveTab";
}

void StartManager::closeAboutForWindow(Window *window)
{
    qDebug() << "Enter closeAboutForWindow";
    if (qApp != nullptr) {
        DAboutDialog *pAboutDialog = qApp->aboutDialog();
        if (pAboutDialog != nullptr) {
            if (pAboutDialog->parent() != nullptr) {
                if (pAboutDialog->parent() == window) {
                    pAboutDialog->close();
                    qDebug() << "Close about dialog for window";
                }
            }
        }
    }
    qDebug() << "Exit closeAboutForWindow";
}
