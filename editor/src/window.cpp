/* -*- Mode: C++; indent-tabs-mode: nil; tab-width: 4 -*-
 * -*- coding: utf-8 -*-
 *
 * Copyright (C) 2011 ~ 2018 Deepin, Inc.
 *               2011 ~ 2018 Wang Yong
 *
 * Author:     Wang Yong <wangyong@deepin.com>
 * Maintainer: Wang Yong <wangyong@deepin.com>
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

#include "danchors.h"
#include "dthememanager.h"
#include "dtoast.h"
#include "utils.h"
#include "window.h"

#include <DSettingsGroup>
#include <DSettings>
#include <DSettingsOption>
#include <DTitlebar>
#include <QApplication>
#include <QPrintDialog>
#include <QPrintPreviewDialog>
#include <QPrinter>
#include <QScreen>
#include <QStyleFactory>

#ifdef DTKWIDGET_CLASS_DFileDialog
#include <DFileDialog>
#else
#include <QFileDialog>
#endif

DWM_USE_NAMESPACE

Window::Window(DMainWindow *parent)
    : DMainWindow(parent),
      m_centralWidget(new QWidget),
      m_editorWidget(new QStackedWidget),
      m_centralLayout(new QVBoxLayout(m_centralWidget)),
      m_tabbar(new Tabbar),
      m_jumpLineBar(new JumpLineBar(this)),
      m_replaceBar(new ReplaceBar),
      m_themePanel(new ThemePanel(this)),
      m_findBar(new FindBar),
      m_settings(new Settings(this)),
      m_windowManager(new DWindowManager),
      m_menu(new QMenu),
      m_titlebarStyleSheet(titlebar()->styleSheet()),
      m_themePath(m_settings->settings->option("advance.editor.theme")->value().toString())
{
    m_blankFileDir = QDir(QStandardPaths::standardLocations(QStandardPaths::DataLocation).first()).filePath("blank-files");
    m_rootSaveDBus = new DBusDaemon::dbus("com.deepin.editor.daemon", "/", QDBusConnection::systemBus(), this);

    // Init.
    installEventFilter(this);
    setAcceptDrops(true);

    // Apply qss theme.
    Utils::applyQss(this, "main.qss");
    loadTheme(m_themePath);

    // Init settings.
    connect(m_settings, &Settings::adjustFont, this, &Window::updateFont);
    connect(m_settings, &Settings::adjustFontSize, this, &Window::updateFontSize);
    connect(m_settings, &Settings::adjustTabSpaceNumber, this, &Window::updateTabSpaceNumber);

    // Init layout and editor.
    m_centralLayout->setMargin(0);
    m_centralLayout->setSpacing(0);

    m_centralLayout->addWidget(m_editorWidget);
    setCentralWidget(m_centralWidget);

    // Init titlebar.
    if (titlebar()) {
        initTitlebar();
    }

    // Init window state with config.
    // Below code must before this->titlebar()->setMenu, otherwise main menu can't display pre-build-in menu items by dtk.
    const QString &windowState = m_settings->settings->option("advance.window.window_state")->value().toString();

    // window minimum size.
    setMinimumSize(600, 400);

    if (windowState == "window_normal") {
        QScreen *screen = QGuiApplication::primaryScreen();
        QRect screenGeometry = screen->geometry();

        resize(QSize(screenGeometry.width() * m_settings->settings->option("advance.window.window_width")->value().toDouble(),
                     screenGeometry.height() * m_settings->settings->option("advance.window.window_height")->value().toDouble()));
        show();
    } else if (windowState == "window_maximum") {
        showMaximized();
    } else if (windowState == "fullscreen") {
        showFullScreen();
    }

    m_windowShowFlag = true;

    // Init find bar.
    connect(m_findBar, &FindBar::findNext, this, &Window::handleFindNext, Qt::QueuedConnection);
    connect(m_findBar, &FindBar::findPrev, this, &Window::handleFindPrev, Qt::QueuedConnection);
    connect(m_findBar, &FindBar::removeSearchKeyword, this, &Window::handleRemoveSearchKeyword, Qt::QueuedConnection);
    connect(m_findBar, &FindBar::updateSearchKeyword, this,
            [=] (QString file, QString keyword) {
                handleUpdateSearchKeyword(m_findBar, file, keyword);
            });

    // Init replace bar.
    connect(m_replaceBar, &ReplaceBar::removeSearchKeyword, this, &Window::handleRemoveSearchKeyword, Qt::QueuedConnection);
    connect(m_replaceBar, &ReplaceBar::replaceAll, this, &Window::handleReplaceAll, Qt::QueuedConnection);
    connect(m_replaceBar, &ReplaceBar::replaceNext, this, &Window::handleReplaceNext, Qt::QueuedConnection);
    connect(m_replaceBar, &ReplaceBar::replaceRest, this, &Window::handleReplaceRest, Qt::QueuedConnection);
    connect(m_replaceBar, &ReplaceBar::replaceSkip, this, &Window::handleReplaceSkip, Qt::QueuedConnection);
    connect(m_replaceBar, &ReplaceBar::updateSearchKeyword, this,
            [=] (QString file, QString keyword) {
                handleUpdateSearchKeyword(m_replaceBar, file, keyword);
            });

    // Init jump line bar.
    QTimer::singleShot(0, m_jumpLineBar, SLOT(hide()));

    connect(m_jumpLineBar, &JumpLineBar::jumpToLine, this, &Window::handleJumpLineBarJumpToLine, Qt::QueuedConnection);
    connect(m_jumpLineBar, &JumpLineBar::backToPosition, this, &Window::handleBackToPosition, Qt::QueuedConnection);
    connect(m_jumpLineBar, &JumpLineBar::lostFocusExit, this, &Window::handleJumpLineBarExit, Qt::QueuedConnection);

    // Make jump line bar pop at top-right of editor.
    DAnchorsBase::setAnchor(m_jumpLineBar, Qt::AnchorTop, m_centralWidget, Qt::AnchorTop);
    DAnchorsBase::setAnchor(m_jumpLineBar, Qt::AnchorRight, m_centralWidget, Qt::AnchorRight);

    // Init theme panel.
    DAnchorsBase::setAnchor(m_themePanel, Qt::AnchorTop, m_centralWidget, Qt::AnchorTop);
    DAnchorsBase::setAnchor(m_themePanel, Qt::AnchorBottom, m_centralWidget, Qt::AnchorBottom);
    DAnchorsBase::setAnchor(m_themePanel, Qt::AnchorRight, m_centralWidget, Qt::AnchorRight);

    // for the first time open the need be init.
    m_themePanel->setSelectionTheme(m_themePath);

    connect(m_themePanel, &ThemePanel::themeChanged, this, &Window::themeChanged);
    connect(this, &Window::requestDragEnterEvent, this, &Window::dragEnterEvent);
    connect(this, &Window::requestDropEvent, this, &Window::dropEvent);
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
    QAction *settingAction(new QAction(tr("Setting"), this));

    m_menu->addAction(newWindowAction);
    m_menu->addAction(newTabAction);
    m_menu->addAction(openFileAction);
    m_menu->addSeparator();
    m_menu->addAction(saveAction);
    m_menu->addAction(saveAsAction);
    m_menu->addAction(printAction);
    m_menu->addAction(switchThemeAction);
    m_menu->addSeparator();
    m_menu->addAction(settingAction);

    m_menu->setStyle(QStyleFactory::create("dlight"));

    titlebar()->setCustomWidget(m_tabbar, Qt::AlignVCenter, false);
    titlebar()->setAutoHideOnFullscreen(true);
    titlebar()->setSeparatorVisible(true);
    titlebar()->setFixedHeight(40);
    titlebar()->setMenu(m_menu);

    connect(m_tabbar, &Tabbar::doubleClicked, titlebar(), &DTitlebar::doubleClicked, Qt::QueuedConnection);
    connect(m_tabbar, &Tabbar::tabReleaseRequested, this, &Window::handleTabReleaseRequested, Qt::QueuedConnection);

    connect(m_tabbar->tabbar, &TabWidget::tabAddRequested, this, static_cast<void (Window::*)()>(&Window::addBlankTab), Qt::QueuedConnection);
    connect(m_tabbar->tabbar, &TabWidget::tabCloseRequested, this, &Window::handleTabCloseRequested, Qt::QueuedConnection);
    connect(m_tabbar->tabbar, &TabWidget::currentChanged, this, &Window::handleCurrentChanged, Qt::QueuedConnection);

    connect(newWindowAction, &QAction::triggered, this, &Window::newWindow);
    connect(newTabAction, &QAction::triggered, this, [=] () { addBlankTab(); });
    connect(openFileAction, &QAction::triggered, this, &Window::openFile);
    connect(saveAction, &QAction::triggered, this, &Window::saveFile);
    connect(saveAsAction, &QAction::triggered, this, &Window::saveAsFile);
    connect(printAction, &QAction::triggered, this, &Window::popupPrintDialog);
    connect(settingAction, &QAction::triggered, this, &Window::popupSettingsDialog);
    connect(switchThemeAction, &QAction::triggered, m_themePanel, &ThemePanel::popup);
    connect(m_themePanel, &ThemePanel::popupFinished, [=] { m_themePanel->setSelectionTheme(m_themePath); });
}

int Window::getTabIndex(const QString &file)
{
    return m_tabbar->getTabIndex(file);
}

void Window::activeTab(int index)
{
    activateWindow();
    m_tabbar->activeTabWithIndex(index);
}

void Window::addTab(const QString &filepath, bool activeTab)
{
    // check whether it is an editable file thround mimeType.
    if (Utils::isEditableFile(filepath)) {
        if (m_tabbar->getTabIndex(filepath) == -1) {
            m_tabbar->addTab(filepath, QFileInfo(filepath).fileName());

            if (!m_editorMap.contains(filepath)) {
                Editor *editor = createEditor();
                editor->loadFile(filepath);

                m_editorMap[filepath] = editor;

                showNewEditor(editor);
            }
        }

        // Activate window.
        activateWindow();

        // Active tab if activeTab is true.
        if (activeTab) {
            int tabIndex = m_tabbar->getTabIndex(filepath);
            if (tabIndex != -1) {
                m_tabbar->activeTabWithIndex(tabIndex);
            }
        }
    } else {
        showNotify(tr("%1 open invalid").arg(QFileInfo(filepath).fileName()));
    }
}

void Window::addTabWithContent(const QString &tabName, const QString &filepath, const QString &content, bool isModified, int index)
{
    // Default index is -1, we change it to right of current tab for new tab actoin in start.
    if (index == -1) {
        index = m_tabbar->getActiveTabIndex() + 1;
    }

    m_tabbar->addTabWithIndex(index, filepath, tabName);

    Editor *editor = createEditor();
    editor->updatePath(filepath);
    editor->textEditor->setPlainText(content);
    editor->textEditor->document()->setModified(isModified);

    m_editorMap[filepath] = editor;

    editor->textEditor->loadHighlighter();

    showNewEditor(editor);
}

void Window::closeTab()
{
    const QString &filePath = m_tabbar->getActiveTabPath();
    const bool isBlankFile = QFileInfo(filePath).dir().absolutePath() == m_blankFileDir;
    Editor *editor = m_editorMap.value(filePath);

    if (!editor)  {
        return;
    }

    // this property holds whether the document has been modified by the user
    bool isModified = editor->textEditor->document()->isModified();

    // document has been modified or unsaved draft document.
    // need to prompt whether to save.
    if (isModified || (isBlankFile && !editor->textEditor->toPlainText().isEmpty())) {
        DDialog *dialog = createSaveFileDialog(tr("Save file"), tr("Do you need to save the file?"));

        connect(dialog, &DDialog::buttonClicked, this, [=] (int index) {
            dialog->hide();

            // don't save.
            if (index == 1) {
                m_tabbar->closeActiveTab();

                // delete the draft document.
                if (isBlankFile) {
                    QFile(filePath).remove();
                }

                handleCloseFile(filePath);
            }
            else if (index == 2) {
                // may to press CANEL button in the save dialog.
                if (saveFile()) {
                    m_tabbar->closeActiveTab();
                    handleCloseFile(filePath);
                }
            }

            focusActiveEditor();
        });

        dialog->exec();
    } else {
        // record last close path.
        m_closeFileHistory << m_tabbar->getActiveTabPath();

        // close tab directly, because all file is save automatically.
        m_tabbar->closeActiveTab();

        // remove blank file.
        if (isBlankFile) {
            QFile::remove(filePath);
        }

        handleCloseFile(filePath);
        focusActiveEditor();
    }
}

void Window::restoreTab()
{
    if (m_closeFileHistory.size() > 0) {
        addTab(m_closeFileHistory.takeLast()) ;
    }
}

Editor* Window::createEditor()
{
    Editor *editor = new Editor();
    editor->textEditor->setThemeWithPath(m_themePath);
    editor->textEditor->setSettings(m_settings);
    editor->textEditor->setTabSpaceNumber(m_settings->settings->option("advance.editor.tab_space_number")->value().toInt());
    editor->textEditor->setFontFamily(m_settings->settings->option("base.font.family")->value().toString());
    setFontSizeWithConfig(editor);

    connect(editor->textEditor, &TextEditor::clickFindAction, this, &Window::popupFindBar, Qt::QueuedConnection);
    connect(editor->textEditor, &TextEditor::clickReplaceAction, this, &Window::popupReplaceBar, Qt::QueuedConnection);
    connect(editor->textEditor, &TextEditor::clickJumpLineAction, this, &Window::popupJumpLineBar, Qt::QueuedConnection);
    connect(editor->textEditor, &TextEditor::clickFullscreenAction, this, &Window::toggleFullscreen, Qt::QueuedConnection);
    connect(editor->textEditor, &TextEditor::popupNotify, this, &Window::showNotify, Qt::QueuedConnection);
    connect(editor->textEditor, &TextEditor::pressEsc, this, &Window::removeBottomWidget, Qt::QueuedConnection);

    return editor;
}

Editor* Window::getActiveEditor()
{
    return m_editorMap.value(m_tabbar->getActiveTabPath());
}

TextEditor* Window::getTextEditor(const QString &filepath)
{
    return m_editorMap.value(filepath)->textEditor;
}

void Window::focusActiveEditor()
{
    if (m_tabbar->getTabCount() > 0) {
        getActiveEditor()->textEditor->setFocus();
    }
}

void Window::openFile()
{
    QFileDialog dialog;
    dialog.setFileMode(QFileDialog::ExistingFiles);
    dialog.setAcceptMode(QFileDialog::AcceptOpen);

    // read history directory.
    const QString historyDir = m_settings->settings->option("advance.editor.file_dialog_dir")->value().toString();
    if (!historyDir.isEmpty()) {
        dialog.setDirectory(historyDir);
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

const QString Window::getSaveFilePath(QString &encode, QString &newline)
{
    encode = "UTF-8";
    newline = "Linux";

#ifdef DTKWIDGET_CLASS_DFileDialog
    DFileDialog dialog(this, tr("Save File"));
    dialog.setAcceptMode(QFileDialog::AcceptSave);
    dialog.addComboBox(tr("Encoding"), getEncodeList());
    dialog.addComboBox(tr("Line Endings"), QStringList() << "Linux" << "Windows" << "Mac OS");

    if (QFileInfo(m_tabbar->getActiveTabPath()).dir().absolutePath() != m_blankFileDir) {
        dialog.setDirectory(QFileInfo(m_tabbar->getActiveTabPath()).dir());
        dialog.selectFile(QFileInfo(m_tabbar->getActiveTabPath()).fileName());
    } else {
        dialog.setDirectory(QDir::homePath());
        dialog.selectFile(m_tabbar->getActiveTabName() + ".txt");
    }

    if (dialog.exec() == QDialog::Accepted) {
        encode = dialog.getComboBoxValue(tr("Encoding"));
        newline = dialog.getComboBoxValue(tr("Newline"));

        return dialog.selectedFiles().value(0);
    } else {
        return "";
    }
#else
    return QFileDialog::getSaveFileName(this, tr("Save File"), QDir(QDir::homePath()).filePath("Blank Document.txt"));
#endif
}

void Window::displayShortcuts()
{
    QRect rect = window()->geometry();
    QPoint pos(rect.x() + rect.width()/2 , rect.y() + rect.height()/2);

    QJsonObject shortcutObj;
    QJsonArray jsonGroups;

    QJsonObject windowJsonGroup;
    windowJsonGroup.insert("groupName", "Window");
    QJsonArray windowJsonItems;
    for (auto option : m_settings->settings->group("shortcuts.window")->options()) {
        QJsonObject jsonItem;
        jsonItem.insert("name", option->name());
        jsonItem.insert("value", option->value().toString().replace("Meta", "Super"));
        windowJsonItems.append(jsonItem);
    }
    windowJsonGroup.insert("groupItems", windowJsonItems);
    jsonGroups.append(windowJsonGroup);

    QJsonObject editorJsonGroup;
    editorJsonGroup.insert("groupName", "Editor");
    QJsonArray editorJsonItems;
    for (auto option : m_settings->settings->group("shortcuts.editor")->options()) {
        QJsonObject jsonItem;
        jsonItem.insert("name", option->name());
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

bool Window::saveFile()
{
    const QString &currentPath = m_tabbar->getActiveTabPath();
    const QString &currentDir = QFileInfo(currentPath).absolutePath();
    bool isBlankFile = QFileInfo(currentPath).dir().absolutePath() == m_blankFileDir;

    // file not finish loadding cannot be saved
    // otherwise you will save the content of the empty.
    if (!m_editorMap[currentPath]->isLoadFinished()) {
        showNotify(tr("File cannot be saved if it is not loaded."));
        return false;
    }

    // save blank file.
    if (isBlankFile) {
        QString encode, newline;
        QString filepath = getSaveFilePath(encode, newline);

        if (!filepath.isEmpty()) {
            const QString tabPath = m_tabbar->getActiveTabPath();
            saveFileAsAnotherPath(tabPath, filepath, encode, newline, true);
            return true;
        } else {
            return false;
        }
    }
    // save root file.
    else if (!m_editorMap[currentPath]->isWritable()) {
        const QString content = getTextEditor(currentPath)->toPlainText();
        bool saveResult = m_rootSaveDBus->saveFile(currentPath.toUtf8(), content.toUtf8(),
                                                   m_editorMap[currentPath]->fileEncode());

        if (saveResult) {
            getTextEditor(currentPath)->setModified(false);
            showNotify(tr("Saved root file %1").arg(m_tabbar->getActiveTabName()));
        } else {
            showNotify(tr("Save root file %1 failed.").arg(m_tabbar->getActiveTabName()));
        }

        return saveResult;
    }
    // save normal file.
    else {
        bool success = m_editorMap.value(m_tabbar->getActiveTabPath())->saveFile();

        if (!success) {
            DDialog *dialog = createSaveFileDialog(tr("Unable to save file"), tr("Do you want to save to another?"));

            connect(dialog, &DDialog::buttonClicked, this, [=] (int index) {
                dialog->hide();

                if (index == 2) {
                    saveAsFile();
                }
            });

            dialog->exec();
        } else {
            // don't show the toast.
            // showNotify(tr("Saved file %1").arg(m_tabbar->getActiveTabName()));
        }

        return true;
    }
}

void Window::saveAsFile()
{
    QString encode, newline;
    QString filepath = getSaveFilePath(encode, newline);
    QString tabPath = m_tabbar->getActiveTabPath();

    if (filepath != "" && filepath != tabPath) {
        saveFileAsAnotherPath(tabPath, filepath, encode, newline, false);
    } else if (filepath == tabPath) {
        m_editorMap.value(filepath)->saveFile(encode, newline);
    }
}

void Window::saveFileAsAnotherPath(const QString &fromPath, const QString &toPath, const QString &encode, const QString &newline, bool deleteOldFile)
{
    if (deleteOldFile) {
        QFile(fromPath).remove();
    }

    m_tabbar->updateTabWithIndex(m_tabbar->getActiveTabIndex(), toPath, QFileInfo(toPath).fileName());

    m_editorMap[toPath] = m_editorMap.take(fromPath);

    m_editorMap[toPath]->updatePath(toPath);
    m_editorMap[toPath]->saveFile(encode, newline);

    getActiveEditor()->textEditor->loadHighlighter();
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

void Window::setFontSizeWithConfig(Editor *editor)
{
    int size = m_settings->settings->option("base.font.size")->value().toInt();
    editor->textEditor->setFontSize(size);

    m_fontSize = size;
}

void Window::popupFindBar()
{
    if (m_findBar->isVisible()) {
        if (m_findBar->isFocus()) {
            QTimer::singleShot(0, m_editorMap.value(m_tabbar->getActiveTabPath())->textEditor, SLOT(setFocus()));
        } else {
            m_findBar->focus();
        }
    } else {
        addBottomWidget(m_findBar);

        QString tabPath = m_tabbar->getActiveTabPath();
        Editor *editor = getActiveEditor();
        QString text = editor->textEditor->textCursor().selectedText();
        int row = editor->textEditor->getCurrentLine();
        int column = editor->textEditor->getCurrentColumn();
        int scrollOffset = editor->textEditor->getScrollOffset();

        m_findBar->activeInput(text, tabPath, row, column, scrollOffset);
    }
}

void Window::popupReplaceBar()
{
    if (m_replaceBar->isVisible()) {
        if (m_replaceBar->isFocus()) {
            QTimer::singleShot(0, m_editorMap.value(m_tabbar->getActiveTabPath())->textEditor, SLOT(setFocus()));
        } else {
            m_replaceBar->focus();
        }
    } else {
        addBottomWidget(m_replaceBar);

        QString tabPath = m_tabbar->getActiveTabPath();
        Editor *editor = getActiveEditor();
        QString text = editor->textEditor->textCursor().selectedText();
        int row = editor->textEditor->getCurrentLine();
        int column = editor->textEditor->getCurrentColumn();
        int scrollOffset = editor->textEditor->getScrollOffset();

        m_replaceBar->activeInput(text, tabPath, row, column, scrollOffset);
    }
}

void Window::popupJumpLineBar()
{
    if (m_jumpLineBar->isVisible()) {
        if (m_jumpLineBar->isFocus()) {
            QTimer::singleShot(0, m_editorMap.value(m_tabbar->getActiveTabPath())->textEditor, SLOT(setFocus()));
        } else {
            m_jumpLineBar->focus();
        }
    } else {
        QString tabPath = m_tabbar->getActiveTabPath();
        Editor *editor = getActiveEditor();
        QString text = editor->textEditor->textCursor().selectedText();
        int row = editor->textEditor->getCurrentLine();
        int column = editor->textEditor->getCurrentColumn();
        int count = editor->textEditor->blockCount();
        int scrollOffset = editor->textEditor->getScrollOffset();

        m_jumpLineBar->activeInput(tabPath, row, column, count, scrollOffset);
    }
}

void Window::toggleFullscreen()
{
    if (isFullScreen()) {
        showNormal();
    }  else {
        showFullScreen();
    }
}

const QStringList Window::getEncodeList()
{
    QStringList encodeList;

    for (int mib : QTextCodec::availableMibs()) {
        QTextCodec *codec = QTextCodec::codecForMib(mib);
        QString encodeName = QString(codec->name()).toUpper();

        if (encodeName != "UTF-8" && !encodeList.contains(encodeName)) {
            encodeList.append(encodeName);
        }
    }

    encodeList.sort();
    encodeList.prepend("UTF-8");

    return encodeList;
}

void Window::remberPositionSave(bool notify)
{
    Editor *editor = getActiveEditor();

    m_remberPositionFilePath = m_tabbar->getActiveTabPath();
    m_remberPositionRow = editor->textEditor->getCurrentLine();
    m_remberPositionColumn = editor->textEditor->getCurrentColumn();
    m_remberPositionScrollOffset = editor->textEditor->getScrollOffset();

    if (notify) {
        // showNotify(tr("记住当前位置"));
    }
}

void Window::remberPositionRestore()
{
    if (m_remberPositionFilePath != "") {
        if (m_editorMap.contains(m_remberPositionFilePath)) {
            QString filepath = m_remberPositionFilePath;
            int scrollOffset = m_remberPositionScrollOffset;
            int row = m_remberPositionRow;
            int column = m_remberPositionColumn;

            remberPositionSave(false);

            activeTab(m_tabbar->getTabIndex(filepath));

            QTimer::singleShot(
                0, this,
                [=] () {
                    m_editorMap.value(filepath)->textEditor->scrollToLine(scrollOffset, row, column);
                });
        } else {
            if (Utils::fileExists(m_remberPositionFilePath)) {
                QString filepath = m_remberPositionFilePath;
                int scrollOffset = m_remberPositionScrollOffset;
                int row = m_remberPositionRow;
                int column = m_remberPositionColumn;

                remberPositionSave(false);

                addTab(filepath);

                QTimer::singleShot(
                    0, this,
                    [=] () {
                        m_editorMap.value(filepath)->textEditor->scrollToLine(scrollOffset, row, column);
                    });
            } else {
                // showNotify(tr("记录位置的文件已经不存在了"));
            }
        }
    }
}

void Window::updateFont(const QString &fontName)
{
    foreach (Editor *editor, m_editorMap.values()) {
        editor->textEditor->setFontFamily(fontName);
    }
}

void Window::updateFontSize(int size)
{
    foreach (Editor *editor, m_editorMap.values()) {
        editor->textEditor->setFontSize(size);
    }

    m_fontSize = size;
}

void Window::updateTabSpaceNumber(int number)
{
    foreach (Editor *editor, m_editorMap.values()) {
        editor->textEditor->setTabSpaceNumber(number);
    }
}

void Window::resizeEvent(QResizeEvent*)
{
    if (m_windowShowFlag) {
        auto states = m_windowManager->getWindowStates(winId());
        if (!states.contains("_NET_WM_STATE_MAXIMIZED_VERT")) {
            QScreen *screen = QGuiApplication::primaryScreen();
            QRect screenGeometry = screen->geometry();
            m_settings->settings->option("advance.window.window_width")->setValue(rect().width() * 1.0 / screenGeometry.width());
            m_settings->settings->option("advance.window.window_height")->setValue(rect().height() * 1.0 / screenGeometry.height());
        }

        DAnchorsBase::setAnchor(m_themePanel, Qt::AnchorTop, m_centralWidget, Qt::AnchorTop);
        DAnchorsBase::setAnchor(m_themePanel, Qt::AnchorBottom, m_centralWidget, Qt::AnchorBottom);
        DAnchorsBase::setAnchor(m_themePanel, Qt::AnchorRight, m_centralWidget, Qt::AnchorRight);
    }
}

void Window::closeEvent(QCloseEvent *e)
{
    QDir blankDir(m_blankFileDir);
    QFileInfoList blankFiles = blankDir.entryInfoList(QDir::Files);

    // save blank content.
    for(Editor *editor : m_editorMap) {
        bool isBlankFile = QFileInfo(editor->textEditor->filepath).dir().absolutePath() == m_blankFileDir;
        if (isBlankFile) {
            editor->saveFile();
        }
    }

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

    emit close();
}

void Window::keyPressEvent(QKeyEvent *keyEvent)
{
    QString key = Utils::getKeyshortcut(keyEvent);

    if (key == Utils::getKeyshortcutFromKeymap(m_settings, "window", "addblanktab")) {
        addBlankTab();
    } else if (key == Utils::getKeyshortcutFromKeymap(m_settings, "window", "newwindow")) {
        newWindow();
    } else if (key == Utils::getKeyshortcutFromKeymap(m_settings, "window", "savefile")) {
        saveFile();
    } else if (key == Utils::getKeyshortcutFromKeymap(m_settings, "window", "saveasfile")) {
        saveAsFile();
    } else if (key == Utils::getKeyshortcutFromKeymap(m_settings, "window", "selectnexttab")) {
        m_tabbar->selectNextTab();
    } else if (key == Utils::getKeyshortcutFromKeymap(m_settings, "window", "selectprevtab")) {
        m_tabbar->selectPrevTab();
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
    } else {
        // Post event to window widget if match Alt+0 ~ Alt+9
        QRegularExpression re("^Alt\\+\\d");
        QRegularExpressionMatch match = re.match(key);
        if (match.hasMatch()) {
            auto tabIndex = key.replace("Alt+", "").toInt();
            if (tabIndex == 9) {
                if (m_tabbar->tabbar->count() > 1) {
                    activeTab(m_tabbar->tabbar->count() - 1);
                }
            } else {
                if (tabIndex <= m_tabbar->tabbar->count()) {
                    activeTab(tabIndex - 1);
                }
            }
        }
    }
}

int Window::getBlankFileIndex()
{
    // Get blank tab index list.
    QList<int> tabIndexes;
    for (int i = 0; i < m_tabbar->tabbar->tabFiles.size(); i++) {
        if (QFileInfo(m_tabbar->tabbar->tabFiles[i]).dir().absolutePath() == m_blankFileDir) {
            auto tabNameList = m_tabbar->tabbar->tabText(i).split("Blank document ");
            if (tabNameList.size() > 1) {
                tabIndexes << tabNameList[1].toInt();
            }
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

void Window::addBlankTab(const QString &blankFile)
{
    QString blankTabPath;
    if (blankFile.isEmpty()) {
        blankTabPath = QDir(m_blankFileDir).filePath(QString("blank_file_%1").arg(QDateTime::currentDateTime().toString("yyyy-MM-dd_hh-mm-ss-zzz")));

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

    auto blankFileIndex = getBlankFileIndex();

    m_tabbar->addTab(blankTabPath, tr("Blank document %1").arg(blankFileIndex));
    Editor *editor = createEditor();
    editor->updatePath(blankTabPath);

    if (!blankFile.isEmpty() && Utils::fileExists(blankFile)) {
        editor->loadFile(blankFile);
    }

    m_editorMap[blankTabPath] = editor;

    showNewEditor(editor);
}

void Window::handleTabReleaseRequested(const QString &tabName, const QString &filepath, int index)
{
    TextEditor *editor = getTextEditor(filepath);
    const QString content = editor->toPlainText();
    const bool isModified = editor->document()->isModified();

    m_tabbar->closeTabWithIndex(index);
    handleCloseFile(filepath);

    emit dropTabOut(tabName, filepath, content, isModified);
}

void Window::handleTabCloseRequested(int index)
{
    activeTab(index);
    closeTab();
}

void Window::handleCloseFile(const QString &filepath)
{
    if (m_editorMap.contains(filepath)) {
        Editor *editor = m_editorMap.value(filepath);

        m_editorWidget->removeWidget(editor);
        m_editorMap.remove(filepath);

        editor->deleteLater();
    }

    // Exit window after close all tabs.
    if (m_editorMap.isEmpty()) {
        DMainWindow::close();
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

    for (auto editor : m_editorMap.values()) {
        editor->textEditor->removeKeywords();
    }

    const QString &filepath = m_tabbar->tabbar->tabFiles.value(index);

    if (m_editorMap.contains(filepath)) {
        Editor *editor = m_editorMap.value(filepath);
        editor->textEditor->setFocus();
        m_editorWidget->setCurrentWidget(editor);
    }
}

void Window::handleJumpLineBarExit()
{
    QTimer::singleShot(0, getActiveEditor()->textEditor, SLOT(setFocus()));
}

void Window::handleJumpLineBarJumpToLine(const QString &filepath, int line, bool focusEditor)
{
    if (m_editorMap.contains(filepath)) {
        getTextEditor(filepath)->jumpToLine(line, true);

        if (focusEditor) {
            QTimer::singleShot(0, getTextEditor(filepath), SLOT(setFocus()));
        }
    }
}

void Window::handleBackToPosition(const QString &file, int row, int column, int scrollOffset)
{
    if (m_editorMap.contains(file)) {
        m_editorMap.value(file)->textEditor->scrollToLine(scrollOffset, row, column);

        QTimer::singleShot(0, m_editorMap.value(file)->textEditor, SLOT(setFocus()));
    }
}

void Window::handleFindNext()
{
    Editor *editor = getActiveEditor();

    editor->textEditor->saveMarkStatus();
    editor->textEditor->updateCursorKeywordSelection(editor->textEditor->getPosition(), true);
    editor->textEditor->renderAllSelections();
    editor->textEditor->restoreMarkStatus();
}

void Window::handleFindPrev()
{
    Editor *editor = getActiveEditor();

    editor->textEditor->saveMarkStatus();
    editor->textEditor->updateCursorKeywordSelection(editor->textEditor->getPosition(), false);
    editor->textEditor->renderAllSelections();
    editor->textEditor->restoreMarkStatus();
}

void Window::handleReplaceAll(const QString &replaceText, const QString &withText)
{
    Editor *editor = getActiveEditor();

    editor->textEditor->replaceAll(replaceText, withText);
}

void Window::handleReplaceNext(const QString &replaceText, const QString &withText)
{
    Editor *editor = getActiveEditor();

    editor->textEditor->replaceNext(replaceText, withText);
}

void Window::handleReplaceRest(const QString &replaceText, const QString &withText)
{
    Editor *editor = getActiveEditor();

    editor->textEditor->replaceRest(replaceText, withText);
}

void Window::handleReplaceSkip()
{
    Editor *editor = getActiveEditor();

    editor->textEditor->updateCursorKeywordSelection(editor->textEditor->getPosition(), true);
    editor->textEditor->renderAllSelections();
}

void Window::handleRemoveSearchKeyword()
{
    getActiveEditor()->textEditor->removeKeywords();
}

void Window::handleUpdateSearchKeyword(QWidget *widget, const QString &file, const QString &keyword)
{
    if (file == m_tabbar->getActiveTabPath() && m_editorMap.contains(file)) {
        // Highlight keyword in text editor.
        m_editorMap.value(file)->textEditor->highlightKeyword(keyword, m_editorMap.value(file)->textEditor->getPosition());

        // Update input widget warning status along with keyword match situation.
        bool findKeyword = m_editorMap.value(file)->textEditor->findKeywordForward(keyword);
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

void Window::removeActiveBlankTab(bool needSaveBefore)
{
    QString blankFile = m_tabbar->getActiveTabPath();

    if (needSaveBefore) {
        if (!saveFile()) {
            // Do nothing if need save but last user not select save file anyway.
            return;
        }

        // Record last close path.
        m_closeFileHistory << m_tabbar->getActiveTabPath();
    }

    // Close current tab.
    m_tabbar->closeActiveTab();

    // Remove blank file from blank file directory.
    QFile(blankFile).remove();
}

void Window::removeActiveReadonlyTab()
{
    QString tabPath = m_tabbar->getActiveTabPath();
    QString realpath = QFileInfo(tabPath).fileName().replace(m_readonlySeparator, QDir().separator());

    m_closeFileHistory << realpath;
    m_tabbar->closeActiveTab();
    focusActiveEditor();

    QFile(tabPath).remove();
}

void Window::showNewEditor(Editor *editor)
{
    m_editorWidget->addWidget(editor);
    m_editorWidget->setCurrentWidget(editor);
}

void Window::showNotify(QString message)
{
    Utils::toast(message, this);
}

DDialog* Window::createSaveFileDialog(QString title, QString content)
{
    DDialog *dialog = new DDialog(title, content, this);
    dialog->setWindowFlags(dialog->windowFlags() | Qt::WindowStaysOnTopHint);
    dialog->setIcon(QIcon(Utils::getQrcPath("logo_48.svg")));
    dialog->addButton(QString(tr("Cancel")), false, DDialog::ButtonNormal);
    dialog->addButton(QString(tr("Don't Save")), false, DDialog::ButtonNormal);
    dialog->addButton(QString(tr("Save")), true, DDialog::ButtonNormal);

    return dialog;
}

void Window::popupSettingsDialog()
{
    DSettingsDialog *dialog = new DSettingsDialog(this);

    dialog->setProperty("_d_dtk_theme", "dark");
    dialog->setProperty("_d_QSSFilename", "DSettingsDialog");
    DThemeManager::instance()->registerWidget(dialog);

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
    connect(&preview, &QPrintPreviewDialog::paintRequested, this,
            [=] (QPrinter *printer) {
                getActiveEditor()->textEditor->print(printer);
            });
    preview.exec();
}

void Window::changeTitlebarBackground(const QString &color)
{
    titlebar()->setStyleSheet(QString("%1"
                                      "Dtk--Widget--DTitlebar {"
                                      "background: %2;"
                                      "}").arg(m_titlebarStyleSheet).arg(color));

    m_tabbar->setTabActiveColor(m_tabbarActiveColor);
}

void Window::changeTitlebarBackground(const QString &startColor, const QString &endColor)
{
    titlebar()->setStyleSheet(QString("%1"
                                      "Dtk--Widget--DTitlebar {"
                                      "background: qlineargradient(x1:0 y1:0, x2:0 y2:1,"
                                      "stop:0 rgba%2,  stop:1 rgba%3);"
                                      "}").arg(m_titlebarStyleSheet).arg(startColor).arg(endColor));

    m_tabbar->setTabActiveColor(m_tabbarActiveColor);
}

void Window::loadTheme(const QString &path)
{
    m_themePath = path;

    QVariantMap jsonMap = Utils::getThemeMapFromPath(path);
    const QString &backgroundColor = jsonMap["editor-colors"].toMap()["background-color"].toString();
    m_tabbarActiveColor = jsonMap["app-colors"].toMap()["tab-active"].toString();

    const QString &tabbarStartColor = jsonMap["app-colors"].toMap()["tab-background-start-color"].toString();
    const QString &tabbarEndColor = jsonMap["app-colors"].toMap()["tab-background-end-color"].toString();

    if (QColor(backgroundColor).lightness() < 128) {
        DThemeManager::instance()->setTheme("dark");
    } else {
        DThemeManager::instance()->setTheme("light");
    }

    changeTitlebarBackground(tabbarStartColor, tabbarEndColor);

    for (Editor *editor : m_editorMap.values()) {
        editor->textEditor->setThemeWithPath(path);
    }

    m_themePanel->setBackground(backgroundColor);
    m_jumpLineBar->setBackground(backgroundColor);
    m_replaceBar->setBackground(backgroundColor);
    m_findBar->setBackground(backgroundColor);
    m_tabbar->tabbar->setDNDColor(jsonMap["app-colors"].toMap()["tab-dnd-start"].toString(), jsonMap["app-colors"].toMap()["tab-dnd-end"].toString());
    m_tabbar->tabbar->setBackground(tabbarStartColor, tabbarEndColor);

    const QString &frameSelectedColor = jsonMap["app-colors"].toMap()["themebar-frame-selected"].toString();
    const QString &frameNormalColor = jsonMap["app-colors"].toMap()["themebar-frame-normal"].toString();
    m_themePanel->setFrameColor(frameSelectedColor, frameNormalColor);
    m_settings->settings->option("advance.editor.theme")->setValue(path);
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

bool Window::eventFilter(QObject *, QEvent *event)
{
    // Try hide word completion window when window start to move or size change.
    if (m_windowShowFlag && (event->type() == QEvent::MouseMove || event->type() == QEvent::WindowStateChange)) {
    }

    return false;
}

void Window::addBlankTab()
{
    addBlankTab("");
}
