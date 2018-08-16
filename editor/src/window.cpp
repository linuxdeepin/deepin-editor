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
#include "wordcompletionitem.h"
#include "dthememanager.h"
#include "dtoast.h"
#include "utils.h"
#include "window.h"
#include "themeitem.h"
#include "wordcompletionitem.h"

#include <DSettingsGroup>
#include <QSqlQuery>
#include <QSqlRecord>
#include <QSqlError>
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
      m_editorWidget(new QWidget),
      m_editorLayout(new QStackedLayout(m_editorWidget)),
      m_centralLayout(new QVBoxLayout(m_centralWidget)),
      m_titleBar(new Titlebar),
      m_jumpLineBar(new JumpLineBar(this)),
      m_replaceBar(new ReplaceBar),
      m_themeBar(new ThemeBar(this)),
      m_findBar(new FindBar),
      m_settings(new Settings(this)),
      m_windowManager(new DWindowManager),
      m_menu(new QMenu),
      m_newWindowAction(new QAction(tr("New window"), this)),
      m_newTabAction(new QAction(tr("New tab"), this)),
      m_openFileAction(new QAction(tr("Open file"), this)),
      m_saveAction(new QAction(tr("Save"), this)),
      m_saveAsAction(new QAction(tr("Save as"), this)),
      m_printAction(new QAction(tr("Print"), this)),
      m_switchThemeAction(new QAction(tr("Switch theme"), this)),
      m_settingAction(new QAction(tr("Setting"), this)),
      m_titlebarStyleSheet(titlebar()->styleSheet()),
      m_themeName(m_settings->settings->option("base.theme.default")->value().toString())
{
    // Init.
    installEventFilter(this);
    setAcceptDrops(true);

    m_blankFileDir = QDir(QStandardPaths::standardLocations(QStandardPaths::DataLocation).first()).filePath("blank-files");
    m_readonlyFileDir = QDir(QStandardPaths::standardLocations(QStandardPaths::DataLocation).first()).filePath("readonly-files");
    autoSaveDBus = new DBusDaemon::dbus("com.deepin.editor.daemon", "/", QDBusConnection::systemBus(), this);

    // Init settings.
    connect(m_settings, &Settings::adjustFont, this, &Window::updateFont);
    connect(m_settings, &Settings::adjustFontSize, this, &Window::updateFontSize);
    connect(m_settings, &Settings::adjustTabSpaceNumber, this, &Window::updateTabSpaceNumber);

    // Init layout and editor.
    m_centralLayout->setMargin(0);
    m_centralLayout->setSpacing(0);

    m_editorLayout->setMargin(0);
    m_editorLayout->setSpacing(0);

    m_centralLayout->addWidget(m_editorWidget);
    setCentralWidget(m_centralWidget);

//    m_inCompletingTimer = new QTimer();
//    m_inCompletingTimer->setSingleShot(true);
//    connect(m_inCompletingTimer, &QTimer::timeout, this, [=] { m_inCompleting = false; });

    // Init titlebar.
    if (titlebar()) {
        titlebar()->setCustomWidget(m_titleBar, Qt::AlignVCenter, false);
        titlebar()->setAutoHideOnFullscreen(true);
        titlebar()->setSeparatorVisible(true);
        titlebar()->setMenu(m_menu);

        connect(m_titleBar, &Titlebar::doubleClicked, titlebar(), &DTitlebar::doubleClicked, Qt::QueuedConnection);
        connect(m_titleBar, &Titlebar::tabReleaseRequested, this, &Window::handleTabReleaseRequested, Qt::QueuedConnection);

        connect(m_titleBar->tabbar, &Tabbar::tabAddRequested, this, static_cast<void (Window::*)()>(&Window::addBlankTab), Qt::QueuedConnection);
        connect(m_titleBar->tabbar, &Tabbar::tabCloseRequested, this, &Window::handleTabCloseRequested, Qt::QueuedConnection);
        connect(m_titleBar->tabbar, &Tabbar::currentChanged, this, &Window::handleCurrentChanged, Qt::QueuedConnection);

        m_menu->setStyle(QStyleFactory::create("dlight"));

        // Init main menu.
        m_menu->addAction(m_newWindowAction);
        m_menu->addAction(m_newTabAction);
        m_menu->addAction(m_openFileAction);
        m_menu->addSeparator();
        m_menu->addAction(m_saveAction);
        m_menu->addAction(m_saveAsAction);
        m_menu->addAction(m_printAction);
        m_menu->addAction(m_switchThemeAction);
        m_menu->addSeparator();
        m_menu->addAction(m_settingAction);

        connect(m_newWindowAction, &QAction::triggered, this, &Window::newWindow);
        connect(m_newTabAction, &QAction::triggered, this, [=] () { addBlankTab(); });
        connect(m_openFileAction, &QAction::triggered, this, &Window::openFile);
        connect(m_saveAction, &QAction::triggered, this, &Window::saveFile);
        connect(m_saveAsAction, &QAction::triggered, this, &Window::saveAsFile);
        connect(m_printAction, &QAction::triggered, this, &Window::popupPrintDialog);
        connect(m_switchThemeAction, &QAction::triggered, this, &Window::popupThemeBar);
        connect(m_settingAction, &QAction::triggered, this, &Window::popupSettingsDialog);
    }

    // Init window state with config.
    // Below code must before this->titlebar()->setMenu, otherwise main menu can't display pre-build-in menu items by dtk.
    auto windowState = m_settings->settings->option("advance.window.window_state")->value().toString();
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

    // Init theme bar.
    DAnchorsBase::setAnchor(m_themeBar, Qt::AnchorTop, m_centralWidget, Qt::AnchorTop);
    DAnchorsBase::setAnchor(m_themeBar, Qt::AnchorBottom, m_centralWidget, Qt::AnchorBottom);
    DAnchorsBase::setAnchor(m_themeBar, Qt::AnchorRight, m_centralWidget, Qt::AnchorRight);

    connect(m_themeBar, &ThemeBar::changeTheme, this, &Window::themeChanged);
    connect(this, &Window::requestDragEnterEvent, this, &Window::dragEnterEvent);
    connect(this, &Window::requestDropEvent, this, &Window::dropEvent);

    QVariantMap jsonMap = Utils::getThemeNodeMap(m_themeName);
    auto frameSelectedColor = jsonMap["app-colors"].toMap()["themebar-frame-selected"].toString();
    auto frameNormalColor = jsonMap["app-colors"].toMap()["themebar-frame-normal"].toString();

    for (DSimpleListItem* item : m_themeBar->items) {
        (static_cast<ThemeItem*>(item))->setFrameColor(frameSelectedColor, frameNormalColor);
    }

    // Init words database.
    // m_wordsDB = QSqlDatabase::addDatabase("QSQLITE");
    // m_wordsDB.setDatabaseName(WORDS_DB_FILE_PATH);

    // if (!m_wordsDB.open()) {
    //     qDebug() << "Error: connection with database fail";
    // } else {
    //     qDebug() << "Database: connection ok";
    // }

    // m_wordCompletionWindow = new WordCompletionWindow();

    // Apply qss theme.
    Utils::applyQss(this, "main.qss");
    loadTheme(m_themeName);
}

