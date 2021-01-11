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

#include "window.h"

#include <DTitlebar>
#include <DAnchors>
#include <DThemeManager>
#include <DToast>
#include <DSettingsWidgetFactory>
#include <DSettingsGroup>
#include <DSettings>
#include <DSettingsOption>
#include <QApplication>
#include <QPrintDialog>
#include <QPrintPreviewDialog>
#include <QPrinter>
#include <QScreen>
#include <QStyleFactory>
#include <QEvent>
#include <DDialog>
#include <QStackedWidget>
#include <QResizeEvent>
#include <QVBoxLayout>
#include <DGuiApplicationHelper>
#include <DMessageManager>
#include <DGuiApplicationHelper>
#include <DPrintPreviewDialog>
#include <QGuiApplication>
#include <QWindow>
#include <DWidgetUtil>

#ifdef DTKWIDGET_CLASS_DFileDialog

#include <DFileDialog>
#else
#include <QFileDialog>
#endif

Window::Window(DMainWindow *parent)
    : DMainWindow(parent),
      m_centralWidget(new QWidget),
      m_editorWidget(new QStackedWidget),
      m_centralLayout(new QVBoxLayout(m_centralWidget)),
      m_tabbar(new Tabbar),
      m_jumpLineBar(new JumpLineBar()),
      m_replaceBar(new ReplaceBar(this)),
      m_themePanel(new ThemePanel(this)),
      m_findBar(new FindBar(this)),
      m_menu(new DMenu),
      m_titlebarStyleSheet(titlebar()->styleSheet())
{
    qRegisterMetaType<TextEdit*>("TextEdit");
    m_blankFileDir = QDir(QStandardPaths::standardLocations(QStandardPaths::DataLocation).first()).filePath("blank-files");
    m_backupDir = QDir(QStandardPaths::standardLocations(QStandardPaths::DataLocation).first()).filePath("backup-files");
    m_autoBackupDir = QDir(QStandardPaths::standardLocations(QStandardPaths::DataLocation).first()).filePath("autoBackup-files");
    m_themePath = Settings::instance()->settings->option("advance.editor.theme")->value().toString();
    m_rootSaveDBus = new DBusDaemon::dbus("com.deepin.editor.daemon", "/", QDBusConnection::systemBus(), this);
    m_settings = Settings::instance();

    // Init.
    setAcceptDrops(true);
    loadTheme(m_themePath);

    //关闭　替换　查找 跳行bar
    connect(this, &Window::pressEsc, m_replaceBar, &ReplaceBar::pressEsc, Qt::QueuedConnection);
    connect(this, &Window::pressEsc, m_findBar, &FindBar::pressEsc, Qt::QueuedConnection);
    connect(this, &Window::pressEsc, m_jumpLineBar, &JumpLineBar::pressEsc, Qt::QueuedConnection);

    // Init settings.
    connect(m_settings, &Settings::sigAdjustFont, this,[this](QString fontName){
       for (EditWrapper *wrapper : m_wrappers.values()) {
           wrapper->textEditor()->setFontFamily(fontName);
       }
    });

    connect(m_settings, &Settings::sigAdjustFontSize, this,[this](int size){
       for (EditWrapper *wrapper : m_wrappers.values()) {
           wrapper->textEditor()->setFontSize(size);
       }

       m_fontSize = size;
    });

    connect(m_settings, &Settings::sigAdjustTabSpaceNumber, this,[this](int number){
            for (EditWrapper *wrapper : m_wrappers.values()) {
                wrapper->textEditor()->setTabSpaceNumber(number);
            }
     });

    connect(m_settings, &Settings::sigThemeChanged, this, [](const QString &path)
    {
        QString strLightTheme = "/usr/share/deepin-editor/themes/deepin.theme";
        QString strDarkTheme = "/usr/share/deepin-editor/themes/deepin_dark.theme";

        if (path == strLightTheme) {
            if (DGuiApplicationHelper::instance()->themeType() == DGuiApplicationHelper::LightType) {
                return;
            } else {
                DGuiApplicationHelper::instance()->setPaletteType(DGuiApplicationHelper::LightType);
                //DGuiApplicationHelper::instance()->setThemeType(DGuiApplicationHelper::LightType);
            }
        } else if (path == strDarkTheme) {
            if (DGuiApplicationHelper::instance()->themeType() == DGuiApplicationHelper::DarkType) {
                return;
            } else {
                DGuiApplicationHelper::instance()->setPaletteType(DGuiApplicationHelper::DarkType);
                //DGuiApplicationHelper::instance()->setThemeType(DGuiApplicationHelper::DarkType);
            }
        }
    });


    connect(m_settings, &Settings::sigAdjustWordWrap, this, [ = ](bool enable) {
        for (EditWrapper *wrapper : m_wrappers.values()) {
            TextEdit *textedit = wrapper->textEditor();
            textedit->setLineWrapMode(enable);
        }
    });

    connect(m_settings,&Settings::sigSetLineNumberShow,this,[=] (bool bIsShow) {
       for(EditWrapper *wrapper : m_wrappers.values()) {
           wrapper->setLineNumberShow(bIsShow);
       }
    });

    connect(m_settings,&Settings::sigAdjustBookmark,this,[=] (bool bIsShow) {
       for(EditWrapper *wrapper : m_wrappers.values()) {
           TextEdit *textedit = wrapper->textEditor();
           textedit->setBookmarkFlagVisable(bIsShow);
       }
    });

    //添加显示空白符 梁卫东　２０２０－０８－１４　１５：２６：３２
    connect(m_settings, &Settings::sigShowBlankCharacter, this, [=] (bool bIsShow) {
       for(EditWrapper *wrapper : m_wrappers.values()) {
           wrapper->setShowBlankCharacter(bIsShow);
       }
    });

    //setHighLineCurrentLine
    connect(m_settings, &Settings::sigHightLightCurrentLine, this, [=] (bool bIsShow) {
       for(EditWrapper *wrapper : m_wrappers.values()) {
           wrapper->textEditor()->setHighLineCurrentLine(bIsShow);
       }
    });

    connect(m_settings, &Settings::sigShowCodeFlodFlag, this, [ = ](bool enable) {
        for (EditWrapper *wrapper : m_wrappers.values()) {
            TextEdit *textedit = wrapper->textEditor();
            textedit->setCodeFlodFlagVisable(enable);
        }
    });

    connect(m_settings, &Settings::sigChangeWindowSize, this, [ = ](QString mode) {
        if (mode == "fullscreen") {
            this->showFullScreen();
        } else if (mode == "window_maximum") {
            this->showNormal();
            this->showMaximized();
        }
        else {
            this->showNormal();
        }
    });

    // Init layout and editor.
    m_centralLayout->setMargin(0);
    m_centralLayout->setSpacing(0);

    m_centralLayout->addWidget(m_editorWidget);
    setWindowIcon(QIcon::fromTheme("deepin-editor"));
    setCentralWidget(m_centralWidget);

    // Init titlebar.
    if (titlebar()) {
        initTitlebar();
    }

    // window minimum size.
    setMinimumSize(1000, 600);
    // resize window size.
    int window_width =Settings::instance()->settings->option("advance.window.window_width")->value().toInt();
    int window_height =Settings::instance()->settings->option("advance.window.window_height")->value().toInt();
    resize(window_width,window_height);


    // Init find bar.
    connect(m_findBar, &FindBar::findNext, this, &Window::handleFindNext, Qt::QueuedConnection);
    connect(m_findBar, &FindBar::findPrev, this, &Window::handleFindPrev, Qt::QueuedConnection);
    connect(m_findBar, &FindBar::removeSearchKeyword, this, &Window::handleRemoveSearchKeyword, Qt::QueuedConnection);
    connect(m_findBar, &FindBar::updateSearchKeyword, this, [ = ](QString file, QString keyword) {
        handleUpdateSearchKeyword(m_findBar, file, keyword);
    });
    connect(m_findBar, &FindBar::sigFindbarClose, this, &Window::slotFindbarClose, Qt::QueuedConnection);

    // Init replace bar.
    //connect(m_replaceBar, &ReplaceBar::removeSearchKeyword, this, &Window::handleRemoveSearchKeyword, Qt::QueuedConnection);

    connect(m_replaceBar, &ReplaceBar::beforeReplace,this, &Window::slot_beforeReplace, Qt::QueuedConnection);
    connect(m_replaceBar, &ReplaceBar::replaceAll, this, &Window::handleReplaceAll, Qt::QueuedConnection);
    connect(m_replaceBar, &ReplaceBar::replaceNext, this, &Window::handleReplaceNext, Qt::QueuedConnection);
    connect(m_replaceBar, &ReplaceBar::replaceRest, this, &Window::handleReplaceRest, Qt::QueuedConnection);
    connect(m_replaceBar, &ReplaceBar::replaceSkip, this, &Window::handleReplaceSkip, Qt::QueuedConnection);
    connect(m_replaceBar, &ReplaceBar::updateSearchKeyword, this, [ = ](QString file, QString keyword) {
        handleUpdateSearchKeyword(m_replaceBar, file, keyword);
    });
    connect(m_replaceBar, &ReplaceBar::sigReplacebarClose, this, &Window::slotReplacebarClose, Qt::QueuedConnection);

    // Init jump line bar.
    //QTimer::singleShot(0, m_jumpLineBar, SLOT(hide()));
    m_jumpLineBar->hide();
    m_jumpLineBar->setParent(this);

    connect(m_jumpLineBar, &JumpLineBar::jumpToLine, this, &Window::handleJumpLineBarJumpToLine, Qt::QueuedConnection);
    connect(m_jumpLineBar, &JumpLineBar::backToPosition, this, &Window::handleBackToPosition, Qt::QueuedConnection);
    connect(m_jumpLineBar, &JumpLineBar::lostFocusExit, this, &Window::handleJumpLineBarExit, Qt::QueuedConnection);

    // Make jump line bar pop at top-right of editor.
    //DAnchorsBase::setAnchor(m_jumpLineBar, Qt::AnchorTop, m_centralWidget, Qt::AnchorTop);
    //DAnchorsBase::setAnchor(m_jumpLineBar, Qt::AnchorRight, m_centralWidget, Qt::AnchorRight);

    DAnchors<JumpLineBar> anchors_jumpLineBar(m_jumpLineBar);
    anchors_jumpLineBar.setAnchor(Qt::AnchorTop, m_centralWidget, Qt::AnchorTop);
    anchors_jumpLineBar.setAnchor(Qt::AnchorRight, m_centralWidget, Qt::AnchorRight);
    anchors_jumpLineBar.setTopMargin(5);
    anchors_jumpLineBar.setRightMargin(5);
    m_jumpLineBar->raise();

    // Init findbar panel.
    static DAnchors<FindBar> anchors_findbar(m_findBar);
    anchors_findbar.setAnchor(Qt::AnchorBottom, m_centralWidget, Qt::AnchorBottom);
    anchors_findbar.setAnchor(Qt::AnchorHorizontalCenter, m_centralWidget, Qt::AnchorHorizontalCenter);
    anchors_findbar.setBottomMargin(1);
    //m_findBar->move(QPoint(10, height() - 5));
    m_findBar->raise();

    // Init replaceBar panel.
    DAnchors<ReplaceBar> anchors_replaceBar(m_replaceBar);
    anchors_replaceBar.setAnchor(Qt::AnchorBottom, m_centralWidget, Qt::AnchorBottom);
    anchors_replaceBar.setAnchor(Qt::AnchorHorizontalCenter, m_centralWidget, Qt::AnchorHorizontalCenter);
    anchors_replaceBar.setBottomMargin(1);
    //m_replaceBar->move(QPoint(10, height() - 57));
    m_replaceBar->raise();

    // Init theme panel.
    DAnchorsBase::setAnchor(m_themePanel, Qt::AnchorTop, m_centralWidget, Qt::AnchorTop);
    DAnchorsBase::setAnchor(m_themePanel, Qt::AnchorBottom, m_centralWidget, Qt::AnchorBottom);
    DAnchorsBase::setAnchor(m_themePanel, Qt::AnchorRight, m_centralWidget, Qt::AnchorRight);

    // for the first time open the need be init.
    m_themePanel->setSelectionTheme(m_themePath);

    connect(m_themePanel, &ThemePanel::themeChanged, this, &Window::themeChanged);
    connect(this, &Window::requestDragEnterEvent, this, &Window::dragEnterEvent);
    connect(this, &Window::requestDropEvent, this, &Window::dropEvent);

    connect(qApp, &QGuiApplication::focusWindowChanged, this, &Window::handleFocusWindowChanged);
    connect(DGuiApplicationHelper::instance(), &DGuiApplicationHelper::themeTypeChanged, this, &Window::slotLoadContentTheme);

     //setChildrenFocus(false);
     Utils::clearChildrenFocus(m_tabbar);//使用此函数把tabbar的组件焦点去掉(左右箭头不能focus)
}

