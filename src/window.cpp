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
#include "toolbar.h"
#include "danchors.h"
#include "dthememanager.h"
#include "dtoast.h"
#include "utils.h"

#include <DSettingsWidgetFactory>
#include <DSettingsGroup>
#include <DSettings>
#include <DSettingsOption>
#include <DTitlebar>
#include <DAnchors>

#include <QApplication>
#include <QPrintDialog>
#include <QPrintPreviewDialog>
#include <QPrinter>
#include <QScreen>
#include <QStyleFactory>
#include <QEvent>

#include <QGuiApplication>
#include <QWindow>

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
      m_settings(new Settings(this)),
      m_menu(new QMenu),
      m_titlebarStyleSheet(titlebar()->styleSheet())
{
    m_blankFileDir = QDir(QStandardPaths::standardLocations(QStandardPaths::DataLocation).first()).filePath("blank-files");
    m_themePath = m_settings->settings->option("advance.editor.theme")->value().toString();
    m_rootSaveDBus = new DBusDaemon::dbus("com.deepin.editor.daemon", "/", QDBusConnection::systemBus(), this);

    // Init.
    setAcceptDrops(true);

    // Apply qss theme.
    //Utils::applyQss(this, "main.qss");
    loadTheme(m_themePath);

    // Init settings.
    connect(m_settings, &Settings::adjustFont, this, &Window::updateFont);
    connect(m_settings, &Settings::adjustFontSize, this, &Window::updateFontSize);
    connect(m_settings, &Settings::adjustTabSpaceNumber, this, &Window::updateTabSpaceNumber);
    connect(m_settings, &Settings::themeChanged, this, &Window::slotSettingResetTheme);
    connect(m_settings, &Settings::adjustWordWrap, this, [=] (bool enable) {
        for (EditWrapper *wrapper : m_wrappers.values()) {
            DTextEdit *textedit = wrapper->textEditor();
            textedit->setLineWrapMode(enable);
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

    // Init window state with config.
    // Below code must before this->titlebar()->setMenu, otherwise main menu can't display pre-build-in menu items by dtk.
    const QString &windowState = m_settings->settings->option("advance.window.windowstate")->value().toString();

    // window minimum size.
    setMinimumSize(800, 500);

    // resize window size.
    QScreen *screen = QGuiApplication::primaryScreen();
    QRect screenGeometry = screen->geometry();

    resize(QSize(screenGeometry.width() * m_settings->settings->option("advance.window.window_width")->value().toDouble(),
                 screenGeometry.height() * m_settings->settings->option("advance.window.window_height")->value().toDouble()));
    show();

    // init window state.
    if (windowState == "window_maximum") {
        showMaximized();
    } else if (windowState == "fullscreen") {
        showFullScreen();
    }

    // Init find bar.
    connect(m_findBar, &FindBar::findNext, this, &Window::handleFindNext, Qt::QueuedConnection);
    connect(m_findBar, &FindBar::findPrev, this, &Window::handleFindPrev, Qt::QueuedConnection);
    connect(m_findBar, &FindBar::removeSearchKeyword, this, &Window::handleRemoveSearchKeyword, Qt::QueuedConnection);
    connect(m_findBar, &FindBar::updateSearchKeyword, this, [=] (QString file, QString keyword) {
        handleUpdateSearchKeyword(m_findBar, file, keyword);
    });
    connect(m_findBar, &FindBar::sigFindbarClose, this, &Window::slotFindbarClose, Qt::QueuedConnection);

    // Init replace bar.
    connect(m_replaceBar, &ReplaceBar::removeSearchKeyword, this, &Window::handleRemoveSearchKeyword, Qt::QueuedConnection);
    connect(m_replaceBar, &ReplaceBar::replaceAll, this, &Window::handleReplaceAll, Qt::QueuedConnection);
    connect(m_replaceBar, &ReplaceBar::replaceNext, this, &Window::handleReplaceNext, Qt::QueuedConnection);
    connect(m_replaceBar, &ReplaceBar::replaceRest, this, &Window::handleReplaceRest, Qt::QueuedConnection);
    connect(m_replaceBar, &ReplaceBar::replaceSkip, this, &Window::handleReplaceSkip, Qt::QueuedConnection);
    connect(m_replaceBar, &ReplaceBar::updateSearchKeyword, this, [=] (QString file, QString keyword) {
        handleUpdateSearchKeyword(m_replaceBar, file, keyword);
    });
    connect(m_replaceBar, &ReplaceBar::sigReplacebarClose, this, &Window::slotReplacebarClose, Qt::QueuedConnection);

    // Init jump line bar.
    QTimer::singleShot(0, m_jumpLineBar, SLOT(hide()));
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
    DAnchors<FindBar> anchors_findbar(m_findBar);
    anchors_findbar.setAnchor(Qt::AnchorBottom, m_centralWidget, Qt::AnchorBottom);
    anchors_findbar.setAnchor(Qt::AnchorHorizontalCenter, m_centralWidget, Qt::AnchorHorizontalCenter);
    anchors_findbar.setBottomMargin(5);
    m_findBar->raise();

    // Init replaceBar panel.
    DAnchors<ReplaceBar> anchors_replaceBar(m_replaceBar);
    anchors_replaceBar.setAnchor(Qt::AnchorBottom, m_centralWidget, Qt::AnchorBottom);
    anchors_replaceBar.setAnchor(Qt::AnchorHorizontalCenter, m_centralWidget, Qt::AnchorHorizontalCenter);
    anchors_replaceBar.setBottomMargin(5);
    anchors_replaceBar->raise();

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
}

Window::~Window()
{
    // We don't need clean pointers because application has exit here.
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
    QAction *findAction(new QAction(QApplication::translate("DTextEdit", "Find"), this));
    QAction *replaceAction(new QAction(QApplication::translate("DTextEdit", "Replace"), this));

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

    m_menu->setStyle(QStyleFactory::create("dlight"));
    m_menu->setMinimumWidth(150);

    ToolBar *toolBar = new ToolBar;
    toolBar->setTabbar(m_tabbar);

    titlebar()->setCustomWidget(toolBar, false);
    titlebar()->setAutoHideOnFullscreen(true);
    titlebar()->setSeparatorVisible(true);
    titlebar()->setFixedHeight(50);
    titlebar()->setMenu(m_menu);
    //titlebar()->setIcon(QIcon(":/images/logo_24.svg"));
    titlebar()->setIcon(QIcon::fromTheme("deepin-editor"));

    connect(m_tabbar, &DTabBar::tabBarDoubleClicked, titlebar(), &DTitlebar::doubleClicked, Qt::QueuedConnection);

    connect(m_tabbar, &Tabbar::closeTabs, this, &Window::handleTabsClosed, Qt::QueuedConnection);
    connect(m_tabbar, &Tabbar::requestHistorySaved, this, [=] (const QString &filePath) {
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

int Window::getTabIndex(const QString &file)
{
    return m_tabbar->indexOf(file);
}

void Window::activeTab(int index)
{
    DMainWindow::activateWindow();
    m_tabbar->setCurrentIndex(index);
}

void Window::addTab(const QString &filepath, bool activeTab)
{
    // check whether it is an editable file thround mimeType.
    if (Utils::isMimeTypeSupport(filepath)) {
        const QString &curPath = m_tabbar->currentPath();
        const QFileInfo fileInfo(filepath);
        QString tabName = fileInfo.fileName();
        EditWrapper *curWrapper = currentWrapper();

        if (!fileInfo.isWritable() && fileInfo.isReadable()) {
            tabName += QString(" (%1)").arg(tr("Read-Only"));
        }

        if (curWrapper) {
            // if the current page is a draft file and is empty
            // no need to create a new tab.
            if (curWrapper->textEditor()->toPlainText().isEmpty() &&
                !m_wrappers.keys().contains(filepath) &&
                Utils::isDraftFile(curPath))
            {
                QFile(curPath).remove();
                m_tabbar->updateTab(m_tabbar->currentIndex(), filepath, tabName);
                m_wrappers[filepath] = m_wrappers.take(curPath);
                m_wrappers[filepath]->updatePath(filepath);
                m_wrappers[filepath]->openFile(filepath);

                return;
            } else {
                if (m_tabbar->indexOf(filepath) != -1) {
                    m_tabbar->setCurrentIndex(m_tabbar->indexOf(filepath));
                }
            }
        }

        // check if have permission to read the file.
        QFile file(filepath);
        if (fileInfo.exists() && !file.open(QIODevice::ReadOnly)) {
            file.close();
            showNotify(QString(tr("You do not have permission to open %1")).arg(filepath));
            return;
        }
        file.close();

        if (m_tabbar->indexOf(filepath) == -1) {
            m_tabbar->addTab(filepath, tabName);

            if (!m_wrappers.contains(filepath)) {
                EditWrapper *wrapper = createEditor();
                wrapper->openFile(filepath);

                m_wrappers[filepath] = wrapper;

                showNewEditor(wrapper);
            }
        }

        // Activate window.
        activateWindow();

        // Active tab if activeTab is true.
        if (activeTab) {
            int tabIndex = m_tabbar->indexOf(filepath);
            if (tabIndex != -1) {
                m_tabbar->setCurrentIndex(tabIndex);
            }
        }
    } else {
        showNotify(tr("Invalid file: %1").arg(QFileInfo(filepath).fileName()));
    }
}

void Window::addTabWithWrapper(EditWrapper *wrapper, const QString &filepath, const QString &tabName, int index)
{
    if (index == -1) {
        index = m_tabbar->currentIndex() + 1;
    }

    // wrapper may be from anther window pointer.
    // reconnect signal.
    connect(wrapper->textEditor(), &DTextEdit::clickFindAction, this, &Window::popupFindBar, Qt::QueuedConnection);
    connect(wrapper->textEditor(), &DTextEdit::clickReplaceAction, this, &Window::popupReplaceBar, Qt::QueuedConnection);
    connect(wrapper->textEditor(), &DTextEdit::clickJumpLineAction, this, &Window::popupJumpLineBar, Qt::QueuedConnection);
    connect(wrapper->textEditor(), &DTextEdit::clickFullscreenAction, this, &Window::toggleFullscreen, Qt::QueuedConnection);
    connect(wrapper->textEditor(), &DTextEdit::popupNotify, this, &Window::showNotify, Qt::QueuedConnection);
    connect(wrapper->textEditor(), &DTextEdit::pressEsc, this, &Window::removeBottomWidget, Qt::QueuedConnection);

    wrapper->disconnect();
    connect(wrapper, &EditWrapper::requestSaveAs, this, &Window::saveAsFile);

    // add wrapper to this window.
    m_tabbar->addTabWithIndex(index, filepath, tabName);
    m_wrappers[filepath] = wrapper;
    wrapper->updatePath(filepath);

    showNewEditor(wrapper);
    wrapper->textEditor()->setThemeWithPath(m_themePath);
}

void Window::closeTab()
{
    const QString &filePath = m_tabbar->currentPath();
    const bool isBlankFile = QFileInfo(filePath).dir().absolutePath() == m_blankFileDir;
    EditWrapper *wrapper = m_wrappers.value(filePath);

    if (!wrapper)  {
        return;
    }

    // this property holds whether the document has been modified by the user
    bool isModified = wrapper->textEditor()->document()->isModified();

    // document has been modified or unsaved draft document.
    // need to prompt whether to save.
    if (isModified || (isBlankFile && !wrapper->textEditor()->toPlainText().isEmpty())) {
        DDialog *dialog = createDialog(tr("Save File"), tr("Do you want to save this file?"));

        connect(dialog, &DDialog::buttonClicked, this, [=] (int index) {
            dialog->hide();

            // don't save.
            if (index == 1) {
                m_tabbar->closeCurrentTab();

                // delete the draft document.
                if (isBlankFile) {
                    QFile(filePath).remove();
                }

                removeWrapper(filePath, true);
            }
            else if (index == 2) {
                // may to press CANEL button in the save dialog.
                if (saveFile()) {
                    m_tabbar->closeCurrentTab();
                    removeWrapper(filePath, true);
                }
            }

            focusActiveEditor();
        });

        dialog->exec();
    } else {
        // record last close path.
        m_closeFileHistory << m_tabbar->currentPath();

        // close tab directly, because all file is save automatically.
        m_tabbar->closeCurrentTab();

        // remove blank file.
        if (isBlankFile) {
            QFile::remove(filePath);
        }

        removeWrapper(filePath, true);
        focusActiveEditor();
    }
}

void Window::restoreTab()
{
    if (m_closeFileHistory.size() > 0) {
        addTab(m_closeFileHistory.takeLast()) ;
    }
}

EditWrapper* Window::createEditor()
{
    EditWrapper *wrapper = new EditWrapper();
    bool wordWrap = m_settings->settings->option("base.font.wordwrap")->value().toBool();

    wrapper->textEditor()->setThemeWithPath(m_themePath);
    wrapper->textEditor()->setSettings(m_settings);
    wrapper->textEditor()->setTabSpaceNumber(m_settings->settings->option("advance.editor.tabspacenumber")->value().toInt());
    wrapper->textEditor()->setFontFamily(m_settings->settings->option("base.font.family")->value().toString());
    wrapper->textEditor()->setModified(false);
    wrapper->textEditor()->setLineWrapMode(wordWrap);
    setFontSizeWithConfig(wrapper);

    connect(wrapper->textEditor(), &DTextEdit::clickFindAction, this, &Window::popupFindBar, Qt::QueuedConnection);
    connect(wrapper->textEditor(), &DTextEdit::clickReplaceAction, this, &Window::popupReplaceBar, Qt::QueuedConnection);
    connect(wrapper->textEditor(), &DTextEdit::clickJumpLineAction, this, &Window::popupJumpLineBar, Qt::QueuedConnection);
    connect(wrapper->textEditor(), &DTextEdit::clickFullscreenAction, this, &Window::toggleFullscreen, Qt::QueuedConnection);
    connect(wrapper->textEditor(), &DTextEdit::popupNotify, this, &Window::showNotify, Qt::QueuedConnection);
    connect(wrapper->textEditor(), &DTextEdit::pressEsc, this, &Window::removeBottomWidget, Qt::QueuedConnection);
    connect(wrapper, &EditWrapper::requestSaveAs, this, &Window::saveAsFile);

    connect(wrapper->textEditor(), &DTextEdit::modificationChanged, this, [=] (const QString &path, bool isModified) {
        int tabIndex = m_tabbar->indexOf(path);
        QString tabName = m_tabbar->textAt(tabIndex);
        QRegularExpression reg("[^*](.+)");
        QRegularExpressionMatch match = reg.match(tabName);

        tabName = match.captured(0);

        if (isModified) {
            tabName.prepend('*');
        }

        m_tabbar->setTabText(tabIndex, tabName);
    });

    return wrapper;
}

EditWrapper* Window::currentWrapper()
{
    return m_wrappers.value(m_tabbar->currentPath());
}

EditWrapper* Window::wrapper(const QString &filePath)
{
    return m_wrappers.value(filePath);
}

DTextEdit* Window::getTextEditor(const QString &filepath)
{
    return m_wrappers.value(filepath)->textEditor();
}

void Window::focusActiveEditor()
{
    if (m_tabbar->count() > 0) {
        currentWrapper()->textEditor()->setFocus();
    }
}

void Window::removeWrapper(const QString &filePath, bool isDelete)
{
    if (m_wrappers.contains(filePath)) {
        EditWrapper *wrapper = m_wrappers.value(filePath);

        m_editorWidget->removeWidget(wrapper);
        m_wrappers.remove(filePath);

        if (isDelete) {
            wrapper->deleteLater();
        }

        // remove all signals on this connection.
        disconnect(wrapper->textEditor(), 0, this, 0);
    }

    // Exit window after close all tabs.
    if (m_wrappers.isEmpty()) {
        DMainWindow::close();
    }
}

void Window::openFile()
{
    QFileDialog dialog;
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

    // save the directory string.
    m_settings->settings->option("advance.editor.file_dialog_dir")->setValue(dialog.directoryUrl().toLocalFile());

    if (mode != QDialog::Accepted) {
        return;
    }

    for (const QString &file : dialog.selectedFiles()) {
        addTab(file);
    }
}

bool Window::saveFile()
{
    const QString &currentPath = m_tabbar->currentPath();
    const QString &currentDir = QFileInfo(currentPath).absolutePath();
    const QFileInfo fileInfo(currentPath);
    bool isBlankFile = fileInfo.dir().absolutePath() == m_blankFileDir;

    QFile temporaryBuffer(currentPath);
    if (!temporaryBuffer.open(QIODevice::WriteOnly)) {
        showNotify(QString(tr("You do not have permission to save %1")).arg(fileInfo.fileName()));
        temporaryBuffer.close();
        return false;

        const QString content = getTextEditor(currentPath)->toPlainText();
        bool saveResult = m_rootSaveDBus->saveFile(currentPath.toUtf8(), content.toUtf8(), "");
        if (saveResult) {
            getTextEditor(currentPath)->setModified(false);
            showNotify(QString("Saved root file %1").arg(m_tabbar->currentName()));
        } else {
            showNotify(QString("Save root file %1 failed.").arg(m_tabbar->currentName()));
        }
        return saveResult;
    }

    temporaryBuffer.close();

    // file not finish loadding cannot be saved
    // otherwise you will save the content of the empty.
    // if (!m_wrappers[currentPath]->isLoadFinished() && !isBlankFile) {
    //     showNotify(tr("File cannot be saved when loading"));
    //     return false;
    // }

    // save blank file.
    if (isBlankFile) {
        return saveAsFile();
    }
    // save normal file.
    else {
        bool success = m_wrappers.value(m_tabbar->currentPath())->saveFile();

        if (!success) {
            DDialog *dialog = createDialog(tr("Unable to save the file"), tr("Do you want to save as another?"));

            connect(dialog, &DDialog::buttonClicked, this, [=] (int index) {
                dialog->hide();

                if (index == 2) {
                    saveAsFile();
                }
            });

            dialog->exec();
        } else {
            currentWrapper()->hideWarningNotices();
            if (m_wrappers.value(m_tabbar->currentPath())->getTextChangeFlag() == true) {
                showNotify(tr("Saved successfully"));
                m_wrappers.value(m_tabbar->currentPath())->setTextChangeFlag(false);
            }

        }
    }

    return true;
}

bool Window::saveAsFile()
{
    QString filePath = m_tabbar->currentPath();
    EditWrapper *wrapper = m_wrappers.value(filePath);
    bool isDraft = Utils::isDraftFile(filePath);
    QFileInfo fileInfo(filePath);

    if (!wrapper)
        return false;

    DFileDialog dialog(this, tr("Save File"));
    dialog.setAcceptMode(QFileDialog::AcceptSave);
    dialog.addComboBox(tr("Encoding"), Utils::getEncodeList());
    dialog.addComboBox(tr("Line Endings"), QStringList() << "Linux" << "Windows" << "Mac OS");
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
        const QByteArray encode = dialog.getComboBoxValue(tr("Encoding")).toUtf8();
        const QString endOfLine = dialog.getComboBoxValue(tr("Line Endings"));
        const QString newFilePath = dialog.selectedFiles().value(0);
        const QFileInfo newFileInfo(newFilePath);
        EditWrapper::EndOfLineMode eol = EditWrapper::eolUnix;

        if (endOfLine == "Windows") {
            eol = EditWrapper::eolDos;
        } else if (endOfLine == "Mac OS") {
            eol = EditWrapper::eolMac;
        }

        if (isDraft) {
            QFile(filePath).remove();
        }

        m_tabbar->updateTab(m_tabbar->currentIndex(), newFilePath, newFileInfo.fileName());

        wrapper->setTextCodec(encode);
        wrapper->updatePath(newFilePath);
        wrapper->setEndOfLineMode(eol);
        wrapper->saveFile();

        m_wrappers.remove(filePath);
        m_wrappers.insert(newFilePath, wrapper);

        wrapper->textEditor()->loadHighlighter();
    } else {
        return false;
    }

    return true;
}

void Window::decrementFontSize()
{
    int size = std::max(m_fontSize - 1, m_settings->minFontSize);
    m_settings->settings->option("base.font.size")->setValue(size);
}

void Window::incrementFontSize()
{
    int size = std::min(m_fontSize + 1, m_settings->maxFontSize);
    m_settings->settings->option("base.font.size")->setValue(size);
}

void Window::resetFontSize()
{
    m_settings->settings->option("base.font.size")->setValue(m_settings->defaultFontSize);
}

void Window::setFontSizeWithConfig(EditWrapper *wrapper)
{
    int size = m_settings->settings->option("base.font.size")->value().toInt();
    wrapper->textEditor()->setFontSize(size);

    m_fontSize = size;
}


void Window::popupFindBar()
{
    if (m_findBar->isVisible()) {
        if (m_findBar->isFocus()) {
            m_wrappers.value(m_tabbar->currentPath())->textEditor()->setFocus();
        } else {
            m_findBar->focus();
        }
    } else {
        //modify by guoshaoyu
        //addBottomWidget(m_findBar);

        QString tabPath = m_tabbar->currentPath();
        EditWrapper *wrapper = currentWrapper();

        //add by guoshaoyu
        if (wrapper->isVisible()) {
            wrapper->m_bottomBar->hide();
        }
        if (m_replaceBar->isVisible()) {
            m_replaceBar->hide();
        }
        m_findBar->show();

        QString text = wrapper->textEditor()->textCursor().selectedText();
        int row = wrapper->textEditor()->getCurrentLine();
        int column = wrapper->textEditor()->getCurrentColumn();
        int scrollOffset = wrapper->textEditor()->getScrollOffset();

        m_findBar->activeInput(text, tabPath, row, column, scrollOffset);

        QTimer::singleShot(10, this, [=] { m_findBar->focus(); });
    }
}

void Window::popupReplaceBar()
{
    if (m_replaceBar->isVisible()) {
        if (m_replaceBar->isFocus()) {
            m_wrappers.value(m_tabbar->currentPath())->textEditor()->setFocus();
        } else {
            m_replaceBar->focus();
        }
    } else {
        //add by guoshaoyu
        EditWrapper *wrapper = currentWrapper();
        if (wrapper->isVisible()) {
            wrapper->m_bottomBar->hide();
        }
        if (m_findBar->isVisible()) {
            m_findBar->hide();
        }
        m_replaceBar->show();
        //addBottomWidget(m_replaceBar);

        QString tabPath = m_tabbar->currentPath();
        QString text = wrapper->textEditor()->textCursor().selectedText();
        int row = wrapper->textEditor()->getCurrentLine();
        int column = wrapper->textEditor()->getCurrentColumn();
        int scrollOffset = wrapper->textEditor()->getScrollOffset();

        m_replaceBar->activeInput(text, tabPath, row, column, scrollOffset);

        QTimer::singleShot(10, this, [=] { m_replaceBar->focus(); });
    }
}

void Window::popupJumpLineBar()
{
    if (m_jumpLineBar->isVisible()) {
        if (m_jumpLineBar->isFocus()) {
            QTimer::singleShot(0, m_wrappers.value(m_tabbar->currentPath())->textEditor(), SLOT(setFocus()));
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
    }
}

void Window::popupSettingsDialog()
{
    DSettingsDialog *dialog = new DSettingsDialog(this);

    dialog->widgetFactory()->registerWidget("fontcombobox", Settings::createFontComBoBoxHandle);
    dialog->setProperty("_d_dtk_theme", "dark");
    dialog->setProperty("_d_QSSFilename", "DSettingsDialog");
    //modify by guoshaoyu
//    DThemeManager::instance()->registerWidget(dialog);

    dialog->updateSettings(m_settings->settings);
    m_settings->dtkThemeWorkaround(dialog, "dlight");

    dialog->exec();
    delete dialog;
    m_settings->settings->sync();
}

void Window::popupPrintDialog()
{
    QPrinter printer(QPrinter::HighResolution);
    QPrintPreviewDialog preview(&printer, this);

    DTextEdit *wrapper = currentWrapper()->textEditor();
    const QString &filePath = wrapper->filepath;
    const QString &fileDir = QFileInfo(filePath).dir().absolutePath();

    if (fileDir == m_blankFileDir) {
        printer.setOutputFileName(QString("%1/%2.pdf").arg(QDir::homePath(), m_tabbar->currentName()));
    } else {
        printer.setOutputFileName(QString("%1/%2.pdf").arg(fileDir, QFileInfo(filePath).baseName()));
    }

    printer.setOutputFormat(QPrinter::PdfFormat);

    connect(&preview, &QPrintPreviewDialog::paintRequested, this, [=] (QPrinter *printer) {
        currentWrapper()->textEditor()->print(printer);
    });

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

void Window::updateFont(const QString &fontName)
{
    for (EditWrapper *wrapper : m_wrappers.values()) {
        wrapper->textEditor()->setFontFamily(fontName);
    }
}

void Window::updateFontSize(int size)
{
    for (EditWrapper *wrapper : m_wrappers.values()) {
        wrapper->textEditor()->setFontSize(size);
    }

    m_fontSize = size;
}

void Window::updateTabSpaceNumber(int number)
{
    for (EditWrapper *wrapper : m_wrappers.values()) {
        wrapper->textEditor()->setTabSpaceNumber(number);
    }
}

void Window::changeTitlebarBackground(const QString &color)
{
//    titlebar()->setStyleSheet(QString("%1"
//                                      "Dtk--Widget--DTitlebar {"
//                                      "background: %2;"
//                                      "}").arg(m_titlebarStyleSheet).arg(color));

//    m_tabbar->setTabActiveColor(m_tabbarActiveColor);
}

void Window::changeTitlebarBackground(const QString &startColor, const QString &endColor)
{
//    titlebar()->setStyleSheet(QString("%1"
//                                      "Dtk--Widget--DTitlebar {"
//                                      "background: qlineargradient(x1:0 y1:0, x2:0 y2:1,"
//                                      "stop:0 rgba%2,  stop:1 rgba%3);"
//                                      "}").arg(m_titlebarStyleSheet).arg(startColor).arg(endColor));

//     m_tabbar->setTabActiveColor(m_tabbarActiveColor);
}

void Window::displayShortcuts()
{
    QRect rect = window()->geometry();
    QPoint pos(rect.x() + rect.width() / 2,
               rect.y() + rect.height() / 2);

    QStringList windowKeymaps;
    windowKeymaps << "addblanktab" << "newwindow" << "savefile"
                  << "saveasfile" << "selectnexttab" << "selectprevtab"
                  << "closetab" << "closeothertabs" << "restoretab"
                  << "openfile" << "incrementfontsize" << "decrementfontsize"
                  << "resetfontsize" << "togglefullscreen" << "find" << "replace"
                  << "jumptoline" << "saveposition" << "restoreposition"
                  << "escape" << "displayshortcuts" << "print";

    QJsonObject shortcutObj;
    QJsonArray jsonGroups;

    QJsonObject windowJsonGroup;
    windowJsonGroup.insert("groupName", QObject::tr("Window"));
    QJsonArray windowJsonItems;

    for (const QString &keymap : windowKeymaps) {
        auto option = m_settings->settings->group("shortcuts.window")->option(QString("shortcuts.window.%1").arg(keymap));
        QJsonObject jsonItem;
        jsonItem.insert("name", QObject::tr(option->name().toUtf8().data()));
        jsonItem.insert("value", option->value().toString().replace("Meta", "Super"));
        windowJsonItems.append(jsonItem);
    }

    windowJsonGroup.insert("groupItems", windowJsonItems);
    jsonGroups.append(windowJsonGroup);

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
                  << "togglecomment" << "undo" << "redo";

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

    shortcutObj.insert("shortcut", jsonGroups);

    QJsonDocument doc(shortcutObj);

    QStringList shortcutString;
    QString param1 = "-j=" + QString(doc.toJson().data());
    QString param2 = "-p=" + QString::number(pos.x()) + "," + QString::number(pos.y());
    shortcutString << param1 << param2;

    QProcess* shortcutViewProcess = new QProcess();
    shortcutViewProcess->startDetached("deepin-shortcut-viewer", shortcutString);

    connect(shortcutViewProcess, SIGNAL(finished(int)), shortcutViewProcess, SLOT(deleteLater()));
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

            if (QFile(blankTabPath).open(QIODevice::ReadWrite)) {
                qDebug() << "Create blank file: " << blankTabPath;
            } else {
                qDebug() << "Can't create blank file: " << blankTabPath;
            }
        }

    } else {
        blankTabPath = blankFile;
    }

    int blankFileIndex = getBlankFileIndex();

    m_tabbar->addTab(blankTabPath, tr("Blank document %1").arg(blankFileIndex));
    EditWrapper *wrapper = createEditor();
    wrapper->updatePath(blankTabPath);

    if (!blankFile.isEmpty() && Utils::fileExists(blankFile)) {
        wrapper->openFile(blankFile);
    }

    m_wrappers[blankTabPath] = wrapper;
    showNewEditor(wrapper);
}

void Window::handleTabCloseRequested(int index)
{
    activeTab(index);
    closeTab();
}

void Window::handleTabsClosed(const QStringList &tabList)
{
    if (tabList.isEmpty()) {
        return;
    }

    QList<EditWrapper *> needSaveList;
    for (const QString &path : tabList) {
        if (m_wrappers.contains(path)) {
            EditWrapper *wrapper = m_wrappers.value(path);
            bool isBlankFile = QFileInfo(path).dir().absolutePath() == m_blankFileDir;
            bool isContentEmpty = wrapper->textEditor()->toPlainText().isEmpty();
            bool isModified = wrapper->textEditor()->document()->isModified();

            if ( (isBlankFile && !isContentEmpty) ||
                 (!isBlankFile && isModified)) {
                needSaveList << wrapper;
            }
        }
    }

    // popup save file dialog.
    if (!needSaveList.isEmpty()) {
        DDialog *dialog = createDialog(tr("Save File"), tr("Do you want to save all the files?"));

        connect(dialog, &DDialog::buttonClicked, this, [&] (int index) {
            dialog->hide();

            // 1: don't save.
            // 2: save
            if (index == 1) {
                // need delete all draft documents.
                for (EditWrapper *wrapper : needSaveList) {
                    if (QFileInfo(wrapper->textEditor()->filepath).dir().absolutePath() == m_blankFileDir) {
                        QFile::remove(wrapper->textEditor()->filepath);
                    }
                }

            } else if (index == 2) {
                for (EditWrapper *wrapper : needSaveList) {
                    const QString &path = wrapper->textEditor()->filepath;
                    if (Utils::isDraftFile(path)) {
                        saveAsFile();
                    } else {
                        wrapper->saveFile();
                    }
                }
            }
        });

        int mode = dialog->exec();
        // click cancel button.
        if (mode == -1 || mode == 0) {
            return;
        }
    }

    // close tabs.
    for (const QString &path : tabList) {
        if (m_wrappers.contains(path)) {
            m_tabbar->closeTab(m_tabbar->indexOf(path));
        }
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

    for (auto wrapper : m_wrappers.values()) {
        wrapper->textEditor()->removeKeywords();
    }

    const QString &filepath = m_tabbar->fileAt(index);

    if (m_wrappers.contains(filepath)) {
        EditWrapper *wrapper = m_wrappers.value(filepath);
        wrapper->textEditor()->setFocus();
        m_editorWidget->setCurrentWidget(wrapper);
    }
}

void Window::handleJumpLineBarExit()
{
    QTimer::singleShot(0, currentWrapper()->textEditor(), SLOT(setFocus()));
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

        QTimer::singleShot(0, m_wrappers.value(file)->textEditor(), SLOT(setFocus()));
    }
}

void Window::handleFindNext()
{
    EditWrapper *wrapper = currentWrapper();

    wrapper->textEditor()->saveMarkStatus();
    wrapper->textEditor()->updateCursorKeywordSelection(wrapper->textEditor()->getPosition(), true);
    wrapper->textEditor()->renderAllSelections();
    wrapper->textEditor()->restoreMarkStatus();
}

void Window::handleFindPrev()
{
    EditWrapper *wrapper = currentWrapper();

    wrapper->textEditor()->saveMarkStatus();
    wrapper->textEditor()->updateCursorKeywordSelection(wrapper->textEditor()->getPosition(), false);
    wrapper->textEditor()->renderAllSelections();
    wrapper->textEditor()->restoreMarkStatus();
}

//add by guoshaoyu
void Window::slotFindbarClose()
{
    EditWrapper *wrapper = currentWrapper();
    if (wrapper->bottomBar()->isHidden())
    {
        wrapper->bottomBar()->show();
    }
}

void Window::slotReplacebarClose()
{
    EditWrapper *wrapper = currentWrapper();
    if (wrapper->bottomBar()->isHidden())
    {
        wrapper->bottomBar()->show();
    }
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
    currentWrapper()->textEditor()->removeKeywords();
}

void Window::handleUpdateSearchKeyword(QWidget *widget, const QString &file, const QString &keyword)
{
    if (file == m_tabbar->currentPath() && m_wrappers.contains(file)) {
        // Highlight keyword in text editor.
        m_wrappers.value(file)->textEditor()->highlightKeyword(keyword, m_wrappers.value(file)->textEditor()->getPosition());

        // Update input widget warning status along with keyword match situation.
        bool findKeyword = m_wrappers.value(file)->textEditor()->findKeywordForward(keyword);
        bool emptyKeyword = keyword.trimmed().isEmpty();

        auto *findBarWidget = qobject_cast<FindBar*>(widget);
        if (findBarWidget != nullptr) {
            if (emptyKeyword) {
                findBarWidget->setMismatchAlert(false);
            } else {
                findBarWidget->setMismatchAlert(!findKeyword);
            }
        } else {
            auto *replaceBarWidget = qobject_cast<ReplaceBar*>(widget);
            if (replaceBarWidget != nullptr) {
                if (emptyKeyword) {
                    replaceBarWidget->setMismatchAlert(false);
                } else {
                    replaceBarWidget->setMismatchAlert(!findKeyword);
                }
            }
        }
    }
}

void Window::addBottomWidget(QWidget *widget)
{
    if (m_centralLayout->count() >= 2) {
        removeBottomWidget();
    }

    m_centralLayout->addWidget(widget);
}

void Window::removeBottomWidget()
{
    auto item = m_centralLayout->takeAt(1);

    if (item) {
        item->widget()->hide();
    }
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
    m_tabbarActiveColor = jsonMap["app-colors"].toMap()["tab-active"].toString();

    const QString &tabbarStartColor = jsonMap["app-colors"].toMap()["tab-background-start-color"].toString();
    const QString &tabbarEndColor = jsonMap["app-colors"].toMap()["tab-background-end-color"].toString();

    //modify by guoshaoyu
//    if (QColor(backgroundColor).lightness() < 128) {
//        DThemeManager::instance()->setTheme("dark");
//    } else {
//        DThemeManager::instance()->setTheme("light");
//    }

    //changeTitlebarBackground(tabbarStartColor, tabbarEndColor);

    for (EditWrapper *wrapper : m_wrappers.values()) {
        wrapper->textEditor()->setThemeWithPath(path);
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

DDialog* Window::createDialog(const QString &title, const QString &content)
{
    DDialog *dialog = new DDialog(title, content, this);
    dialog->setWindowFlags(dialog->windowFlags() | Qt::WindowStaysOnTopHint);
    dialog->setIcon(QIcon(Utils::getQrcPath("logo_48.svg")));
    dialog->addButton(QString(tr("Cancel")), false, DDialog::ButtonNormal);
    dialog->addButton(QString(tr("Discard")), false, DDialog::ButtonNormal);
    dialog->addButton(QString(tr("Save")), true, DDialog::ButtonNormal);

    return dialog;
}

void Window::slotLoadContentTheme(DGuiApplicationHelper::ColorType themeType)
{
    qDebug() << "themeType:" << themeType;
    if(themeType == DGuiApplicationHelper::ColorType::LightType)
    {
        loadTheme("/usr/share/deepin-editor/themes/deepin.theme");
    }
    else if(themeType == DGuiApplicationHelper::ColorType::DarkType)
    {
        loadTheme("/usr/share/deepin-editor/themes/deepin_dark.theme");
    }
}

void Window::slotSettingResetTheme(const QString &path)
{

    QString strDefaultTheme = "/usr/share/deepin-editor/themes/deepin.theme";
    if (path == strDefaultTheme) {
        DGuiApplicationHelper::instance()->setPaletteType(DGuiApplicationHelper::LightType);
    }

    loadTheme(path);
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

    if (fi.exists() && !fi.isWritable()) {
        tabName.append(readOnlyStr);
    }

    m_tabbar->setTabText(m_tabbar->currentIndex(), tabName);
}

void Window::resizeEvent(QResizeEvent *e)
{
    if (m_themePanel->isVisible()) {
        updateThemePanelGeomerty();
    }

    if (!isMaximized() && !isFullScreen()) {
        QScreen *screen = QApplication::primaryScreen();
        QRect screenGeometry = screen->geometry();
        m_settings->settings->option("advance.window.window_width")->setValue(rect().width() * 1.0 / screenGeometry.width());
        m_settings->settings->option("advance.window.window_height")->setValue(rect().height() * 1.0 / screenGeometry.height());
    }

    //add by guoshaoyu
    m_findBar->resize(width() - 10, m_findBar->height());
    m_replaceBar->resize(width() - 10, m_replaceBar->height());

    DMainWindow::resizeEvent(e);
}

void Window::closeEvent(QCloseEvent *e)
{
    e->ignore();

    QList<EditWrapper *> needSaveList;
    for (EditWrapper *wrapper : m_wrappers) {
        // save all the draft documents.
        if (QFileInfo(wrapper->textEditor()->filepath).dir().absolutePath() == m_blankFileDir) {
            wrapper->saveFile();
            continue;
        }

        if (wrapper->textEditor()->document()->isModified()) {
            needSaveList << wrapper;
        }
    }

    if (!needSaveList.isEmpty()) {
        DDialog *dialog = createDialog(tr("Save File"), tr("Do you want to save all the files?"));

        connect(dialog, &DDialog::buttonClicked, this, [=] (int index) {
            dialog->hide();

            if (index == 2) {
                // save all the files.
                for (EditWrapper *wrapper : needSaveList) {
                    wrapper->saveFile();
                }
            }
        });

        const int mode = dialog->exec();
        if (mode == -1 || mode == 0) {
            return;
        }
    }

    // save all draft documents.
    QDir blankDir(m_blankFileDir);
    QFileInfoList blankFiles = blankDir.entryInfoList(QDir::Files);

    // clear blank files that have no content.
    for (const QFileInfo &blankFile : blankFiles) {
        QFile file(blankFile.absoluteFilePath());

        if (!file.open(QFile::ReadOnly)) {
            continue;
        }

        if (file.readAll().simplified().isEmpty()) {
            file.remove();
        }

        file.close();
    }

    e->accept();
    emit close();
}

void Window::keyPressEvent(QKeyEvent *e)
{
    QString key = Utils::getKeyshortcut(e);

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
    } else if (key == Utils::getKeyshortcutFromKeymap(m_settings, "window", "togglefullscreen")) {
        toggleFullscreen();
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
        removeBottomWidget();
    } else if (key == Utils::getKeyshortcutFromKeymap(m_settings, "window", "displayshortcuts")) {
        displayShortcuts();
    } else if (key == Utils::getKeyshortcutFromKeymap(m_settings, "window", "print")) {
        popupPrintDialog();
    } else if (e->key() == Qt::Key_F5) {
        currentWrapper()->refresh();
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

void Window::dropEvent(QDropEvent* event)
{
    const QMimeData* mimeData = event->mimeData();

    if (mimeData->hasUrls()) {
        for (auto url : mimeData->urls()) {
            addTab(url.toLocalFile(), true);
        }
    }
}