Window::~Window()
{
    // We don't need clean pointers because application has exit here.
}

int Window::getTabIndex(const QString &file)
{
    for (int i = 0; i < m_titleBar->tabbar->tabFiles.size(); i++) {
        qDebug() << "******** Tab " << i << m_titleBar->tabbar->tabFiles[i];
    }

    return m_titleBar->getTabIndex(file);
}

void Window::activeTab(int index)
{
    activateWindow();
    m_titleBar->activeTabWithIndex(index);
}

void Window::addTab(const QString &file, bool activeTab)
{
    QString filepath = file;

    if (Utils::isEditableFile(filepath)) {
        if (!Utils::fileIsWritable(file)) {
            filepath = QDir(m_readonlyFileDir).filePath(filepath.replace(QDir().separator(), m_readonlySeparator));

            if (!Utils::fileExists(filepath)) {
                QString directory = QFileInfo(filepath).dir().absolutePath();
                QDir().mkpath(directory);
            }

            QFile::copy(file, filepath);
        }

        if (m_titleBar->getTabIndex(filepath) == -1) {
            m_titleBar->addTab(filepath, QFileInfo(file).fileName());

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
            int tabIndex = m_titleBar->getTabIndex(filepath);
            if (tabIndex != -1) {
                m_titleBar->activeTabWithIndex(tabIndex);
            }
        }
    } else {
        showNotify(tr("%1 不是一个有效的可编辑文件").arg(QFileInfo(filepath).fileName()));
    }
}