Window::~Window()
{
    // We don't need clean pointers because application has exit here.
}

void Window::updateModifyStatus(const QString &path, bool isModified)
{
    int tabIndex = m_tabbar->indexOf(path);
    if(tabIndex == -1) return;
    QString tabName = m_tabbar->textAt(tabIndex);
    QRegularExpression reg("[^*](.+)");
    QRegularExpressionMatch match = reg.match(tabName);

    tabName = match.captured(0);
    if (isModified) tabName.prepend('*');
    m_tabbar->setTabText(tabIndex, tabName);
    //判断是否需要阻塞系统关机
    emit sigJudgeBlockShutdown();
}

void Window::updateSaveAsFileName(QString strOldFilePath, QString strNewFilePath)
{
    int tabIndex = m_tabbar->indexOf(strOldFilePath);
    EditWrapper *wrapper = m_wrappers.value(strOldFilePath);
    m_tabbar->updateTab(tabIndex, strNewFilePath, QFileInfo(strNewFilePath).fileName());
    wrapper->updatePath(strNewFilePath);
    m_wrappers.remove(strOldFilePath);
    m_wrappers.insert(strNewFilePath, wrapper);
}

void Window::showCenterWindow(bool bIsCenter)
{
    // Init window state with config.
    // Below code must before this->titlebar()->setMenu, otherwise main menu can't display pre-build-in menu items by dtk.
    const QString &windowState = Settings::instance()->settings->option("advance.window.windowstate")->value().toString();

    if (bIsCenter) {
        Dtk::Widget::moveToCenter(this);
        show();
    }

    // init window state.
    if (windowState == "window_maximum") {
        showMaximized();
    } else if (windowState == "fullscreen") {
        showFullScreen();
    } else {
        showNormal();
    }
}

void Window::initTitlebar()
{
    QAction *newWindowAction(new QAction(tr("New window"), this));
    QAction *newTabAction(new QAction(tr("New tab"), this));
    QAction *openFileAction(new QAction(tr("Open file"), this));
    QAction *saveAction(new QAction(tr("Save"), this));
    QAction *saveAsAction(new QAction(tr("Save as"), this));
    QAction *printAction(new QAction(tr("Print"), this));
    QAction *switchThemeAction(new QAction(tr("Switch theme"), this));
    QAction *settingAction(new QAction(tr("Settings"), this));
    QAction *findAction(new QAction(QApplication::translate("TextEdit", "Find"), this));
    QAction *replaceAction(new QAction(QApplication::translate("TextEdit", "Replace"), this));

    m_menu->addAction(newWindowAction);
    m_menu->addAction(newTabAction);
    m_menu->addAction(openFileAction);
    m_menu->addSeparator();
    m_menu->addAction(findAction);
    m_menu->addAction(replaceAction);
    m_menu->addAction(saveAction);
    m_menu->addAction(saveAsAction);
    m_menu->addAction(printAction);
    //m_menu->addAction(switchThemeAction);
    m_menu->addSeparator();
    m_menu->addAction(settingAction);

    m_menu->setMinimumWidth(150);

    titlebar()->addWidget(m_tabbar);

    titlebar()->setCustomWidget(m_tabbar, false);
    titlebar()->setSeparatorVisible(false);
    titlebar()->setMenu(m_menu);
    titlebar()->setIcon(QIcon::fromTheme("deepin-editor"));

    titlebar()->setFocusPolicy(Qt::NoFocus);         //设置titlebar无焦点，点击titlebar时光标不移动

    DIconButton *addButton = m_tabbar->findChild<DIconButton *>("AddButton");
    addButton->setFocusPolicy(Qt::NoFocus);
    DIconButton *optionBtn = titlebar()->findChild<DIconButton *>("DTitlebarDWindowOptionButton");
    optionBtn->setFocusPolicy(Qt::NoFocus);
    DIconButton *minBtn = titlebar()->findChild<DIconButton *>("DTitlebarDWindowMinButton");
    minBtn->setFocusPolicy(Qt::NoFocus);
    DIconButton *quitFullBtn = titlebar()->findChild<DIconButton *>("DTitlebarDWindowQuitFullscreenButton");
    quitFullBtn->setFocusPolicy(Qt::NoFocus);
    DIconButton *maxBtn = titlebar()->findChild<DIconButton *>("DTitlebarDWindowMaxButton");
    maxBtn->setFocusPolicy(Qt::NoFocus);
    DIconButton *closeBtn = titlebar()->findChild<DIconButton *>("DTitlebarDWindowCloseButton");
    closeBtn->setFocusPolicy(Qt::NoFocus);

    connect(m_tabbar, &DTabBar::tabBarDoubleClicked, titlebar(), &DTitlebar::doubleClicked, Qt::QueuedConnection);

    connect(m_tabbar, &Tabbar::closeTabs, this, &Window::handleTabsClosed, Qt::QueuedConnection);
    connect(m_tabbar, &Tabbar::requestHistorySaved, this, [ = ](const QString & filePath) {
        if (QFileInfo(filePath).dir().absolutePath() == m_blankFileDir) {
            return;
        }

        if (!m_closeFileHistory.contains(filePath)) {
            m_closeFileHistory << filePath;
        }
    });

    connect(m_tabbar, &DTabBar::tabCloseRequested, this, &Window::handleTabCloseRequested, Qt::QueuedConnection);
    connect(m_tabbar, &DTabBar::tabAddRequested, this, static_cast<void (Window::*)()>(&Window::addBlankTab), Qt::QueuedConnection);
    connect(m_tabbar, &DTabBar::currentChanged, this, &Window::handleCurrentChanged, Qt::QueuedConnection);



    connect(newWindowAction, &QAction::triggered, this, &Window::newWindow);
    connect(newTabAction, &QAction::triggered, this, static_cast<void (Window::*)()>(&Window::addBlankTab));
    connect(openFileAction, &QAction::triggered, this, &Window::openFile);
    connect(findAction, &QAction::triggered, this, &Window::popupFindBar);
    connect(replaceAction, &QAction::triggered, this, &Window::popupReplaceBar);
    connect(saveAction, &QAction::triggered, this, &Window::saveFile);
    connect(saveAsAction, &QAction::triggered, this, &Window::saveAsFile);
    connect(printAction, &QAction::triggered, this, &Window::popupPrintDialog);
    connect(settingAction, &QAction::triggered, this, &Window::popupSettingsDialog);
    connect(switchThemeAction, &QAction::triggered, this, &Window::popupThemePanel);
}

bool Window::checkBlockShutdown()
{
//    qDebug() << "Enter function [" << __FUNCTION__ << "].";
    //判断是否有未保存的tab项
    for (int i = 0; i < m_tabbar->count(); i++) {
//        qDebug() << "m_tabbar->textAt(i):" << m_tabbar->textAt(i);
        if (m_tabbar->textAt(i).isNull()) {
            return false;
        }
        //如果有未保存的tab项，return true阻塞系统关机
        if (m_tabbar->textAt(i).at(0) == '*') {
            return true;
        }
    }

    return false;
}

int Window::getTabIndex(const QString &file)
{
    return m_tabbar->indexOf(file);
}

void Window::activeTab(int index)
{
    DMainWindow::activateWindow();
    m_tabbar->setCurrentIndex(index);
}

Tabbar *Window::getTabbar()
{
    return m_tabbar;
}

void Window::addTab(const QString &filepath, bool activeTab)
{
    // check whether it is an editable file thround mimeType.
    if (Utils::isMimeTypeSupport(filepath)) {
        const QFileInfo fileInfo(filepath);
        QString tabName = fileInfo.fileName();
        EditWrapper *curWrapper = currentWrapper();

        if (!fileInfo.isWritable() && fileInfo.isReadable()) {
            tabName += QString(" (%1)").arg(tr("Read-Only"));
        }

        if (curWrapper) {
            // if the current page is a draft file and is empty
            if (m_tabbar->indexOf(filepath) != -1) {
                m_tabbar->setCurrentIndex(m_tabbar->indexOf(filepath));
            }
        }

        // check if have permission to read the file.
        QFile file(filepath);
        QFile::Permissions permissions = file.permissions();
        bool bIsRead = (permissions & QFile::ReadUser ||permissions & QFile::ReadOwner || permissions & QFile::ReadOther);
        if(fileInfo.exists() && !bIsRead)
        {
           DMessageManager::instance()->sendMessage(m_editorWidget->currentWidget(), QIcon(":/images/warning.svg")
                                                    , QString(tr("You do not have permission to open %1")).arg(filepath));
           return;
        }

        if (StartManager::instance()->checkPath(filepath)) {

            m_tabbar->addTab(filepath, tabName);

            if (!m_wrappers.contains(filepath)) {
                EditWrapper *wrapper = createEditor();
                showNewEditor(wrapper);
                m_wrappers[filepath] = wrapper;

                if (!fileInfo.isWritable() && fileInfo.isReadable()) {
                    wrapper->textEditor()->setReadOnlyPermission(true);
                }

                wrapper->openFile(filepath,filepath);
            }
            // Activate window.
            activateWindow();
        }

        // Active tab if activeTab is true.
        if (activeTab) {
            int tabIndex = m_tabbar->indexOf(filepath);
            if (tabIndex != -1) {
                m_tabbar->setCurrentIndex(tabIndex);
            }
        }
    } else {
        if (currentWrapper() == nullptr) {
            this->addBlankTab();
        }
        DMessageManager::instance()->sendMessage(m_editorWidget->currentWidget(), QIcon(":/images/warning.svg")
                                                 , tr("Invalid file: %1").arg(QFileInfo(filepath).fileName()));
    }
}

void Window::addTabWithWrapper(EditWrapper *wrapper, const QString &filepath, const QString &qstrTruePath, const QString &tabName, int index)
{
    if (index == -1) {
        index = m_tabbar->currentIndex() + 1;
    }

    //这里会重复连接信号和槽，先全部取消
    QDBusConnection dbus = QDBusConnection::sessionBus();
    dbus.systemBus().disconnect("com.deepin.daemon.Gesture",
                                "/com/deepin/daemon/Gesture", "com.deepin.daemon.Gesture",
                                "Event",
                                wrapper->textEditor(), SLOT(fingerZoom(QString, QString, int)));
    wrapper->textEditor()->disconnect();
    connect(wrapper->textEditor(), &TextEdit::clickFindAction, this, &Window::popupFindBar, Qt::QueuedConnection);
    connect(wrapper->textEditor(), &TextEdit::clickReplaceAction, this, &Window::popupReplaceBar, Qt::QueuedConnection);
    connect(wrapper->textEditor(), &TextEdit::clickJumpLineAction, this, &Window::popupJumpLineBar, Qt::QueuedConnection);
    connect(wrapper->textEditor(), &TextEdit::clickFullscreenAction, this, &Window::toggleFullscreen, Qt::QueuedConnection);
    connect(wrapper->textEditor(), &TextEdit::popupNotify, this, &Window::showNotify, Qt::QueuedConnection);
    connect(wrapper->textEditor(), &TextEdit::signal_setTitleFocus, this, &Window::slot_setTitleFocus, Qt::QueuedConnection);

    dbus.systemBus().connect("com.deepin.daemon.Gesture",
                                "/com/deepin/daemon/Gesture", "com.deepin.daemon.Gesture",
                                "Event",
                                wrapper->textEditor(), SLOT(fingerZoom(QString, QString, int)));


    // add wrapper to this window.
    m_tabbar->addTabWithIndex(index, filepath, tabName);
    m_wrappers[filepath] = wrapper;
    wrapper->updatePath(filepath,qstrTruePath);

    showNewEditor(wrapper);
    wrapper->OnThemeChangeSlot(m_themePath);
}

bool Window::closeTab()
{
    const QString &filePath = m_tabbar->currentPath();
    EditWrapper *wrapper = m_wrappers.value(filePath);

    if (m_reading_list.contains(currentWrapper()->textEditor())) {
        QProcess::startDetached("dbus-send  --print-reply --dest=com.iflytek.aiassistant /aiassistant/tts com.iflytek.aiassistant.tts.stopTTSDirectly");
    }

    if (!wrapper) return false;

    disconnect(wrapper,nullptr);
    disconnect(wrapper->textEditor(), &TextEdit::textChanged,nullptr,nullptr);

    // this property holds whether the document has been modified by the user
    bool isModified = wrapper->isModified();
    bool isDraftFile = wrapper->isDraftFile();
    bool bIsBackupFile = false;

    if (wrapper->isTemFile()) {
        bIsBackupFile = true;
    }

    if(wrapper->getFileLoading()) isModified = false;

    if (isDraftFile) {
        if(isModified){
            DDialog *dialog = createDialog(tr("Do you want to save as another?"), "");
            int res = dialog->exec();

            //取消或关闭弹窗不做任务操作
            if(res == 0 || res == -1) {
                return false;
            }

            //不保存
            if(res == 1){
               removeWrapper(filePath, true);
               m_tabbar->closeCurrentTab();
               QFile(filePath).remove();
               return true;
            }

            //保存
            if(res == 2){
               if(wrapper->saveDraftFile())
               {
                   removeWrapper(filePath, true);
                   m_tabbar->closeCurrentTab();
                   QFile(filePath).remove();
               }
            }
        } else {
            removeWrapper(filePath, true);
            m_tabbar->closeCurrentTab();
            QFile(filePath).remove();
        }
    }
    // document has been modified or unsaved draft document.
    // need to prompt whether to save.
    else {
        QFileInfo fileInfo (filePath);

        if(isModified){
            DDialog *dialog = createDialog(tr("Do you want to save this file?"), "");
            int res = dialog->exec();

            //取消或关闭弹窗不做任务操作
            if(res == 0 || res == -1) return false;

            //不保存
            if(res == 1){
               removeWrapper(filePath, true);
               m_tabbar->closeCurrentTab();

               //删除备份文件
               if (bIsBackupFile) {
                   QFile(filePath).remove();
               }

               //删除自动备份文件
               if (QFileInfo(m_autoBackupDir).exists()) {
                   fileInfo.setFile(wrapper->textEditor()->getTruePath());
                   QString name = fileInfo.absolutePath().replace("/","_");
                   QDir(m_autoBackupDir).remove(fileInfo.baseName() + "." + name + "." + fileInfo.suffix());
               }

               return true;
            }

            //保存
            if(res == 2){
                if(bIsBackupFile){                    
                  if(wrapper->saveFile())
                  {
                      removeWrapper(filePath, true);
                      m_tabbar->closeCurrentTab();
                      QFile(filePath).remove();
                  }             
                }else {
                   if(wrapper->saveFile())
                   {
                       removeWrapper(filePath, true);
                       m_tabbar->closeCurrentTab();
                   }
                }
            }          
        }else {
            removeWrapper(filePath, true);
            m_tabbar->closeCurrentTab();
        }

        //删除自动备份文件
        if (QFileInfo(m_autoBackupDir).exists()) {
            fileInfo.setFile(wrapper->textEditor()->getTruePath());
            QString name = fileInfo.absolutePath().replace("/","_");
            QDir(m_autoBackupDir).remove(fileInfo.baseName() + "." + name + "." + fileInfo.suffix());
        }
    }

    return true;
}

void Window::restoreTab()
{
    if (m_closeFileHistory.size() > 0) {
        addTab(m_closeFileHistory.takeLast());
    }
}


EditWrapper *Window::createEditor()
{
    EditWrapper *wrapper = new EditWrapper(this);
    connect(wrapper->textEditor(), &TextEdit::signal_readingPath, this, &Window::slot_saveReadingPath, Qt::QueuedConnection);
    connect(wrapper->textEditor(), &TextEdit::signal_setTitleFocus, this, &Window::slot_setTitleFocus, Qt::QueuedConnection);
    connect(wrapper->textEditor(), &TextEdit::clickFindAction, this, &Window::popupFindBar, Qt::QueuedConnection);
    connect(wrapper->textEditor(), &TextEdit::clickReplaceAction, this, &Window::popupReplaceBar, Qt::QueuedConnection);
    connect(wrapper->textEditor(), &TextEdit::clickJumpLineAction, this, &Window::popupJumpLineBar, Qt::QueuedConnection);
    connect(wrapper->textEditor(), &TextEdit::clickFullscreenAction, this, &Window::toggleFullscreen, Qt::QueuedConnection);
    connect(wrapper->textEditor(), &TextEdit::popupNotify, this, &Window::showNotify, Qt::QueuedConnection);
    connect(wrapper->textEditor(), &TextEdit::textChanged, this, [=](){
        updateJumpLineBar(wrapper->textEditor());
    }, Qt::QueuedConnection);


    bool wordWrap = m_settings->settings->option("base.font.wordwrap")->value().toBool();
    wrapper->textEditor()->m_pIsShowCodeFoldArea = m_settings->settings->option("base.font.codeflod")->value().toBool();
    wrapper->OnThemeChangeSlot(m_themePath);
    wrapper->textEditor()->setSettings(m_settings);
    wrapper->textEditor()->setTabSpaceNumber(m_settings->settings->option("advance.editor.tabspacenumber")->value().toInt());
    wrapper->textEditor()->setFontFamily(m_settings->settings->option("base.font.family")->value().toString());
    wrapper->textEditor()->setLineWrapMode(wordWrap);
    setFontSizeWithConfig(wrapper);

    //设置显示空白符　梁卫东
    wrapper->setShowBlankCharacter(m_settings->settings->option("base.font.showblankcharacter")->value().toBool());
    //yanyuhan 设置行号显示
    wrapper->setLineNumberShow(m_settings->settings->option("base.font.showlinenumber")->value().toBool(),true);
    wrapper->textEditor()->setHighLineCurrentLine(m_settings->settings->option("base.font.hightlightcurrentline")->value().toBool());
    wrapper->textEditor()->setBookmarkFlagVisable(m_settings->settings->option("base.font.showbookmark")->value().toBool(), true);
    wrapper->textEditor()->setCodeFlodFlagVisable(m_settings->settings->option("base.font.codeflod")->value().toBool(), true);
    wrapper->textEditor()->updateLeftAreaWidget();

    return wrapper;
}

EditWrapper *Window::currentWrapper()
{
    if (m_wrappers.contains(m_tabbar->currentPath())) {
       return m_wrappers.value(m_tabbar->currentPath());
    } else {
       return nullptr;
    }
}

EditWrapper *Window::wrapper(const QString &filePath)
{
    if (m_wrappers.contains(filePath)) {
       return m_wrappers.value(filePath);     
    } else {
       for (auto wrapper : m_wrappers) {
           QString truePath = wrapper->textEditor()->getTruePath();

           if (truePath == filePath) {
               return m_wrappers.value(wrapper->textEditor()->getFilePath());
           }
       }

       return nullptr;
    }
}

TextEdit *Window::getTextEditor(const QString &filepath)
{
    if (m_wrappers.contains(filepath)) {
       return m_wrappers.value(filepath)->textEditor();
    } else {
       return nullptr;
    }
}

void Window::focusActiveEditor()
{
    if (m_tabbar->count() > 0) {
        if (currentWrapper() == nullptr) {
            return;
        }
        currentWrapper()->textEditor()->setFocus();
    }
}

void Window::removeWrapper(const QString &filePath, bool isDelete)
{
    EditWrapper *wrapper = m_wrappers.value(filePath);

    if (wrapper) {
        m_editorWidget->removeWidget(wrapper);

        m_wrappers.remove(filePath);
        if (isDelete) {
            disconnect(wrapper->textEditor(),nullptr);
            disconnect(wrapper,nullptr);
            wrapper->setQuitFlag();
            wrapper->deleteLater();
        }
    }

    // Exit window after close all tabs.
    if (m_wrappers.isEmpty()) {
        DMainWindow::close();
    }
}