void Window::addTabWithContent(const QString &tabName, const QString &filepath, const QString &content, int index)
{
    qDebug() << "!!!!!!!!!! editorMap contains path '" << filepath << "' " << m_editorMap.contains(filepath);

    // Default index is -1, we change it to right of current tab for new tab actoin in start.
    if (index == -1) {
        index = m_titleBar->getActiveTabIndex() + 1;
    }

    m_titleBar->addTabWithIndex(index, filepath, tabName);

    Editor *editor = createEditor();
    editor->updatePath(filepath);
    editor->textEditor->setPlainText(content);

    m_editorMap[filepath] = editor;

    editor->textEditor->loadHighlighter();

    showNewEditor(editor);
}

void Window::closeTab()
{
    const QString &filePath = m_titleBar->getActiveTabPath();
    const bool isBlankFile = QFileInfo(filePath).dir().absolutePath() == m_blankFileDir;

    // if the editor text changes, will prompt the user to save.
    if (m_editorMap[filePath]->textEditor->document()->isModified()) {
        DDialog *dialog = createSaveFileDialog(tr("Save file"), tr("Do you need to save the file?"));

        connect(dialog, &DDialog::buttonClicked, this,
                [=] (int index) {
                    dialog->hide();

                    // click the "dont't save" button.
                    if (index == 1) {
                        m_titleBar->closeActiveTab();

                        if (isBlankFile) {
                            // remove blank file.
                            QFile(filePath).remove();
                        }
                    }
                    // click the "save" button.
                    else if (index == 2) {
                        saveFile();
                        m_titleBar->closeActiveTab();
                    }

                    focusActiveEditor();
                    handleCloseFile(filePath);
                });

        dialog->exec();
    } else {
        // Record last close path.
        m_closeFileHistory << m_titleBar->getActiveTabPath();

        // Close tab directly, because all file is save automatically.
        m_titleBar->closeActiveTab();

        handleCloseFile(filePath);
        focusActiveEditor();
    }

    // if (QFileInfo(m_titleBar->getActiveTabPath()).dir().absolutePath() == m_blankFileDir) {
    //     QString content = getActiveEditor()->textEditor->toPlainText();

    //     // Don't save blank tab if nothing in it.
    //     if (content.size() == 0) {
    //         removeActiveBlankTab();
    //     } else {
    //         DDialog *dialog = createSaveFileDialog(tr("Save dragft"), tr("Do you need to save the draft?"));

    //         connect(dialog, &DDialog::buttonClicked, this,
    //                 [=] (int index) {
    //                     dialog->hide();

    //                     // Remove blank tab if user click "don't save" button.
    //                     if (index == 1) {
    //                         removeActiveBlankTab();
    //                         focusActiveEditor();
    //                     }
    //                     // Save blank tab as file and then remove blank tab if user click "save" button.
    //                     else if (index == 2) {
    //                         removeActiveBlankTab(true);
    //                         focusActiveEditor();
    //                     }
    //                 });
    //     }
    // } else if (QFileInfo(m_titleBar->getActiveTabPath()).dir().absolutePath() == m_readonlyFileDir) {
    //     QString realpath = QFileInfo(m_titleBar->getActiveTabPath()).fileName().replace(m_readonlySeparator, QDir().separator());

    //     QString editorContent = getActiveEditor()->textEditor->toPlainText();
    //     QString fileContent = Utils::getFileContent(realpath);

    //     if (editorContent == fileContent) {
    //         removeActiveReadonlyTab();
    //     } else {
    //         DDialog *dialog = createSaveFileDialog(tr("Save file"), tr("Do you need to save content to %1 ?").arg(realpath));

    //         connect(dialog, &DDialog::buttonClicked, this,
    //                 [=] (int index) {
    //                     dialog->hide();

    //                     // Remove tab if user click "don't save" button.
    //                     if (index == 1) {
    //                         removeActiveReadonlyTab();
    //                     }
    //                     // Save content to file and then remove tab if user click "save" button.
    //                     else if (index == 2) {
    //                         bool result = autoSaveDBus->saveFile(realpath, editorContent);

    //                         if (!result) {
    //                             qDebug() << tr("Save root file %1 failed").arg(realpath);
    //                         }

    //                         removeActiveReadonlyTab();
    //                     }
    //                 });
    //     }
    // } else {
    //     // Record last close path.
    //     m_closeFileHistory << m_titleBar->getActiveTabPath();

    //     // Close tab directly, because all file is save automatically.
    //     m_titleBar->closeActiveTab();

    //     focusActiveEditor();
    // }
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
    editor->textEditor->setThemeWithName(m_themeName);
    // editor->textEditor->setEnglishCompleter(m_settings->settings->option("advance.editor.enable_english_helper")->value().toBool());
    setFontSizeWithConfig(editor);
    editor->textEditor->setSettings(m_settings);
    editor->textEditor->setTabSpaceNumber(m_settings->settings->option("advance.editor.tab_space_number")->value().toInt());
    editor->textEditor->setFontFamily(m_settings->settings->option("base.font.family")->value().toString());
    // editor->textEditor->setEnglishWordsDB(m_wordsDB);

    connect(editor->textEditor, &TextEditor::clickFindAction, this, &Window::popupFindBar, Qt::QueuedConnection);
    connect(editor->textEditor, &TextEditor::clickReplaceAction, this, &Window::popupReplaceBar, Qt::QueuedConnection);
    connect(editor->textEditor, &TextEditor::clickJumpLineAction, this, &Window::popupJumpLineBar, Qt::QueuedConnection);
    connect(editor->textEditor, &TextEditor::clickFullscreenAction, this, &Window::toggleFullscreen, Qt::QueuedConnection);
    connect(editor->textEditor, &TextEditor::popupCompletionWindow, this, &Window::handlePopupCompletionWindow, Qt::QueuedConnection);

    connect(editor->textEditor, &TextEditor::selectNextCompletion, this, &Window::handleSelectNextCompletion, Qt::QueuedConnection);
    connect(editor->textEditor, &TextEditor::selectPrevCompletion, this, &Window::handleSelectPrevCompletion, Qt::QueuedConnection);
    connect(editor->textEditor, &TextEditor::selectFirstCompletion, this, &Window::handleSelectFirstCompletion, Qt::QueuedConnection);
    connect(editor->textEditor, &TextEditor::selectLastCompletion, this, &Window::handleSelectLastCompletion, Qt::QueuedConnection);
    connect(editor->textEditor, &TextEditor::confirmCompletion, this, &Window::handleConfirmCompletion, Qt::QueuedConnection);
    connect(editor->textEditor, &TextEditor::popupNotify, this, &Window::showNotify, Qt::QueuedConnection);
    // connect(editor->textEditor, &QPlainTextEdit::cursorPositionChanged, this, [=]() {
    //         if (!m_inCompleting && m_wordCompletionWindow->isVisible()) {
    //             m_wordCompletionWindow->hide();
    //         }
    //     });

    // connect(editor->textEditor, &TextEditor::click, this, [=]() {
    //         m_wordCompletionWindow->hide();
    //     });
    // connect(editor->textEditor, &TextEditor::pressEsc, this, [=]() {
    //         m_wordCompletionWindow->hide();
    //     });

    return editor;
}