void Window::openFile()
{
    QFileDialog dialog(this);
    dialog.setFileMode(QFileDialog::ExistingFiles);
    dialog.setAcceptMode(QFileDialog::AcceptOpen);

    // read history directory.
    QString historyDirStr = m_settings->settings->option("advance.editor.file_dialog_dir")->value().toString();

    if (historyDirStr.isEmpty()) {
        historyDirStr = QStandardPaths::writableLocation(QStandardPaths::DocumentsLocation);
    }

    QDir historyDir(historyDirStr);

    if (historyDir.exists()) {
        dialog.setDirectory(historyDir);
    } else {
        qDebug() << "historyDir or default path not existed:" << historyDir;
    }

    const int mode = dialog.exec();

    PerformanceMonitor::openFileStart();
    // save the directory string.
    m_settings->settings->option("advance.editor.file_dialog_dir")->setValue(dialog.directoryUrl().toLocalFile());

    if (mode != QDialog::Accepted) {
        return;
    }

    QStringList supportfileNames;
    QStringList otherfiles;
    for (const QString &file : dialog.selectedFiles()) {

        if(Utils::isMimeTypeSupport(file))
        {
            supportfileNames.append(file);
        }else {
            otherfiles.append(file);
        }

        //先添加支持的文件
    }
    foreach (QString var, supportfileNames) {
        addTab(var, true);
    }

    //后添加不支持文件　在最后编辑页面显示
    foreach (QString var, otherfiles) {
        addTab(var, true);
    }
}

bool Window::saveFile()
{
    EditWrapper* wrapperEdit = currentWrapper();

    //大文本加载过程不允许保存
    if(!wrapperEdit || wrapperEdit->getFileLoading()) return false;

    bool isDraftFile = wrapperEdit->isDraftFile();
//    bool isEmpty = wrapperEdit->isPlainTextEmpty();
    QString filePath = wrapperEdit->textEditor()->getTruePath();

    // save blank file.
    if (isDraftFile) {
        return saveAsFile();
    }

    if (filePath.isEmpty()) {
        filePath = wrapperEdit->textEditor()->getFilePath();
    }

    QFileInfo info(filePath);
    //判断文件是否有写的权限
    QFile temporaryBuffer(filePath);
    QFile::Permissions pers = temporaryBuffer.permissions();
    bool isWrite = ((pers & QFile::WriteUser) || (pers & QFile::WriteOwner) || (pers & QFile::WriteOther));

    if(!isWrite){
        DMessageManager::instance()->sendMessage(m_editorWidget->currentWidget(), QIcon(":/images/warning.svg")
                                                 , QString(tr("You do not have permission to save %1")).arg(info.fileName()));
        return false;
    }

    // save normal file.
    QString temPath = "";

    if (wrapperEdit->isTemFile()) {
        temPath = wrapperEdit->textEditor()->getFilePath();
    }

    wrapperEdit->updatePath(temPath,filePath);

    bool success = wrapperEdit->saveFile();

    if(success){
        currentWrapper()->hideWarningNotices();
        showNotify(tr("Saved successfully"));

        //删除备份文件
        if (!temPath.isEmpty()) {
            QFile(temPath).remove();
        }

        //删除自动备份文件
        if (QFileInfo(m_autoBackupDir).exists()) {
            QFileInfo fileInfo(filePath);
            QString name = fileInfo.absolutePath().replace("/","_");
            QDir(m_autoBackupDir).remove(fileInfo.baseName() + "." + name + "." + fileInfo.suffix());
        }

        return true;
    }else {
        DDialog *dialog = createDialog(tr("Do you want to save as another?"), "");

        wrapperEdit->setUpdatesEnabled(false);
        int mode =  dialog->exec();
        wrapperEdit->setUpdatesEnabled(true);
        wrapperEdit->hideWarningNotices();
        dialog->deleteLater();
        dialog = nullptr;

        if(mode == 2){
            return  saveAsFile();
        }

        return false;
    }
}

bool Window::saveAsFile()
{
    return !saveAsFileToDisk().isEmpty();
}

QString Window::saveAsFileToDisk()
{
    EditWrapper *wrapper = currentWrapper();
    //大文本加载过程不允许保存　梁卫东
    if (!wrapper || wrapper->getFileLoading()) return QString();

    bool isDraft = wrapper->isDraftFile();
    QFileInfo fileInfo(wrapper->textEditor()->getTruePath());

    DFileDialog dialog(this, tr("Save File"));
    dialog.setAcceptMode(QFileDialog::AcceptSave);
    dialog.addComboBox(QObject::tr("Encoding"),  QStringList() << wrapper->getTextEncode());
    dialog.setDirectory(QDir::homePath());

    if (isDraft) {
        QRegularExpression reg("[^*](.+)");
        QRegularExpressionMatch match = reg.match(m_tabbar->currentName());
        dialog.selectFile(match.captured(0) + ".txt");
    } else {
        dialog.setDirectory(fileInfo.dir());
        dialog.selectFile(fileInfo.fileName());
    }

    wrapper->setUpdatesEnabled(false);
    int mode = dialog.exec();
    wrapper->setUpdatesEnabled(true);


    if (mode == QDialog::Accepted) {
        const QByteArray encode = dialog.getComboBoxValue(QObject::tr("Encoding")).toUtf8();
        const QString endOfLine = dialog.getComboBoxValue(QObject::tr("Line Endings"));
        const QString newFilePath = dialog.selectedFiles().value(0);

        wrapper->updatePath(wrapper->filePath(),newFilePath);
        wrapper->saveFile();
        QFile(wrapper->filePath()).remove();

        //删除自动备份文件
        if (QFileInfo(m_autoBackupDir).exists()) {
            QString truePath = wrapper->textEditor()->getTruePath();
            QFileInfo fileInfo(truePath);
            QString name = fileInfo.absolutePath().replace("/","_");
            QDir(m_autoBackupDir).remove(fileInfo.baseName() + "." + name + "." + fileInfo.suffix());
        }

        updateSaveAsFileName(wrapper->filePath(),newFilePath);
        return newFilePath;
    }

    return QString();
}

QString Window::saveBlankFileToDisk()
{
    QString filePath = m_tabbar->currentPath();
    EditWrapper *wrapper = m_wrappers.value(filePath);
    bool isDraft = Utils::isDraftFile(filePath);
    QFileInfo fileInfo(filePath);

    if (!wrapper)
        //return false;
        return QString();

    DFileDialog dialog(this, tr("Save File"));
    dialog.setAcceptMode(QFileDialog::AcceptSave);
    dialog.addComboBox(tr("Encoding"), QStringList() << wrapper->getTextEncode());
    dialog.setDirectory(QDir::homePath());

    if (isDraft) {
        QRegularExpression reg("[^*](.+)");
        QRegularExpressionMatch match = reg.match(m_tabbar->currentName());
        dialog.selectFile(match.captured(0) + ".txt");
    } else {
        dialog.setDirectory(fileInfo.dir());
        dialog.selectFile(fileInfo.fileName());
    }

    int mode = dialog.exec();
    if (mode == QDialog::Accepted) {   
        const QString newFilePath = dialog.selectedFiles().value(0);
        wrapper->updatePath(newFilePath);
        wrapper->saveFile();

        m_wrappers.remove(filePath);
        m_wrappers.insert(newFilePath, wrapper);

       // wrapper->textEditor()->loadHighlighter();
        return newFilePath;
    }

    return QString();
}

bool Window::saveAsOtherTabFile(EditWrapper *wrapper)
{
    QString filePath = wrapper->textEditor()->getFilePath();
    bool isDraft = Utils::isDraftFile(filePath);
    QFileInfo fileInfo(filePath);
    int index = m_tabbar->indexOf(filePath);
    QString strTabText = m_tabbar->tabText(index);

    if (!wrapper)
        return false;

    DFileDialog dialog(this, tr("Save File"));
    dialog.setAcceptMode(QFileDialog::AcceptSave);
    dialog.addComboBox(QObject::tr("Encoding"), Utils::getEncodeList());
    dialog.addComboBox(QObject::tr("Line Endings"), QStringList() << "Linux" << "Windows" << "Mac OS");
    dialog.setDirectory(QDir::homePath());

    if (isDraft) {
        QRegularExpression reg("[^*](.+)");
        QRegularExpressionMatch match = reg.match(strTabText);
        dialog.selectFile(match.captured(0) + ".txt");
    } else {
        dialog.setDirectory(fileInfo.dir());
        dialog.selectFile(fileInfo.fileName());
    }

    int mode = dialog.exec();
    if (mode == QDialog::Accepted) {
        const QByteArray encode = dialog.getComboBoxValue(QObject::tr("Encoding")).toUtf8();
        const QString endOfLine = dialog.getComboBoxValue(QObject::tr("Line Endings"));
        const QString newFilePath = dialog.selectedFiles().value(0);
        const QFileInfo newFileInfo(newFilePath);
//        EditWrapper::EndOfLineMode eol = EditWrapper::eolUnix;

//        if (endOfLine == "Windows") {
//            eol = EditWrapper::eolDos;
//        } else if (endOfLine == "Mac OS") {
//            eol = EditWrapper::eolMac;
//        }

        if (isDraft) {
            QFile(filePath).remove();
        }

        //m_tabbar->updateTab(m_tabbar->currentIndex(), newFilePath, newFileInfo.fileName());

      //  wrapper->setTextCodec(encode);
        wrapper->updatePath(newFilePath);
        //wrapper->setEndOfLineMode(eol);
        wrapper->saveFile();

       // wrapper->textEditor()->loadHighlighter();
    } else {
        return false;
    }

    return true;
}

void Window::changeSettingDialogComboxFontNumber(int fontNumber)
{
    m_settings->settings->option("base.font.size")->setValue(fontNumber);
}

void Window::decrementFontSize()
{
    int size = std::max(m_fontSize - 1, m_settings->m_iMinFontSize);
    m_settings->settings->option("base.font.size")->setValue(size);
}

void Window::incrementFontSize()
{
    int size = std::min(m_fontSize + 1, m_settings->m_iMaxFontSize);
    m_settings->settings->option("base.font.size")->setValue(size);
}

void Window::resetFontSize()
{
    m_settings->settings->option("base.font.size")->setValue(m_settings->m_iDefaultFontSize);
}

void Window::setFontSizeWithConfig(EditWrapper *wrapper)
{
    int size = m_settings->settings->option("base.font.size")->value().toInt();
    wrapper->textEditor()->setFontSize(size);

    m_fontSize = size;
}


void Window::popupFindBar()
{
//    if (m_findBar->isVisible()) {
//        m_findBar->move(QPoint(10, height() - 59));
//        if (m_findBar->isFocus()) {
//            m_wrappers.value(m_tabbar->currentPath())->textEditor()->setFocus();
//        } else {
//            m_findBar->focus();
//        }
//    } else {
        //addBottomWidget(m_findBar);
        m_findBar->setSearched(false);
        QString tabPath = m_tabbar->currentPath();
        EditWrapper *wrapper = currentWrapper();


        if (currentWrapper() == nullptr) {
            return;
        }

        if (wrapper->textEditor()->document()->isEmpty()) {
            return;
        }

        currentWrapper()->bottomBar()->updateSize(59);

        if (m_replaceBar->isVisible()) {
            m_replaceBar->hide();
        }
        m_findBar->raise();
        m_findBar->move(QPoint(10, height() - 59));
        m_findBar->show();

        QString text = wrapper->textEditor()->textCursor().selectedText();
        int row = wrapper->textEditor()->getCurrentLine();
        int column = wrapper->textEditor()->getCurrentColumn();
        int scrollOffset = wrapper->textEditor()->getScrollOffset();

        m_findBar->activeInput(text, tabPath, row, column, scrollOffset);

        QTimer::singleShot(10, this, [ = ] { m_findBar->focus(); });
   // }
}

void Window::popupReplaceBar()
{
    if (currentWrapper() == nullptr) {
        return;
    }

    if (currentWrapper()->textEditor()->document()->isEmpty()) {
        return;
    }

    QTextCursor cursor = currentWrapper()->textEditor()->textCursor();

    m_replaceBar->setsearched(false);
    EditWrapper *curWrapper = currentWrapper();
    bool bIsReadOnly = curWrapper->textEditor()->getReadOnlyMode();

    if(cursor.hasSelection())
    {
        currentWrapper()->textEditor()->setCursorStart(cursor.selectionStart());
    }


    if (bIsReadOnly) {
        showNotify(tr("Read-Only mode is on"));
        return;
    }

//    if (curWrapper->textEditor()->toPlainText().isEmpty()) {
//        return;
//    }

	currentWrapper()->bottomBar()->updateSize(59);

    EditWrapper *wrapper = currentWrapper();
    if (m_findBar->isVisible()) {
        m_findBar->hide();
    }
    m_replaceBar->raise();
    m_replaceBar->move(QPoint(10, height() - 59));
    m_replaceBar->show();
    //addBottomWidget(m_replaceBar);

    QString tabPath = m_tabbar->currentPath();
    QString text = wrapper->textEditor()->textCursor().selectedText();
    int row = wrapper->textEditor()->getCurrentLine();
    int column = wrapper->textEditor()->getCurrentColumn();
    int scrollOffset = wrapper->textEditor()->getScrollOffset();

    m_replaceBar->activeInput(text, tabPath, row, column, scrollOffset);

    QTimer::singleShot(10, this, [ = ] { m_replaceBar->focus(); });
}

void Window::popupJumpLineBar()
{
    EditWrapper *curWrapper = currentWrapper();

    if(curWrapper == nullptr) {
        return;
    }

    if (curWrapper->textEditor()->document()->isEmpty()) {
        return;
    }

    if(m_jumpLineBar->isVisible())
    {
        m_jumpLineBar->hide();
        return;
    }

    if (m_jumpLineBar->isVisible()) {
        if (m_jumpLineBar->isFocus()) {
            //QTimer::singleShot(0, m_wrappers.value(m_tabbar->currentPath())->textEditor(), SLOT(setFocus()));
        } else {
            m_jumpLineBar->focus();
        }
    } else {
        QString tabPath = m_tabbar->currentPath();
        EditWrapper *wrapper = currentWrapper();
        QString text = wrapper->textEditor()->textCursor().selectedText();
        int row = wrapper->textEditor()->getCurrentLine();
        int column = wrapper->textEditor()->getCurrentColumn();
        int count = wrapper->textEditor()->blockCount();
        int scrollOffset = wrapper->textEditor()->getScrollOffset();

        m_jumpLineBar->activeInput(tabPath, row, column, count, scrollOffset);
        m_jumpLineBar->show();
        m_jumpLineBar->focus();
    }
}

void Window::updateJumpLineBar(TextEdit* editor)
{
    if(m_jumpLineBar->isVisible())
    {
        QString tabPath = m_tabbar->currentPath();
        QString text = editor->textCursor().selectedText();
        int row = editor->getCurrentLine();
        int column = editor->getCurrentColumn();
        int count = editor->blockCount();
        int scrollOffset = editor->getScrollOffset();
        m_jumpLineBar->activeInput(tabPath, row, column, count, scrollOffset);
    }
    if(editor && !editor->ifHasHighlight())
    {
        m_findBar->setSearched(false);
        m_replaceBar->setsearched(false);
    }
}

void Window::popupSettingsDialog()
{
    DSettingsDialog *dialog = new DSettingsDialog(this);
    dialog->widgetFactory()->registerWidget("fontcombobox", Settings::createFontComBoBoxHandle);
    dialog->widgetFactory()->registerWidget("keySequenceEdit",Settings::createKeySequenceEditHandle);

    m_settings->setSettingDialog(dialog);

    dialog->updateSettings(m_settings->settings);

    dialog->exec();

    delete dialog;
    m_settings->settings->sync();
}

void Window::popupPrintDialog()
{
    //大文本加载过程不允许打印操作
    if(currentWrapper() && currentWrapper()->getFileLoading()) return;

    const QString &filePath = currentWrapper()->textEditor()->getFilePath();
    const QString &fileDir = QFileInfo(filePath).dir().absolutePath();

    //适配打印接口2.0，dtk大于 5.4.3 版才合入最新的2.0打印控件接口
#if (DTK_VERSION_MAJOR > 5 \
    || (DTK_VERSION_MAJOR >=5 && DTK_VERSION_MINOR > 4) \
    || (DTK_VERSION_MAJOR >= 5 && DTK_VERSION_MINOR >= 4 && DTK_VERSION_PATCH > 3))

	DPrinter printer(QPrinter::HighResolution);
    DPrintPreviewDialog preview( this);

    if (fileDir == m_blankFileDir) {
        preview.setDocName(QString("%1/%2.pdf").arg(QDir::homePath(), m_tabbar->currentName()));
    } else {
        preview.setDocName(QString("%1/%2.pdf").arg(fileDir, QFileInfo(filePath).baseName()));
    }

    connect(&preview, &DPrintPreviewDialog::paintRequested, this, [ = ](DPrinter * printer) {
        currentWrapper()->textEditor()->print(printer);
    });
#else
	QPrinter printer(QPrinter::HighResolution);
    QPrintPreviewDialog preview( this);

    if (fileDir == m_blankFileDir) {
            printer.setOutputFileName(QString("%1/%2.pdf").arg(QDir::homePath(), m_tabbar->currentName()));
            printer.setDocName(QString("%1/%2.pdf").arg(QDir::homePath(), m_tabbar->currentName()));
    } else {
        printer.setOutputFileName(QString("%1/%2.pdf").arg(fileDir, QFileInfo(filePath).baseName()));
        printer.setDocName(QString("%1/%2.pdf").arg(fileDir, QFileInfo(filePath).baseName()));
    }

    //printer.setOutputFormat(QPrinter::PdfFormat);

    connect(&preview, &QPrintPreviewDialog::paintRequested, this, [ = ](QPrinter * printer) {
        currentWrapper()->textEditor()->print(printer);
    });
#endif

    preview.exec();
}

void Window::popupThemePanel()
{
    updateThemePanelGeomerty();
    m_themePanel->setSelectionTheme(m_themePath);
    m_themePanel->popup();
}

void Window::toggleFullscreen()
{
    if (isFullScreen()) {
        showNormal();
    }  else {
        showFullScreen();
    }
}

void Window::remberPositionSave()
{
    EditWrapper *wrapper = currentWrapper();

    m_remberPositionFilePath = m_tabbar->currentPath();
    m_remberPositionRow = wrapper->textEditor()->getCurrentLine();
    m_remberPositionColumn = wrapper->textEditor()->getCurrentColumn();
    m_remberPositionScrollOffset = wrapper->textEditor()->getScrollOffset();

    currentWrapper()->showNotify(tr("Current location remembered"));
}

void Window::remberPositionRestore()
{
    if (m_remberPositionFilePath.isEmpty()) {
        return;
    }

    if (m_wrappers.contains(m_remberPositionFilePath)) {
        const QString &filePath = m_remberPositionFilePath;
        const int &scrollOffset = m_remberPositionScrollOffset;
        const int &row = m_remberPositionRow;
        const int &column = m_remberPositionColumn;

        activeTab(m_tabbar->indexOf(m_remberPositionFilePath));
        m_wrappers.value(filePath)->textEditor()->scrollToLine(scrollOffset, row, column);
    }
}


void Window::displayShortcuts()
{
    QRect rect = window()->geometry();
    QPoint pos(rect.x() + rect.width() / 2,
               rect.y() + rect.height() / 2);

    //窗体快捷键
    QStringList windowKeymaps;
    windowKeymaps << "addblanktab" << "newwindow" << "savefile"
                  << "saveasfile" << "selectnexttab" << "selectprevtab"
                  << "closetab" << "closeothertabs" << "restoretab"
                  << "openfile" << "incrementfontsize" << "decrementfontsize"
                  << "resetfontsize" << "togglefullscreen" << "find" << "replace"
                  << "jumptoline" << "saveposition" << "restoreposition"
                  << "escape" << "print";

    QJsonObject shortcutObj;
    QJsonArray jsonGroups;

    QJsonObject windowJsonGroup;
    windowJsonGroup.insert("groupName", QObject::tr("Window"));
    QJsonArray windowJsonItems;

    for (const QString &keymap : windowKeymaps) {
        auto option = m_settings->settings->group("shortcuts.window")->option(QString("shortcuts.window.%1").arg(keymap));
        QJsonObject jsonItem;
        jsonItem.insert("name", QObject::tr(option->name().toUtf8().data()));
        if (keymap != "incrementfontsize" && keymap != "decrementfontsize") {
            jsonItem.insert("value", option->value().toString().replace("Meta", "Super"));
        } else if (keymap == "incrementfontsize") {
            QString strIncrementfontValue = QString(tr("Ctrl+'='"));
            jsonItem.insert("value", strIncrementfontValue.replace("Meta", "Super"));
        } else if (keymap == "decrementfontsize" && option->value().toString() == "Ctrl+-") {
            QString strDecrementfontValue = QString(tr("Ctrl+'-'"));
            jsonItem.insert("value", strDecrementfontValue.replace("Meta", "Super"));
        }

        windowJsonItems.append(jsonItem);
    }

    windowJsonGroup.insert("groupItems", windowJsonItems);
    jsonGroups.append(windowJsonGroup);

    //编辑快捷键
    QStringList editorKeymaps;
    editorKeymaps << "indentline" << "backindentline" << "forwardchar"
                  << "backwardchar" << "forwardword" << "backwardword"
                  << "nextline" << "prevline" << "newline" << "opennewlineabove"
                  << "opennewlinebelow" << "duplicateline" << "killline"
                  << "killcurrentline" << "swaplineup" << "swaplinedown"
                  << "scrolllineup" << "scrolllinedown" << "scrollup"
                  << "scrolldown" << "movetoendofline" << "movetostartofline"
                  << "movetoend" << "movetostart" << "movetolineindentation"
                  << "upcaseword" << "downcaseword" << "capitalizeword"
                  << "killbackwardword" << "killforwardword" << "forwardpair"
                  << "backwardpair" << "selectall" << "copy" << "cut"
                  << "paste" << "transposechar" << "setmark" << "exchangemark"
                  << "copylines" << "cutlines" << "joinlines" << "togglereadonlymode"
                  << "togglecomment" << "removecomment" << "undo" << "redo" << "switchbookmark" << "movetoprebookmark"
                  << "movetonextbookmark" << "mark";

    QJsonObject editorJsonGroup;
    editorJsonGroup.insert("groupName", tr("Editor"));
    QJsonArray editorJsonItems;

    for (const QString &keymap : editorKeymaps) {
        auto option = m_settings->settings->group("shortcuts.editor")->option(QString("shortcuts.editor.%1").arg(keymap));
        QJsonObject jsonItem;
        jsonItem.insert("name", QObject::tr(option->name().toUtf8().data()));
        jsonItem.insert("value", option->value().toString().replace("Meta", "Super"));
        editorJsonItems.append(jsonItem);
    }
    editorJsonGroup.insert("groupItems", editorJsonItems);
    jsonGroups.append(editorJsonGroup);

    //设置快捷键
    QStringList setupKeymaps;
    setupKeymaps << "help" << "displayshortcuts";

    QJsonObject setupJsonGroup;
    setupJsonGroup.insert("groupName", tr("Settings"));
    QJsonArray setupJsonItems;

    for (const QString &keymap : setupKeymaps) {
        auto option = m_settings->settings->group("shortcuts.window")->option(QString("shortcuts.window.%1").arg(keymap));
        QJsonObject jsonItem;
        jsonItem.insert("name", QObject::tr(option->name().toUtf8().data()));
        jsonItem.insert("value", option->value().toString().replace("Meta", "Super"));
        setupJsonItems.append(jsonItem);
    }
    setupJsonGroup.insert("groupItems", setupJsonItems);
    jsonGroups.append(setupJsonGroup);

    shortcutObj.insert("shortcut", jsonGroups);

    QJsonDocument doc(shortcutObj);

    QStringList shortcutString;
    QString param1 = "-j=" + QString(doc.toJson().data());
    QString param2 = "-p=" + QString::number(pos.x()) + "," + QString::number(pos.y());
    shortcutString << param1 << param2;

    QProcess *shortcutViewProcess = new QProcess();
    shortcutViewProcess->startDetached("deepin-shortcut-viewer", shortcutString);

    connect(shortcutViewProcess, SIGNAL(finished(int)), shortcutViewProcess, SLOT(deleteLater()));
}