Editor* Window::getActiveEditor()
{
    return m_editorMap[m_titleBar->getActiveTabPath()];
}

TextEditor* Window::getTextEditor(const QString &filepath)
{
    return m_editorMap[filepath]->textEditor;
}

void Window::focusActiveEditor()
{
    if (m_titleBar->getTabCount() > 0) {
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
    dialog.setDirectory(QFileInfo(m_titleBar->getActiveTabPath()).dir());
    dialog.selectFile(tr("Blank Document") + ".txt");
    dialog.setAcceptMode(QFileDialog::AcceptSave);
    dialog.addComboBox(tr("Encoding"), getEncodeList());
    dialog.addComboBox(tr("Line Endings"), QStringList() << "Linux" << "Windows" << "Mac OS");

    if (QFileInfo(m_titleBar->getActiveTabPath()).dir().absolutePath() != m_blankFileDir) {
        dialog.selectFile(QFileInfo(m_titleBar->getActiveTabPath()).fileName());
    }

    if (dialog.exec() == QDialog::Accepted) {
        encode = dialog.getComboBoxValue(tr("Encoding"));
        newline = dialog.getComboBoxValue(tr("Newline"));

        return dialog.selectedFiles().value(0);
    } else {
        return "";
    }
#else
    return QFileDialog::getSaveFileName(this, "Save File", QDir(QDir::homePath()).filePath("Blank Document.txt"));
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
    if (QFileInfo(m_titleBar->getActiveTabPath()).dir().absolutePath() == m_blankFileDir) {
        QString encode, newline;
        QString filepath = getSaveFilePath(encode, newline);

        if (filepath != "") {
            QString tabPath = m_titleBar->getActiveTabPath();

            saveFileAsAnotherPath(tabPath, filepath, encode, newline, true);

            return true;
        } else {
            return false;
        }
    } else if (QFileInfo(m_titleBar->getActiveTabPath()).dir().absolutePath() == m_readonlyFileDir) {
        QString realpath = QFileInfo(m_titleBar->getActiveTabPath()).fileName().replace(m_readonlySeparator, QDir().separator());
        QString content = getActiveEditor()->textEditor->toPlainText();

        bool result = autoSaveDBus->saveFile(realpath, content);

        if (!result) {
            qDebug() << QString("Save root file %1 failed").arg(realpath);
        }

        showNotify(tr("文件已保存"));

        return result;
    } else {
        bool success = m_editorMap[m_titleBar->getActiveTabPath()]->saveFile();
        if (success) {
            showNotify(tr("%1  文件已保存").arg(m_titleBar->getActiveTabName()));
        }

        return true;
    }
}

void Window::saveAsFile()
{
    QString encode, newline;
    QString filepath = getSaveFilePath(encode, newline);
    QString tabPath = m_titleBar->getActiveTabPath();

    if (filepath != "" && filepath != tabPath) {
        saveFileAsAnotherPath(tabPath, filepath, encode, newline, false);
    } else if (filepath == tabPath) {
        m_editorMap[filepath]->saveFile(encode, newline);
    }
}

void Window::saveFileAsAnotherPath(const QString &fromPath, const QString &toPath, const QString &encode, const QString &newline, bool deleteOldFile)
{
    if (deleteOldFile) {
        QFile(fromPath).remove();
    }

    m_titleBar->updateTabWithIndex(m_titleBar->getActiveTabIndex(), toPath, QFileInfo(toPath).fileName());

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
            QTimer::singleShot(0, m_editorMap[m_titleBar->getActiveTabPath()]->textEditor, SLOT(setFocus()));
        } else {
            m_findBar->focus();
        }
    } else {
        addBottomWidget(m_findBar);

        QString tabPath = m_titleBar->getActiveTabPath();
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
            QTimer::singleShot(0, m_editorMap[m_titleBar->getActiveTabPath()]->textEditor, SLOT(setFocus()));
        } else {
            m_replaceBar->focus();
        }
    } else {
        addBottomWidget(m_replaceBar);

        QString tabPath = m_titleBar->getActiveTabPath();
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
            QTimer::singleShot(0, m_editorMap[m_titleBar->getActiveTabPath()]->textEditor, SLOT(setFocus()));
        } else {
            m_jumpLineBar->focus();
        }
    } else {
        QString tabPath = m_titleBar->getActiveTabPath();
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
    foreach (int mib, QTextCodec::availableMibs()) {
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

    m_remberPositionFilePath = m_titleBar->getActiveTabPath();
    m_remberPositionRow = editor->textEditor->getCurrentLine();
    m_remberPositionColumn = editor->textEditor->getCurrentColumn();
    m_remberPositionScrollOffset = editor->textEditor->getScrollOffset();

    if (notify) {
        showNotify(tr("记住当前位置"));
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

            activeTab(m_titleBar->getTabIndex(filepath));

            QTimer::singleShot(
                0, this,
                [=] () {
                    m_editorMap[filepath]->textEditor->scrollToLine(scrollOffset, row, column);
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
                        m_editorMap[filepath]->textEditor->scrollToLine(scrollOffset, row, column);
                    });
            } else {
                showNotify(tr("记录位置的文件已经不存在了"));
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

        DAnchorsBase::setAnchor(m_themeBar, Qt::AnchorTop, m_centralWidget, Qt::AnchorTop);
        DAnchorsBase::setAnchor(m_themeBar, Qt::AnchorBottom, m_centralWidget, Qt::AnchorBottom);
        DAnchorsBase::setAnchor(m_themeBar, Qt::AnchorRight, m_centralWidget, Qt::AnchorRight);
    }
}

void Window::closeEvent(QCloseEvent *)
{
    QDir directory = QDir(QDir(QStandardPaths::standardLocations(QStandardPaths::DataLocation).first()).filePath("blank-files"));
    QStringList blankFiles = directory.entryList(QStringList(), QDir::Files);

    foreach(QString blankFile, blankFiles) {
        QString blankFilePath = QDir(directory).filePath(blankFile);

        QFile file(blankFilePath);
        if (!file.open(QFile::ReadOnly)) {
            break;
        }

        QTextStream in(&file);
        if (in.readAll().trimmed().size() == 0) {
            file.remove();

            qDebug() << QString("File %1 is empty, clean unused blank document.").arg(blankFilePath);
        }

        file.close();
    }

    emit close();
}

void Window::keyPressEvent(QKeyEvent *keyEvent)
{
    QString key = Utils::getKeyshortcut(keyEvent);

    // qDebug() << "!!!!!!!!!!!! " << key << Utils::getKeyshortcutFromKeymap(settings, "window", "selectnexttab");

    if (key == Utils::getKeyshortcutFromKeymap(m_settings, "window", "addblanktab")) {
        addBlankTab();
    } else if (key == Utils::getKeyshortcutFromKeymap(m_settings, "window", "newwindow")) {
        newWindow();
    } else if (key == Utils::getKeyshortcutFromKeymap(m_settings, "window", "savefile")) {
        saveFile();
    } else if (key == Utils::getKeyshortcutFromKeymap(m_settings, "window", "saveasfile")) {
        saveAsFile();
    } else if (key == Utils::getKeyshortcutFromKeymap(m_settings, "window", "selectnexttab")) {
        m_titleBar->selectNextTab();
    } else if (key == Utils::getKeyshortcutFromKeymap(m_settings, "window", "selectprevtab")) {
        m_titleBar->selectPrevTab();
    } else if (key == Utils::getKeyshortcutFromKeymap(m_settings, "window", "closetab")) {
        closeTab();
    } else if (key == Utils::getKeyshortcutFromKeymap(m_settings, "window", "restoretab")) {
        restoreTab();
    } else if (key == Utils::getKeyshortcutFromKeymap(m_settings, "window", "closeothertabs")) {
        m_titleBar->closeOtherTabs();
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
                if (m_titleBar->tabbar->count() > 1) {
                    activeTab(m_titleBar->tabbar->count() - 1);
                }
            } else {
                if (tabIndex <= m_titleBar->tabbar->count()) {
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
    for (int i = 0; i < m_titleBar->tabbar->tabFiles.size(); i++) {
        if (QFileInfo(m_titleBar->tabbar->tabFiles[i]).dir().absolutePath() == m_blankFileDir) {
            auto tabNameList = m_titleBar->tabbar->tabText(i).split("Blank document ");
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

    m_titleBar->addTab(blankTabPath, tr("Blank document %1").arg(blankFileIndex));
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
    const QString content = getTextEditor(filepath)->toPlainText();

    m_titleBar->closeTabWithIndex(index);
    handleCloseFile(filepath);

    emit dropTabOut(tabName, filepath, content);
}

void Window::handleTabCloseRequested(int index)
{
    activeTab(index);
    closeTab();
}

void Window::handleCloseFile(const QString &filepath)
{
    if (m_editorMap.contains(filepath)) {
        Editor *editor = m_editorMap[filepath];

        m_editorLayout->removeWidget(editor);
        m_editorMap.remove(filepath);

        editor->deleteLater();
    }

    // Exit window after close all tabs.
    if (m_editorMap.count() == 0) {
        close();

        deleteLater();
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

    const QString &filepath = m_titleBar->tabbar->tabFiles.value(index);

    if (m_editorMap.contains(filepath)) {
        m_editorLayout->setCurrentWidget(m_editorMap[filepath]);
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
        m_editorMap[file]->textEditor->scrollToLine(scrollOffset, row, column);

        QTimer::singleShot(0, m_editorMap[file]->textEditor, SLOT(setFocus()));
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
    if (file == m_titleBar->getActiveTabPath() && m_editorMap.contains(file)) {
        // Highlight keyword in text editor.
        m_editorMap[file]->textEditor->highlightKeyword(keyword, m_editorMap[file]->textEditor->getPosition());

        // Update input widget warning status along with keyword match situation.
        bool findKeyword = m_editorMap[file]->textEditor->findKeywordForward(keyword);
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
    QString blankFile = m_titleBar->getActiveTabPath();

    if (needSaveBefore) {
        if (!saveFile()) {
            // Do nothing if need save but last user not select save file anyway.
            return;
        }

        // Record last close path.
        m_closeFileHistory << m_titleBar->getActiveTabPath();
    }

    // Close current tab.
    m_titleBar->closeActiveTab();

    // Remove blank file from blank file directory.
    QFile(blankFile).remove();
}

void Window::removeActiveReadonlyTab()
{
    QString tabPath = m_titleBar->getActiveTabPath();
    QString realpath = QFileInfo(tabPath).fileName().replace(m_readonlySeparator, QDir().separator());

    m_closeFileHistory << realpath;
    m_titleBar->closeActiveTab();
    focusActiveEditor();

    QFile(tabPath).remove();
}

void Window::showNewEditor(Editor *editor)
{
    m_editorLayout->addWidget(editor);
    m_editorLayout->setCurrentWidget(editor);
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

void Window::popupThemeBar()
{
    // Select current theme.
    m_themeBar->themeView->clearSelections();
    QList<DSimpleListItem*> items;
    foreach (auto item, m_themeBar->items) {
        if ((static_cast<ThemeItem*>(item))->themeName == m_themeName) {
            items << item;
            break;
        }
    }
    m_themeBar->themeView->addSelections(items);

    // Popup theme bar.
    m_themeBar->popup();
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

void Window::changeTitlebarBackground(const QString &startColor, const QString &endColor)
{
    titlebar()->setStyleSheet(QString("%1"
                                      "Dtk--Widget--DTitlebar {"
                                      "background: qlineargradient(x1:0 y1:0, x2:0 y2:1,"
                                      "stop:0 rgba%2,  stop:1 rgba%3);"
                                      "}").arg(m_titlebarStyleSheet).arg(startColor).arg(endColor));

    QVariantMap jsonMap = Utils::getThemeNodeMap(m_themeName);
    m_titleBar->setTabActiveColor(jsonMap["app-colors"].toMap()["tab-active"].toString());
}

bool Window::wordCompletionWindowIsVisible()
{
    // return m_wordCompletionWindow->isVisible();
    return false;
}

void Window::handlePopupCompletionWindow(const QString &word, const QPoint &pos, const QStringList &words)
{
    // QStringList wordlist = words;
    // if (wordlist.size() >= 1 && word.size() > 2) {
    //     QPoint p = pos;
    //     // Adjust word completion window y coordinate if it out of screen.
    //     QScreen *screen = QGuiApplication::primaryScreen();
    //     QRect screenGeometry = screen->geometry();

    //     int wordCompletionWindowHeight = m_wordCompletionWindow->lineHeight * (wordlist.size() > 10? 10 : wordlist.size());

    //     if (pos.y() + wordCompletionWindowHeight > screenGeometry.height()) {
    //         p.setY(pos.y() - getActiveEditor()->textEditor->fontMetrics().height() - wordCompletionWindowHeight);
    //     }

    //     if (pos.x() + m_wordCompletionWindow->windowWidth > screenGeometry.width()) {
    //         p.setX(pos.x() - m_wordCompletionWindow->windowWidth);
    //     }

    //     m_wordCompletionWindow->move(p);
    //     m_wordCompletionWindow->show();

    //     // Add completion words.
    //     std::sort(wordlist.begin(), wordlist.end(), [=] (const QString &a, const QString &b) {
    //         return a < b;
    //     });

    //     m_wordCompletionWindow->addWords(wordlist);

    //     // Update theme to listview items.
    //     QVariantMap jsonMap = Utils::getThemeNodeMap(m_themeName);
    //     auto selectedBackgroundColor = jsonMap["app-colors"].toMap()["english-completer-item-selected-background"].toString();
    //     auto selectedTextColor = jsonMap["app-colors"].toMap()["english-completer-item-selected-text"].toString();
    //     auto normalBackgroundColor = jsonMap["app-colors"].toMap()["english-completer-item-normal-background"].toString();
    //     auto normalTextColor = jsonMap["app-colors"].toMap()["english-completer-item-normal-text"].toString();
    //     auto frameColor = jsonMap["app-colors"].toMap()["english-completer-window-frame"].toString();

    //     for (DSimpleListItem* item : m_wordCompletionWindow->items) {
    //         (static_cast<WordCompletionItem*>(item))->setColors(selectedBackgroundColor, selectedTextColor, normalBackgroundColor, normalTextColor);
    //     }
    //     m_wordCompletionWindow->listview->setFrame(true, QColor(frameColor), 0.3);
    //     m_wordCompletionWindow->listview->repaint();

    //     m_inCompleting = true;
    //     if (m_inCompletingTimer->isActive()) {
    //         m_inCompletingTimer->stop();
    //     }
    //     m_inCompletingTimer->start(2000);
    // } else {
    //     m_wordCompletionWindow->hide();
    // }
}

void Window::handleSelectNextCompletion()
{
    // if (m_wordCompletionWindow->isVisible()) {
    //     m_wordCompletionWindow->listview->selectNextItem();
    // } else {
    //     if (!getActiveEditor()->textEditor->getEnglishCompleter()) {
    //         showNotify(tr("请先开启英语助手"));
    //     }
    // }
}

void Window::handleSelectPrevCompletion()
{
    // if (m_wordCompletionWindow->isVisible()) {
    //     m_wordCompletionWindow->listview->selectPrevItem();
    // } else {
    //     if (!getActiveEditor()->textEditor->getEnglishCompleter()) {
    //         showNotify(tr("请先开启英语助手"));
    //     }
    // }
}

void Window::handleSelectFirstCompletion()
{
    // if (m_wordCompletionWindow->isVisible()) {
    //     m_wordCompletionWindow->listview->selectFirstItem();
    // } else {
    //     if (!getActiveEditor()->textEditor->getEnglishCompleter()) {
    //         showNotify(tr("请先开启英语助手"));
    //     }
    // }
}

void Window::handleSelectLastCompletion()
{
    // if (m_wordCompletionWindow->isVisible()) {
    //     m_wordCompletionWindow->listview->selectLastItem();
    // } else {
    //     if (!getActiveEditor()->textEditor->getEnglishCompleter()) {
    //         showNotify(tr("请先开启英语助手"));
    //     }
    // }
}

void Window::handleConfirmCompletion()
{
    // if (m_wordCompletionWindow->isVisible()) {
    //     auto completionItems = m_wordCompletionWindow->listview->getSelections();

    //     for (auto item : completionItems) {
    //         WordCompletionItem* wordItem = static_cast<WordCompletionItem*>(item);
    //         m_wordCompletionWindow->hide();

    //         getActiveEditor()->textEditor->completionWord(wordItem->name);

    //         break;
    //     }
    // }
}

void Window::loadTheme(const QString &name)
{
    m_themeName = name;

    QVariantMap jsonMap = Utils::getThemeNodeMap(name);
    const QString &backgroundColor = jsonMap["editor-colors"].toMap()["background-color"].toString();

    if (QColor(backgroundColor).lightness() < 128) {
        DThemeManager::instance()->setTheme("dark");
        m_titleBar->tabbar->setBackground(m_darkTabBackgroundStartColor, m_darkTabBackgroundEndColor);
        changeTitlebarBackground(m_darkTabBackgroundStartColor, m_darkTabBackgroundEndColor);
    } else {
        DThemeManager::instance()->setTheme("light");
        m_titleBar->tabbar->setBackground(m_lightTabBackgroundStartColor, m_lightTabBackgroundEndColor);
        changeTitlebarBackground(m_lightTabBackgroundStartColor, m_lightTabBackgroundEndColor);
    }

    for (auto editor : m_editorMap.values()) {
        editor->textEditor->setThemeWithName(name);
    }

    m_themeBar->setBackground(backgroundColor);
    m_jumpLineBar->setBackground(backgroundColor);
    m_replaceBar->setBackground(backgroundColor);
    m_findBar->setBackground(backgroundColor);
    m_titleBar->tabbar->setDNDColor(jsonMap["app-colors"].toMap()["tab-dnd-start"].toString(), jsonMap["app-colors"].toMap()["tab-dnd-end"].toString());

    auto frameSelectedColor = jsonMap["app-colors"].toMap()["themebar-frame-selected"].toString();
    auto frameNormalColor = jsonMap["app-colors"].toMap()["themebar-frame-normal"].toString();
    for (DSimpleListItem* item : m_themeBar->items) {
        (static_cast<ThemeItem*>(item))->setFrameColor(frameSelectedColor, frameNormalColor);
    }
    m_themeBar->themeView->repaint();

    m_settings->settings->option("base.theme.default")->setValue(name);
}

void Window::dragEnterEvent(QDragEnterEvent *event)
{
    // Accept drag event if mime type is url.
//    if (event->mimeData()->hasUrls()) {
//        event->acceptProposedAction();
//    }

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
        if (wordCompletionWindowIsVisible()) {
            // m_wordCompletionWindow->hide();
        }
    }

    return false;
}

void Window::addBlankTab()
{
    addBlankTab("");
}