void Window::backupFile()
{
    if (!QFileInfo(m_backupDir).exists()) {
        QDir().mkpath(m_backupDir);
    }

    QMap<QString, EditWrapper *> wrappers = m_wrappers;
    QStringList listBackupInfo;
    QString filePath,localPath,curPos;
    QFileInfo fileInfo;
    m_qlistTemFile.clear();
    m_qlistTemFile = wrappers.keys();

    for (EditWrapper *wrapper : wrappers) {
        if(wrapper->getFileLoading()) continue;

        filePath = wrapper->textEditor()->getFilePath();
        localPath = wrapper->textEditor()->getTruePath();

        if (localPath.isEmpty()) {
            localPath = wrapper->textEditor()->getFilePath();
        }

        StartManager::FileTabInfo tabInfo = StartManager::instance()->getFileTabInfo(filePath);
        curPos = QString::number(wrapper->textEditor()->textCursor().position());              
        fileInfo.setFile(localPath);

        QJsonObject jsonObject;
        QJsonDocument document;
        jsonObject.insert("localPath",localPath);
        jsonObject.insert("cursorPosition",QString::number(wrapper->textEditor()->textCursor().position()));
        jsonObject.insert("modify",wrapper->isModified());      
        QList<int> bookmarkList= wrapper->textEditor()->getBookmarkInfo();
        if (!bookmarkList.isEmpty()) {
            QString bookmarkInfo;

            //记录书签
            for (int i = 0;i < bookmarkList.count();i++) {
                if (i == bookmarkList.count() - 1) {
                    bookmarkInfo.append(QString::number(bookmarkList.value(i)));
                } else {
                    bookmarkInfo.append(QString::number(bookmarkList.value(i)) + ",");
                }
            }

            jsonObject.insert("bookMark",bookmarkInfo);
        }

        //记录活动页
        if (filePath == m_tabbar->currentPath()) {
            jsonObject.insert("focus",true);
        }

        //保存备份文件
        if (Utils::isDraftFile(filePath)) {
            wrapper->saveTemFile(filePath);
        } else {
            if (wrapper->isModified()) {
                QString name = fileInfo.absolutePath().replace("/","_");
                QString qstrFilePath = m_backupDir + "/" + fileInfo.baseName() + "." + name + "." + fileInfo.suffix();
                jsonObject.insert("temFilePath",qstrFilePath);
                wrapper->saveTemFile(qstrFilePath);
            }
        }

        //使用json串形式保存
        document.setObject(jsonObject);
        QByteArray byteArray = document.toJson(QJsonDocument::Compact);
        m_qlistTemFile.replace(tabInfo.tabIndex,byteArray);
    }

    //将json串列表写入配置文件
    m_settings->settings->option("advance.editor.browsing_history_temfile")->setValue(m_qlistTemFile);

    //删除自动备份文件
    if (QFileInfo(m_autoBackupDir).exists()) {
        QDir(m_autoBackupDir).removeRecursively();
    }
}

bool Window::closeAllFiles()
{
    bool bIsCloseAll = true;
    QMap<QString, EditWrapper *> wrappers = m_wrappers;

    //关闭所有文件
    for (int i = 0;i < wrappers.count();i++) {
        m_tabbar->setCurrentIndex(0);

        if (!closeTab()) {
            bIsCloseAll = false;
        }
    }

    return bIsCloseAll;
}

void Window::addTemFileTab(QString qstrPath,QString qstrName,QString qstrTruePath,bool bIsTemFile)
{      
    if (qstrPath.isEmpty() || !Utils::fileExists(qstrPath)) {
        return;
    }

    EditWrapper *wrapper = createEditor();
    m_tabbar->addTab(qstrPath, qstrName);   
    wrapper->openFile(qstrPath,qstrTruePath,bIsTemFile);
    m_wrappers[qstrPath] = wrapper;
    showNewEditor(wrapper);
}

QMap<QString, EditWrapper *> Window::getWrappers()
{
    return m_wrappers;
}

void Window::setChildrenFocus(bool ok)
{
    QMap<QString, EditWrapper *>::Iterator it = m_wrappers.begin();
    for(;it != m_wrappers.end();it++){
        it.value()->bottomBar()->setChildrenFocus(ok);
    }

    if(ok){
        DIconButton *addButton = m_tabbar->findChild<DIconButton *>("AddButton");
        DIconButton *optionBtn = titlebar()->findChild<DIconButton *>("DTitlebarDWindowOptionButton");
        DIconButton *minBtn = titlebar()->findChild<DIconButton *>("DTitlebarDWindowMinButton");
        DIconButton *quitFullBtn = titlebar()->findChild<DIconButton *>("DTitlebarDWindowQuitFullscreenButton");
        DIconButton *maxBtn = titlebar()->findChild<DIconButton *>("DTitlebarDWindowMaxButton");
        DIconButton *closeBtn = titlebar()->findChild<DIconButton *>("DTitlebarDWindowCloseButton");

        titlebar()->setFocusPolicy(Qt::TabFocus);
        if(addButton) addButton->setFocusPolicy(Qt::TabFocus);
        if(optionBtn) optionBtn->setFocusPolicy(Qt::TabFocus);
        if(minBtn) minBtn->setFocusPolicy(Qt::TabFocus);
        if(quitFullBtn) quitFullBtn->setFocusPolicy(Qt::TabFocus);
        if(maxBtn) maxBtn->setFocusPolicy(Qt::TabFocus);
        if(closeBtn) closeBtn->setFocusPolicy(Qt::TabFocus);

        setTabOrder(addButton, optionBtn);
        setTabOrder(optionBtn, minBtn);
        setTabOrder(minBtn, quitFullBtn);
        setTabOrder(quitFullBtn, maxBtn);
        setTabOrder(maxBtn, closeBtn);
    }else {
        DIconButton *addButton = m_tabbar->findChild<DIconButton *>("AddButton");
        DIconButton *optionBtn = titlebar()->findChild<DIconButton *>("DTitlebarDWindowOptionButton");
        DIconButton *minBtn = titlebar()->findChild<DIconButton *>("DTitlebarDWindowMinButton");
        DIconButton *quitFullBtn = titlebar()->findChild<DIconButton *>("DTitlebarDWindowQuitFullscreenButton");
        DIconButton *maxBtn = titlebar()->findChild<DIconButton *>("DTitlebarDWindowMaxButton");
        DIconButton *closeBtn = titlebar()->findChild<DIconButton *>("DTitlebarDWindowCloseButton");

        titlebar()->setFocusPolicy(Qt::NoFocus);
        if(addButton) addButton->setFocusPolicy(Qt::NoFocus);
        if(optionBtn) optionBtn->setFocusPolicy(Qt::NoFocus);
        if(minBtn) minBtn->setFocusPolicy(Qt::NoFocus);
        if(quitFullBtn) quitFullBtn->setFocusPolicy(Qt::NoFocus);
        if(maxBtn) maxBtn->setFocusPolicy(Qt::NoFocus);
        if(closeBtn) closeBtn->setFocusPolicy(Qt::NoFocus);
    }
}

void Window::addBlankTab()
{
    addBlankTab("");
}

void Window::addBlankTab(const QString &blankFile)
{
    QString blankTabPath;

    if (blankFile.isEmpty()) {
        const QString &fileName = QString("blank_file_%1").arg(QDateTime::currentDateTime().toString("yyyy-MM-dd_hh-mm-ss-zzz"));
        blankTabPath = QDir(m_blankFileDir).filePath(fileName);

        if (!Utils::fileExists(blankTabPath)) {
            QDir().mkpath(m_blankFileDir);
        }
    } else {
        blankTabPath = blankFile;
    }

    int blankFileIndex = getBlankFileIndex();
    m_tabbar->addTab(blankTabPath, tr("Untitled %1").arg(blankFileIndex));
    EditWrapper *wrapper = createEditor();
    wrapper->updatePath(blankTabPath);

    if (!blankFile.isEmpty() && Utils::fileExists(blankFile)) {
        wrapper->openFile(blankTabPath,blankTabPath);
    }

    m_wrappers[blankTabPath] = wrapper;
    showNewEditor(wrapper);
}

void Window::handleTabCloseRequested(int index)
{
    activeTab(index);
    closeTab();
}

void Window::handleTabsClosed(QStringList tabList)
{
    if (tabList.isEmpty()) {
        return;
    }

    for (const QString &path : tabList) {
        int index = m_tabbar->indexOf(path);
        m_tabbar->setCurrentIndex(index);
        closeTab();
    }
}

void Window::handleCurrentChanged(const int &index)
{
    if (m_findBar->isVisible()) {
        m_findBar->hide();
    }

    if (m_replaceBar->isVisible()) {
        m_replaceBar->hide();
    }

    if (m_jumpLineBar->isVisible()) {
        m_jumpLineBar->hide();
    }

    for (auto wrapper : m_wrappers.values()) {
         wrapper->textEditor()->removeKeywords();
    }

    if(currentWrapper())
    {
        currentWrapper()->checkForReload();
        checkTabbarForReload();
    }

    const QString &filepath = m_tabbar->fileAt(index);
    bool bIsContains = false;

    if (m_wrappers.contains(filepath)) {
        EditWrapper *wrapper = m_wrappers.value(filepath);
        wrapper->textEditor()->setFocus();

        for (int i = 0;i < m_editorWidget->count();i++) {
            if (m_editorWidget->widget(i) == wrapper) {
                bIsContains = true;
            }
        }

        if (!bIsContains) {
            m_editorWidget->addWidget(wrapper);
        }

        m_editorWidget->setCurrentWidget(wrapper);
    }

    if (currentWrapper() != nullptr) {
        currentWrapper()->bottomBar()->show();
        currentWrapper()->bottomBar()->updateSize(32);
    }
}

void Window::handleJumpLineBarExit()
{
    if (currentWrapper() != nullptr) {
        QTimer::singleShot(0, currentWrapper()->textEditor(), SLOT(setFocus()));
    }
}

void Window::handleJumpLineBarJumpToLine(const QString &filepath, int line, bool focusEditor)
{
    if (m_wrappers.contains(filepath)) {
        getTextEditor(filepath)->jumpToLine(line, true);

        if (focusEditor) {
            QTimer::singleShot(0, getTextEditor(filepath), SLOT(setFocus()));
        }
    }
}

void Window::handleBackToPosition(const QString &file, int row, int column, int scrollOffset)
{
    if (m_wrappers.contains(file)) {
        m_wrappers.value(file)->textEditor()->scrollToLine(scrollOffset, row, column);
    }
}

void Window::handleFindNext()
{
    EditWrapper *wrapper = currentWrapper();
    wrapper->textEditor()->saveMarkStatus();
    wrapper->textEditor()->updateCursorKeywordSelection(wrapper->textEditor()->getPosition(), true);
    wrapper->textEditor()->renderAllSelections();
    wrapper->textEditor()->restoreMarkStatus();
    wrapper->textEditor()->updateLeftAreaWidget();
}

void Window::handleFindPrev()
{
    EditWrapper *wrapper = currentWrapper();
    wrapper->textEditor()->saveMarkStatus();
    wrapper->textEditor()->updateCursorKeywordSelection(wrapper->textEditor()->getPosition(), false);
    wrapper->textEditor()->renderAllSelections();
    wrapper->textEditor()->restoreMarkStatus();
    wrapper->textEditor()->updateLeftAreaWidget();
}

void Window::slotFindbarClose()
{
    EditWrapper *wrapper = currentWrapper();

    if (wrapper->bottomBar()->isHidden()) {
        wrapper->bottomBar()->show();
    }

    wrapper->bottomBar()->updateSize(32);
    currentWrapper()->textEditor()->setFocus();
    currentWrapper()->textEditor()->tellFindBarClose();
}

void Window::slotReplacebarClose()
{
    EditWrapper *wrapper = currentWrapper();

    if (wrapper->bottomBar()->isHidden()) {
        wrapper->bottomBar()->show();
    }

    wrapper->bottomBar()->updateSize(32);
    currentWrapper()->textEditor()->setFocus();
    currentWrapper()->textEditor()->tellFindBarClose();
}

void Window::handleReplaceAll(const QString &replaceText, const QString &withText)
{
    EditWrapper *wrapper = currentWrapper();
    wrapper->textEditor()->replaceAll(replaceText, withText);
}

void Window::handleReplaceNext(const QString &replaceText, const QString &withText)
{
    EditWrapper *wrapper = currentWrapper();
    wrapper->textEditor()->replaceNext(replaceText, withText);
}

void Window::handleReplaceRest(const QString &replaceText, const QString &withText)
{
    EditWrapper *wrapper = currentWrapper();
    wrapper->textEditor()->replaceRest(replaceText, withText);
}

void Window::handleReplaceSkip()
{
    EditWrapper *wrapper = currentWrapper();
    wrapper->textEditor()->updateCursorKeywordSelection(wrapper->textEditor()->getPosition(), true);
    wrapper->textEditor()->renderAllSelections();
}

void Window::handleRemoveSearchKeyword()
{
    if (currentWrapper() != nullptr) {
        currentWrapper()->textEditor()->removeKeywords();
    }
}

void Window::handleUpdateSearchKeyword(QWidget *widget, const QString &file, const QString &keyword)
{
    if (file == m_tabbar->currentPath() && m_wrappers.contains(file)) {

        // Update input widget warning status along with keyword match situation.
        bool findKeyword = m_wrappers.value(file)->textEditor()->highlightKeyword(keyword, m_wrappers.value(file)->textEditor()->getPosition());
        //    bool findKeyword = m_wrappers.value(file)->textEditor()->findKeywordForward(keyword);
        bool emptyKeyword = keyword.trimmed().isEmpty();

        auto *findBarWidget = qobject_cast<FindBar *>(widget);
        if (findBarWidget != nullptr) {
            if (emptyKeyword) {
                findBarWidget->setMismatchAlert(false);
            } else {
                findBarWidget->setMismatchAlert(!findKeyword);
            }
        } else {
            auto *replaceBarWidget = qobject_cast<ReplaceBar *>(widget);
            if (replaceBarWidget != nullptr) {
                if (emptyKeyword) {
                    replaceBarWidget->setMismatchAlert(false);
                } else {
                    replaceBarWidget->setMismatchAlert(!findKeyword);
                }
            }
        }
    }
    EditWrapper *wrapper = currentWrapper();
    wrapper->textEditor()->updateLeftAreaWidget();
}

void Window::loadTheme(const QString &path)
{
    QFileInfo fileInfo(path);
    if (!fileInfo.exists()) {
        return;
    }

    m_themePath = path;

    QVariantMap jsonMap = Utils::getThemeMapFromPath(path);
    const QString &backgroundColor = jsonMap["editor-colors"].toMap()["background-color"].toString();
    const QString &tabbarStartColor = jsonMap["app-colors"].toMap()["tab-background-start-color"].toString();
    const QString &tabbarEndColor = jsonMap["app-colors"].toMap()["tab-background-end-color"].toString();


    for (EditWrapper *wrapper : m_wrappers.values()) {
        wrapper->OnThemeChangeSlot(m_themePath);
    }

    m_themePanel->setBackground(backgroundColor);
    m_tabbar->setBackground(tabbarStartColor, tabbarEndColor);
    m_tabbar->setDNDColor(jsonMap["app-colors"].toMap()["tab-dnd-start"].toString(), jsonMap["app-colors"].toMap()["tab-dnd-end"].toString());

    const QString &frameSelectedColor = jsonMap["app-colors"].toMap()["themebar-frame-selected"].toString();
    const QString &frameNormalColor = jsonMap["app-colors"].toMap()["themebar-frame-normal"].toString();
    m_themePanel->setFrameColor(frameSelectedColor, frameNormalColor);
    m_settings->settings->option("advance.editor.theme")->setValue(path);
}

void Window::showNewEditor(EditWrapper *wrapper)
{
    m_editorWidget->addWidget(wrapper);
    m_editorWidget->setCurrentWidget(wrapper);
}

void Window::showNotify(const QString &message)
{
    //DMainWindow::sendMessage(QIcon(":/images/ok.svg"), message);
    //如果是第一次打开编辑器，需要创建一个空白编辑显示框窗体
    if (currentWrapper() == nullptr) {
        this->addBlankTab();
    }
    currentWrapper()->showNotify(message);
}

int Window::getBlankFileIndex()
{
    // get blank tab index list.
    QList<int> tabIndexes;

    // tabFiles.size()
    for (int i = 0; i < m_tabbar->count(); ++i) {
        // find all the blank tab index number.
        QFileInfo fileInfo(m_tabbar->fileAt(i));
        if (fileInfo.dir().absolutePath() == m_blankFileDir) {
            const QString tabText = m_tabbar->textAt(i);
            QRegularExpression reg("(\\d+)");
            QRegularExpressionMatch match = reg.match(tabText);

            tabIndexes << match.captured(1).toInt();
        }
    }
    std::sort(tabIndexes.begin(), tabIndexes.end());

    // Return 1 if no blank file exists.
    if (tabIndexes.size() == 0) {
        return 1;
    }

    // Return first mismatch index as new blank file index.
    for (int j = 0; j < tabIndexes.size(); j++) {
        if (tabIndexes[j] != j + 1) {
            return j + 1;
        }
    }

    // Last, return biggest index as blank file index.
    return tabIndexes.size() + 1;
}

DDialog *Window::createDialog(const QString &title, const QString &content)
{
    DDialog *dialog = new DDialog(title, content, this);
    dialog->setWindowFlags(dialog->windowFlags() | Qt::WindowStaysOnTopHint);
    dialog->setIcon(QIcon::fromTheme("deepin-editor"));
    dialog->addButton(QString(tr("Cancel")), false, DDialog::ButtonNormal);
    dialog->addButton(QString(tr("Discard")), false, DDialog::ButtonNormal);
    dialog->addButton(QString(tr("Save")), true, DDialog::ButtonRecommend);

    return dialog;
}

void Window::slotLoadContentTheme(DGuiApplicationHelper::ColorType themeType)
{
    if (themeType == DGuiApplicationHelper::ColorType::LightType) {
        loadTheme("/usr/share/deepin-editor/themes/deepin.theme");
        if (DGuiApplicationHelper::instance()->paletteType() == DGuiApplicationHelper::ColorType::UnknownType) {
            DGuiApplicationHelper::instance()->setPaletteType(DGuiApplicationHelper::ColorType::UnknownType);
        }
    } else if (themeType == DGuiApplicationHelper::ColorType::DarkType) {
        loadTheme("/usr/share/deepin-editor/themes/deepin_dark.theme");
        if (DGuiApplicationHelper::instance()->paletteType() == DGuiApplicationHelper::ColorType::UnknownType) {
            DGuiApplicationHelper::instance()->setPaletteType(DGuiApplicationHelper::ColorType::UnknownType);
        }
    }

    QString qstrColor = palette().color(QPalette::Active,QPalette::Text).name();

    for (auto wrapper : m_wrappers) {
        wrapper->textEditor()->setEditPalette(qstrColor,qstrColor);
    }

    qstrColor = palette().color(QPalette::Active,QPalette::ButtonText).name();
    QString qstrHighlightColor = palette().color(QPalette::Active,QPalette::HighlightedText).name();

    m_tabbar->setTabPalette(qstrColor,qstrHighlightColor);
}

void Window::slotSettingResetTheme(const QString &path)
{
    QString strLightTheme = "/usr/share/deepin-editor/themes/deepin.theme";
    QString strDarkTheme = "/usr/share/deepin-editor/themes/deepin_dark.theme";

    if (path == strLightTheme) {
        if (DGuiApplicationHelper::instance()->themeType() == DGuiApplicationHelper::LightType) {
            return;
        } else {
            DGuiApplicationHelper::instance()->setPaletteType(DGuiApplicationHelper::LightType);
            //DGuiApplicationHelper::instance()->setThemeType(DGuiApplicationHelper::LightType);
        }
    } else if (path == strDarkTheme) {
        if (DGuiApplicationHelper::instance()->themeType() == DGuiApplicationHelper::DarkType) {
            return;
        } else {
            DGuiApplicationHelper::instance()->setPaletteType(DGuiApplicationHelper::DarkType);
            //DGuiApplicationHelper::instance()->setThemeType(DGuiApplicationHelper::DarkType);
        }
    }
}

void Window::slot_saveReadingPath()
{
    m_reading_list.clear();
    m_reading_list.append(currentWrapper()->textEditor());
}

void Window::slot_beforeReplace(QString _)
{
    currentWrapper()->textEditor()->beforeReplace(_);
}

void Window::slot_setTitleFocus()
{
    QMap<QString, EditWrapper *>::Iterator it = m_wrappers.begin();
    for(;it != m_wrappers.end();it++){
        it.value()->bottomBar()->setChildrenFocus(true);
    }

    titlebar()->setFocusPolicy(Qt::TabFocus);
    titlebar()->setFocus(Qt::MouseFocusReason);
    DIconButton *addButton = m_tabbar->findChild<DIconButton *>("AddButton");
    addButton->setFocusPolicy(Qt::TabFocus);
    DIconButton *optionBtn = titlebar()->findChild<DIconButton *>("DTitlebarDWindowOptionButton");
    optionBtn->setFocusPolicy(Qt::TabFocus);
    DIconButton *minBtn = titlebar()->findChild<DIconButton *>("DTitlebarDWindowMinButton");
    minBtn->setFocusPolicy(Qt::TabFocus);

    DIconButton *quitFullBtn = titlebar()->findChild<DIconButton *>("DTitlebarDWindowQuitFullscreenButton");
    quitFullBtn->setFocusPolicy(Qt::TabFocus);
    DIconButton *maxBtn = titlebar()->findChild<DIconButton *>("DTitlebarDWindowMaxButton");
    maxBtn->setFocusPolicy(Qt::TabFocus);
    DIconButton *closeBtn = titlebar()->findChild<DIconButton *>("DTitlebarDWindowCloseButton");
    closeBtn->setFocusPolicy(Qt::TabFocus);
    QWidget::setTabOrder(addButton, optionBtn);
    QWidget::setTabOrder(optionBtn, minBtn);
    QWidget::setTabOrder(minBtn, quitFullBtn);
    QWidget::setTabOrder(quitFullBtn, maxBtn);
    QWidget::setTabOrder(maxBtn, closeBtn);
    currentWrapper()->bottomBar()->setChildrenFocus(true,closeBtn);
}

void Window::handleFocusWindowChanged(QWindow *w)
{
    if (windowHandle() != w || !currentWrapper() || !isActiveWindow()) {
        return;
    }

    currentWrapper()->checkForReload();
    checkTabbarForReload();
}

void Window::updateThemePanelGeomerty()
{
    int yOffset = isFullScreen() ? 0 : titlebar()->height();
    QRect themePanelRect(0, yOffset, 250, height() - yOffset);
    themePanelRect.moveRight(rect().right());
    m_themePanel->setGeometry(themePanelRect);
}

void Window::checkTabbarForReload()
{
    QFileInfo fi(m_tabbar->currentPath());
    QString tabName = m_tabbar->currentName();
    QString readOnlyStr = QString(" (%1)").arg(tr("Read-Only"));
    tabName.remove(readOnlyStr);

    EditWrapper *wrapper = m_wrappers.value(m_tabbar->currentPath());
    if (fi.exists() && !fi.isWritable()) {
        tabName.append(readOnlyStr);
        wrapper->textEditor()->setReadOnlyPermission(true);
    } else {
        wrapper->textEditor()->setReadOnlyPermission(false);
    }

    //m_tabbar->setTabText(m_tabbar->currentIndex(), tabName);
    //判断是否需要阻塞系统关机
    emit sigJudgeBlockShutdown();
}

void Window::resizeEvent(QResizeEvent *e)
{
    if (m_themePanel->isVisible()) {
        updateThemePanelGeomerty();
    }

    if (!isMaximized() && !isFullScreen()) {
        m_settings->settings->option("advance.window.window_width")->setValue(rect().width());
        m_settings->settings->option("advance.window.window_height")->setValue(rect().height());
    }

    m_findBar->resize(width() - 20, m_findBar->height());
    m_findBar->move(QPoint(10, height() - 59));
    m_replaceBar->resize(width() - 20, m_replaceBar->height());
    m_replaceBar->move(QPoint(10, height() - 59));

//    if (!(m_tabbar->currentPath() == "")) {
//        EditWrapper *wrapper = m_wrappers.value(m_tabbar->currentPath());
//        wrapper->textEditor()->hideRightMenu();
//    }

    DMainWindow::resizeEvent(e);
}

void Window::closeEvent(QCloseEvent *e)
{
    PerformanceMonitor::closeAppStart();

    if (StartManager::instance()->isMultiWindow()) {
        if (!closeAllFiles()) {
            e->ignore();
            return;
        }
    } else {
        backupFile();
    }

    QProcess::startDetached("dbus-send  --print-reply --dest=com.iflytek.aiassistant /aiassistant/tts com.iflytek.aiassistant.tts.stopTTSDirectly");

    QList<EditWrapper *> needSaveList;
    QMap<QString, EditWrapper *> wrappers = m_wrappers;

    for (EditWrapper *wrapper : wrappers) {
        m_wrappers.remove(wrapper->filePath());
        disconnect(wrapper->textEditor());
        wrapper->setQuitFlag();
        wrapper->deleteLater();
    }

    disconnect(m_settings,nullptr,this,nullptr);
    //this->close();
    emit close();
    return DMainWindow::closeEvent(e);
}

void Window::hideEvent(QHideEvent *event)
{
    if(this->isVisible())
    {
        if (currentWrapper() != nullptr) {
            currentWrapper()->textEditor()->setFocus();
        }
    }
    //如果查找浮窗正显示着，则隐藏
    if (m_findBar->isVisible()) {
       // m_findBar->hide();
        if (currentWrapper() != nullptr) {
            currentWrapper()->bottomBar()->show();
        }
    }

    //如果替换浮窗正显示着，则隐藏
//    if (m_replaceBar->isVisible()) {
//      //  m_replaceBar->hide();
//        if (currentWrapper() != nullptr) {
//            currentWrapper()->m_bottomBar->show();
//        }
//    }
     DMainWindow::hideEvent(event);
}

void Window::keyPressEvent(QKeyEvent *e)
{
    QString key = Utils::getKeyshortcut(e);

    if (key == Utils::getKeyshortcutFromKeymap(m_settings, "window", "decrementfontsize")
            || key == Utils::getKeyshortcutFromKeymap(m_settings, "window", "incrementfontsize")
            || key == Utils::getKeyshortcutFromKeymap(m_settings, "window", "togglefullscreen")) {
        currentWrapper()->textEditor()->setCodeFoldWidgetHide(true);
    }

    if (key == Utils::getKeyshortcutFromKeymap(m_settings, "window", "addblanktab")) {
        addBlankTab();
    } else if (key == Utils::getKeyshortcutFromKeymap(m_settings, "window", "newwindow")) {
        emit newWindow();
    } else if (key == Utils::getKeyshortcutFromKeymap(m_settings, "window", "savefile")) {
        saveFile();
    } else if (key == Utils::getKeyshortcutFromKeymap(m_settings, "window", "saveasfile")) {
        saveAsFile();
    } else if (key == Utils::getKeyshortcutFromKeymap(m_settings, "window", "selectnexttab")) {
        m_tabbar->nextTab();
    } else if (key == Utils::getKeyshortcutFromKeymap(m_settings, "window", "selectprevtab")) {
        m_tabbar->previousTab();
    } else if (key == Utils::getKeyshortcutFromKeymap(m_settings, "window", "closetab")) {
        closeTab();
    } else if (key == Utils::getKeyshortcutFromKeymap(m_settings, "window", "restoretab")) {
        restoreTab();
    } else if (key == Utils::getKeyshortcutFromKeymap(m_settings, "window", "closeothertabs")) {
        m_tabbar->closeOtherTabs();
    } else if (key == Utils::getKeyshortcutFromKeymap(m_settings, "window", "openfile")) {
        openFile();
    } else if (key == Utils::getKeyshortcutFromKeymap(m_settings, "window", "incrementfontsize")) {
        incrementFontSize();
    } else if (key == Utils::getKeyshortcutFromKeymap(m_settings, "window", "decrementfontsize")) {
        decrementFontSize();
    } else if (key == Utils::getKeyshortcutFromKeymap(m_settings, "window", "resetfontsize")) {
        resetFontSize();
    }  else if (key == Utils::getKeyshortcutFromKeymap(m_settings, "window", "togglefullscreen")) {
        DIconButton *minBtn = titlebar()->findChild<DIconButton *>("DTitlebarDWindowMinButton");
        DIconButton *quitFullBtn = titlebar()->findChild<DIconButton *>("DTitlebarDWindowQuitFullscreenButton");
        quitFullBtn->setFocusPolicy(Qt::TabFocus);
        DIconButton *maxBtn = titlebar()->findChild<DIconButton *>("DTitlebarDWindowMaxButton");
        if(minBtn->hasFocus()||maxBtn->hasFocus())
        {
            toggleFullscreen();
            quitFullBtn->setFocus();
        }
        else {
            toggleFullscreen();
        }
    } else if (key == Utils::getKeyshortcutFromKeymap(m_settings, "window", "find")) {
        popupFindBar();
    } else if (key == Utils::getKeyshortcutFromKeymap(m_settings, "window", "replace")) {
        popupReplaceBar();
    } else if (key == Utils::getKeyshortcutFromKeymap(m_settings, "window", "jumptoline")) {
        popupJumpLineBar();
    } else if (key == Utils::getKeyshortcutFromKeymap(m_settings, "window", "saveposition")) {
        remberPositionSave();
    } else if (key == Utils::getKeyshortcutFromKeymap(m_settings, "window", "restoreposition")) {
        remberPositionRestore();
    } else if (key == Utils::getKeyshortcutFromKeymap(m_settings, "window", "escape")) {
       emit pressEsc();
    } else if (key == Utils::getKeyshortcutFromKeymap(m_settings, "window", "displayshortcuts")) {
        displayShortcuts();
    } else if (key == Utils::getKeyshortcutFromKeymap(m_settings, "window", "print")) {
        popupPrintDialog();
    } else {
        // Post event to window widget if match Alt+0 ~ Alt+9
        QRegularExpression re("^Alt\\+\\d");
        QRegularExpressionMatch match = re.match(key);
        if (match.hasMatch()) {
            auto tabIndex = key.replace("Alt+", "").toInt();
            if (tabIndex == 9) {
                if (m_tabbar->count() > 1) {
                    activeTab(m_tabbar->count() - 1);
                }
            } else {
                if (tabIndex <= m_tabbar->count()) {
                    activeTab(tabIndex - 1);
                }
            }
        }
    }
}

void Window::dragEnterEvent(QDragEnterEvent *event)
{
    // Accept drag event if mime type is url.
    event->accept();
}

void Window::dropEvent(QDropEvent *event)
{
    const QMimeData *mimeData = event->mimeData();

    if (mimeData->hasUrls()) {
        QStringList supportfileNames;
        QStringList otherfiles;
        for (auto url : mimeData->urls()) {
            QString file = url.toLocalFile();
            if(Utils::isMimeTypeSupport(file))
            {
                supportfileNames.append(file);
            }else {
                otherfiles.append(file);
            }
        }

        //先添加支持的文件
        foreach (QString var, supportfileNames) {
            addTab(var, true);
        }

        //后添加不支持文件　在最后编辑页面显示
        foreach (QString var, otherfiles) {
            addTab(var, true);
        }
    }
}
