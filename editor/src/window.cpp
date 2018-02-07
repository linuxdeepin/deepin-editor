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
#include "dsettingsdialog.h"
#include "dthememanager.h"
#include "dtoast.h"
#include "utils.h"
#include "window.h"

#include <DSettings>
#include <DSettingsOption>
#include <DTitlebar>
#include <QApplication>
#include <QDateTime>
#include <QDebug>
#include <QDir>
#include <QFileDialog>
#include <QLabel>
#include <QPrintDialog>
#include <QPrintPreviewDialog>
#include <QPrinter>
#include <QScreen>
#include <QStyleFactory>
#include <QSvgWidget>
#include <QTemporaryFile>
#include <qsettingbackend.h>

DTK_USE_NAMESPACE
DWIDGET_USE_NAMESPACE

Window::Window(DMainWindow *parent) : DMainWindow(parent)
{
    // Init theme.
    DThemeManager::instance()->setTheme("dark");

    // Init.
    installEventFilter(this);   // add event filter
    blankFileDir = QDir(QStandardPaths::standardLocations(QStandardPaths::DataLocation).first()).filePath("blank-files");

    // Init settings.
    settings = new Settings();
    settings->init();

    // Init layout and editor.
    layoutWidget = new QWidget();
    this->setCentralWidget(layoutWidget);

    layout = new QVBoxLayout(layoutWidget);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->setSpacing(0);

    editorWidget = new QWidget();
    editorLayout = new QStackedLayout(editorWidget);
    editorLayout->setContentsMargins(0, 0, 0, 0);

    layout->addWidget(editorWidget);

    // Init titlebar.
    if (this->titlebar()) {
        // Init tabbar.
        tabbar = new Tabbar();
        this->titlebar()->setCustomWidget(tabbar, Qt::AlignVCenter, false);
        this->titlebar()->setSeparatorVisible(true);
        this->titlebar()->setAutoHideOnFullscreen(true);

        connect(tabbar, &Tabbar::doubleClicked, this->titlebar(), &DTitlebar::doubleClicked, Qt::QueuedConnection);
        connect(tabbar, &Tabbar::switchToFile, this, &Window::handleSwitchToFile, Qt::QueuedConnection);
        connect(tabbar, &Tabbar::closeFile, this, &Window::handleCloseFile, Qt::QueuedConnection);
        connect(tabbar, &Tabbar::tabAddRequested, this,
                [=]() {
                    addBlankTab();
                }, Qt::QueuedConnection);
        connect(tabbar, &Tabbar::tabReleaseRequested, this, &Window::handleTabReleaseRequested, Qt::QueuedConnection);
        
        menu = new QMenu();
        menu->setStyle(QStyleFactory::create("dlight"));
        
        // Init main menu.
        newWindowAction = new QAction("New window", this);
        newTabAction = new QAction("New tab", this);
        openFileAction = new QAction("Open file", this);
        saveAction = new QAction("Save", this);
        saveAsAction = new QAction("Save as", this);
        printAction = new QAction("Print", this);
        switchThemeAction = new QAction("Switch theme", this);
        settingAction = new QAction("Setting", this);
        
        menu->addAction(newWindowAction);
        menu->addAction(newTabAction);
        menu->addAction(openFileAction);
        menu->addSeparator();
        menu->addAction(saveAction);
        menu->addAction(saveAsAction);
        menu->addAction(printAction);
        menu->addAction(switchThemeAction);
        menu->addSeparator();
        menu->addAction(settingAction);
        this->titlebar()->setMenu(menu);
        
        connect(newWindowAction, &QAction::triggered, this, &Window::newWindow);
        connect(newTabAction, &QAction::triggered, this,
                [=] () {
                    addBlankTab();
                });
        connect(openFileAction, &QAction::triggered, this, &Window::openFile);
        connect(saveAction, &QAction::triggered, this, &Window::saveFile);
        connect(saveAsAction, &QAction::triggered, this, &Window::saveAsFile);
        connect(printAction, &QAction::triggered, this, &Window::popupPrintDialog);
        connect(settingAction, &QAction::triggered, this, &Window::popupSettingDialog);
    }

    // Init find bar.
    findBar = new FindBar();

    connect(findBar, &FindBar::backToPosition, this, &Window::handleBackToPosition, Qt::QueuedConnection);
    connect(findBar, &FindBar::findNext, this, &Window::handleFindNext, Qt::QueuedConnection);
    connect(findBar, &FindBar::findPrev, this, &Window::handleFindPrev, Qt::QueuedConnection);
    connect(findBar, &FindBar::removeSearchKeyword, this, &Window::handleRemoveSearchKeyword, Qt::QueuedConnection);
    connect(findBar, &FindBar::updateSearchKeyword, this, &Window::handleUpdateSearchKeyword, Qt::QueuedConnection);

    // Init replace bar.
    replaceBar = new ReplaceBar();
    connect(replaceBar, &ReplaceBar::backToPosition, this, &Window::handleBackToPosition, Qt::QueuedConnection);
    connect(replaceBar, &ReplaceBar::removeSearchKeyword, this, &Window::handleRemoveSearchKeyword, Qt::QueuedConnection);
    connect(replaceBar, &ReplaceBar::replaceAll, this, &Window::handleReplaceAll, Qt::QueuedConnection);
    connect(replaceBar, &ReplaceBar::replaceNext, this, &Window::handleReplaceNext, Qt::QueuedConnection);
    connect(replaceBar, &ReplaceBar::replaceRest, this, &Window::handleReplaceRest, Qt::QueuedConnection);
    connect(replaceBar, &ReplaceBar::replaceSkip, this, &Window::handleReplaceSkip, Qt::QueuedConnection);
    connect(replaceBar, &ReplaceBar::updateSearchKeyword, this, &Window::handleUpdateSearchKeyword, Qt::QueuedConnection);

    // Init jump line bar.
    jumpLineBar = new JumpLineBar(this);
    QTimer::singleShot(0, jumpLineBar, SLOT(hide()));

    connect(jumpLineBar, &JumpLineBar::jumpToLine, this, &Window::handleJumpLineBarJumpToLine, Qt::QueuedConnection);
    connect(jumpLineBar, &JumpLineBar::backToPosition, this, &Window::handleBackToPosition, Qt::QueuedConnection);
    connect(jumpLineBar, &JumpLineBar::lostFocusExit, this, &Window::handleJumpLineBarExit, Qt::QueuedConnection);

    // Make jump line bar pop at top-right of editor.
    DAnchorsBase::setAnchor(jumpLineBar, Qt::AnchorTop, layoutWidget, Qt::AnchorTop);
    DAnchorsBase::setAnchor(jumpLineBar, Qt::AnchorRight, layoutWidget, Qt::AnchorRight);
    
    // Apply qss theme.
    Utils::applyQss(this, "main.qss");
    Utils::applyQss(this->titlebar(), "main.qss");
}

Window::~Window()
{
    // We don't need clean pointers because application has exit here.
}

int Window::getTabIndex(QString file)
{
    return tabbar->getTabIndex(file);
}


void Window::activeTab(int index)
{
    activateWindow();
    tabbar->activeTabWithIndex(index);
}

void Window::addTab(QString file)
{
    if (tabbar->getTabIndex(file) == -1) {
        tabbar->addTab(file, QFileInfo(file).fileName());

        if (!editorMap.contains(file)) {
            Editor *editor = createEditor();
            editor->loadFile(file);

            editorMap[file] = editor;

            showNewEditor(editor);
        }
    }

    activateWindow();
}

void Window::addTabWithContent(QString tabName, QString filepath, QString content)
{
    tabbar->addTab(filepath, tabName);

    Editor *editor = createEditor();
    editor->updatePath(filepath);
    editor->textEditor->setPlainText(content);

    editorMap[filepath] = editor;

    showNewEditor(editor);
}

void Window::closeTab()
{
    if (QFileInfo(tabbar->getActiveTabPath()).dir().absolutePath() == blankFileDir) {
        QString content = getActiveEditor()->textEditor->toPlainText();

        // Don't save blank tab if nothing in it.
        if (content.size() == 0) {
            removeActiveBlankTab();
        } else {
            DDialog *dialog = createSaveBlankFileDialog();

            connect(dialog, &DDialog::buttonClicked, this,
                    [=] (int index) {
                        dialog->hide();

                        // Remove blank tab if user click "don't save" button.
                        if (index == 1) {
                            removeActiveBlankTab();
                        }
                        // Save blank tab as file and then remove blank tab if user click "save" button.
                        else if (index == 2) {
                            removeActiveBlankTab(true);
                        }
                    });
        }
    } else {
        // Close tab directly, because all file is save automatically.
        tabbar->closeActiveTab();
    }
}

Editor* Window::createEditor()
{
    Editor *editor = new Editor();
    setFontSizeWithConfig(editor);
    
    connect(editor->textEditor, &TextEditor::clickFindAction, this, &Window::popupFindBar, Qt::QueuedConnection);
    connect(editor->textEditor, &TextEditor::clickReplaceAction, this, &Window::popupReplaceBar, Qt::QueuedConnection);
    connect(editor->textEditor, &TextEditor::clickJumpLineAction, this, &Window::popupJumpLineBar, Qt::QueuedConnection);
    connect(editor->textEditor, &TextEditor::clickFullscreenAction, this, &Window::toggleFullscreen, Qt::QueuedConnection);

    return editor;
}

Editor* Window::getActiveEditor()
{
    return editorMap[tabbar->getActiveTabPath()];
}

TextEditor* Window::getTextEditor(QString filepath)
{
    return editorMap[filepath]->textEditor;
}

void Window::openFile()
{
    QFileDialog dialog(0, QString(), QDir::homePath());
    dialog.setFileMode(QFileDialog::AnyFile);
    dialog.setAcceptMode(QFileDialog::AcceptOpen);

    if (dialog.exec()) {
        foreach (QString file, dialog.selectedFiles()) {
            addTab(file);
        }
    }
}

bool Window::saveFile()
{
    if (QFileInfo(tabbar->getActiveTabPath()).dir().absolutePath() == blankFileDir) {
        QString filepath = QFileDialog::getSaveFileName(this, "Save File", QDir::homePath());

        if (filepath != "") {
            QString tabPath = tabbar->getActiveTabPath();

            saveFileAsAnotherPath(tabPath, filepath, true);
            
            return true;
        } else {
            return false;
        }
    } else {
        auto toast = new DToast(this);

        toast->setText("文件已自动保存");
        toast->setIcon(QIcon(Utils::getQrcPath("logo_24.svg")));
        toast->pop();

        toast->move((width() - toast->width()) / 2,
                    height() - toast->height() - toastPaddingBottom);

        return true;
    }
}

void Window::saveAsFile()
{
    QString filepath = QFileDialog::getSaveFileName(this, "Save File", QDir::homePath());
    QString tabPath = tabbar->getActiveTabPath();

    if (filepath != "" && filepath != tabPath) {
        saveFileAsAnotherPath(tabPath, filepath);
    }
}

void Window::saveFileAsAnotherPath(QString fromPath, QString toPath, bool deleteOldFile)
{
    if (deleteOldFile) {
        QFile(fromPath).remove();
    }

    tabbar->updateTabWithIndex(tabbar->getActiveTabIndex(), toPath, QFileInfo(toPath).fileName());

    editorMap[toPath] = editorMap.take(fromPath);

    editorMap[toPath]->updatePath(toPath);
    editorMap[toPath]->saveFile();
    
    getActiveEditor()->textEditor->loadHighlighter();
}

void Window::decrementFontSize()
{
    foreach (Editor *editor, editorMap.values()) {
        int size = std::max(fontSize - 1, settings->minFontSize);
        editor->textEditor->setFontSize(size);
        saveFontSize(size);
    }
}

void Window::incrementFontSize()
{
    foreach (Editor *editor, editorMap.values()) {
        int size = std::min(fontSize + 1, settings->maxFontSize);
        editor->textEditor->setFontSize(size);
        saveFontSize(size);
    }
}

void Window::resetFontSize()
{
    foreach (Editor *editor, editorMap.values()) {
        editor->textEditor->setFontSize(settings->defaultFontSize);
        saveFontSize(settings->defaultFontSize);
    }
}

void Window::saveFontSize(int size)
{
    fontSize = size;

    settings->setOption("default_font_size", fontSize);
}

void Window::setFontSizeWithConfig(Editor *editor)
{
    int size =  settings->getOption("default_font_size").toInt();
    editor->textEditor->setFontSize(size);

    fontSize = size;
}

void Window::popupFindBar()
{
    if (findBar->isVisible()) {
        if (findBar->isFocus()) {
            QTimer::singleShot(0, editorMap[tabbar->getActiveTabPath()]->textEditor, SLOT(setFocus()));
        } else {
            findBar->focus();
        }
    } else {
        addBottomWidget(findBar);

        QString tabPath = tabbar->getActiveTabPath();
        Editor *editor = getActiveEditor();
        QString text = editor->textEditor->textCursor().selectedText();
        int row = editor->textEditor->getCurrentLine();
        int column = editor->textEditor->getCurrentColumn();
        int scrollOffset = editor->textEditor->getScrollOffset();

        findBar->activeInput(text, tabPath, row, column, scrollOffset);
    }
}

void Window::popupReplaceBar()
{
    if (replaceBar->isVisible()) {
        if (replaceBar->isFocus()) {
            QTimer::singleShot(0, editorMap[tabbar->getActiveTabPath()]->textEditor, SLOT(setFocus()));
        } else {
            replaceBar->focus();
        }
    } else {
        addBottomWidget(replaceBar);

        QString tabPath = tabbar->getActiveTabPath();
        Editor *editor = getActiveEditor();
        QString text = editor->textEditor->textCursor().selectedText();
        int row = editor->textEditor->getCurrentLine();
        int column = editor->textEditor->getCurrentColumn();
        int scrollOffset = editor->textEditor->getScrollOffset();

        replaceBar->activeInput(text, tabPath, row, column, scrollOffset);
    }
}

void Window::popupJumpLineBar()
{
    if (jumpLineBar->isVisible()) {
        if (jumpLineBar->isFocus()) {
            QTimer::singleShot(0, editorMap[tabbar->getActiveTabPath()]->textEditor, SLOT(setFocus()));
        } else {
            jumpLineBar->focus();
        }
    } else {
        QString tabPath = tabbar->getActiveTabPath();
        Editor *editor = getActiveEditor();
        QString text = editor->textEditor->textCursor().selectedText();
        int row = editor->textEditor->getCurrentLine();
        int column = editor->textEditor->getCurrentColumn();
        int count = editor->textEditor->blockCount();
        int scrollOffset = editor->textEditor->getScrollOffset();

        jumpLineBar->activeInput(tabPath, row, column, count, scrollOffset);
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

void Window::keyPressEvent(QKeyEvent *keyEvent)
{
    QString key = Utils::getKeymap(keyEvent);

    // qDebug() << key;

    if (key == "Ctrl + T") {
        addBlankTab();
    } else if (key == "Ctrl + S") {
        saveFile();
    } else if (key == "Ctrl + Shift + S") {
        saveAsFile();
    } else if (key == "Ctrl + Tab") {
        tabbar->selectNextTab();
    } else if (key == "Ctrl + Shift + Backtab") {
        tabbar->selectPrevTab();
    } else if (key == "Ctrl + W") {
        closeTab();
    } else if (key == "Ctrl + Shift + W") {
        tabbar->closeOtherTabs();
    } else if (key == "Ctrl + O") {
        openFile();
    } else if (key == "Ctrl + =") {
        incrementFontSize();
    } else if (key == "Ctrl + -") {
        decrementFontSize();
    } else if (key == "Ctrl + 0") {
        resetFontSize();
    } else if (key == "F11") {
        toggleFullscreen();
    } else if (key == "Ctrl + Shift + F") {
        popupFindBar();
    } else if (key == "Ctrl + Shift + H") {
        popupReplaceBar();
    } else if (key == "Ctrl + Shift + G") {
        popupJumpLineBar();
    } else if (key == "Esc") {
        removeBottomWidget();
    }
}

void Window::addBlankTab(QString blankFile)
{
    QString blankTabPath;
    if (blankFile == "") {
        blankTabPath = QDir(blankFileDir).filePath(QString("blank_file_%1").arg(QDateTime::currentDateTime().toString("yyyy-MM-dd_hh-mm-ss")));
        if (!Utils::fileExists(blankTabPath)) {
            QDir().mkpath(blankFileDir);
            if (QFile(blankTabPath).open(QIODevice::ReadWrite)) {
                qDebug() << "Create blank file: " << blankTabPath;
            } else {
                qDebug() << "Can't create blank file: " << blankTabPath;
            }
        }
    } else {
        blankTabPath = blankFile;
    }

    tabbar->addTab(blankTabPath, "Blank document");
    Editor *editor = createEditor();
    editor->updatePath(blankTabPath);

    if (blankFile != "" && Utils::fileExists(blankFile)) {
        editor->loadFile(blankFile);
    }

    editorMap[blankTabPath] = editor;

    showNewEditor(editor);
}

void Window::handleTabReleaseRequested(QString tabName, QString filepath, int index)
{
    tabbar->closeTabWithIndex(index);

    QString content = getTextEditor(filepath)->toPlainText();
    dropTabOut(tabName, filepath, content);
}

void Window::handleCloseFile(QString filepath)
{
    if (editorMap.contains(filepath)) {
        Editor *editor = editorMap[filepath];

        editorLayout->removeWidget(editor);
        editorMap.remove(filepath);

        editor->deleteLater();
    }

    // Exit window after close all tabs.
    if (editorMap.count() == 0) {
        deleteLater();
    }
}

void Window::handleSwitchToFile(QString filepath)
{
    if (editorMap.contains(filepath)) {
        editorLayout->setCurrentWidget(editorMap[filepath]);
    }
}

void Window::handleJumpLineBarExit()
{
    QTimer::singleShot(0, getActiveEditor()->textEditor, SLOT(setFocus()));
}

void Window::handleJumpLineBarJumpToLine(QString filepath, int line, bool focusEditor)
{
    if (editorMap.contains(filepath)) {
        getTextEditor(filepath)->jumpToLine(line, true);

        if (focusEditor) {
            QTimer::singleShot(0, getTextEditor(filepath), SLOT(setFocus()));
        }
    }
}

void Window::handleBackToPosition(QString file, int row, int column, int scrollOffset)
{
    if (editorMap.contains(file)) {
        editorMap[file]->textEditor->scrollToLine(scrollOffset, row, column);

        QTimer::singleShot(0, editorMap[file]->textEditor, SLOT(setFocus()));
    }
}

void Window::handleFindNext()
{
    Editor *editor = getActiveEditor();

    editor->textEditor->updateCursorKeywordSelection(editor->textEditor->getPosition(), true);
    editor->textEditor->renderAllSelections();
}

void Window::handleFindPrev()
{
    Editor *editor = getActiveEditor();

    editor->textEditor->updateCursorKeywordSelection(editor->textEditor->getPosition(), false);
    editor->textEditor->renderAllSelections();
}

void Window::handleReplaceAll(QString replaceText, QString withText)
{
    Editor *editor = getActiveEditor();

    editor->textEditor->replaceAll(replaceText, withText);
}

void Window::handleReplaceNext(QString replaceText, QString withText)
{
    Editor *editor = getActiveEditor();

    editor->textEditor->replaceNext(replaceText, withText);
}

void Window::handleReplaceRest(QString replaceText, QString withText)
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

void Window::handleUpdateSearchKeyword(QString file, QString keyword)
{
    if (file == tabbar->getActiveTabPath() && editorMap.contains(file)) {
        editorMap[file]->textEditor->highlightKeyword(keyword, editorMap[file]->textEditor->getPosition());
    }
}

void Window::addBottomWidget(QWidget *widget)
{
    if (layout->count() >= 2) {
        removeBottomWidget();
    }

    layout->addWidget(widget);
}

void Window::removeBottomWidget()
{
    QWidget *widget = layout->takeAt(1)->widget();
    widget->hide();
}

void Window::removeActiveBlankTab(bool needSaveBefore)
{
    QString blankFile = tabbar->getActiveTabPath();

    if (needSaveBefore) {
        if (!saveFile()) {
            // Do nothing if need save but last user not select save file anyway.
            return;
        }
    }

    // Close current tab.
    tabbar->closeActiveTab();

    // Remove blank file from blank file directory.
    QFile(blankFile).remove();
}

void Window::showNewEditor(Editor *editor)
{
    editorLayout->addWidget(editor);
    editorLayout->setCurrentWidget(editor);
}

DDialog* Window::createSaveBlankFileDialog()
{
    DDialog *dialog = new DDialog("Save dragft", "Do you need to save the draft?", this);
    dialog->setWindowFlags(dialog->windowFlags() | Qt::WindowStaysOnTopHint);
    dialog->setIcon(QIcon(Utils::getQrcPath("logo_48.svg")));
    dialog->addButton(QString(tr("Cancel")), false, DDialog::ButtonNormal);
    dialog->addButton(QString(tr("Don't Save")), false, DDialog::ButtonNormal);
    dialog->addButton(QString(tr("Save")), true, DDialog::ButtonNormal);
    dialog->show();

    return dialog;
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

void Window::popupSettingDialog()
{
    auto backend = new Dtk::Core::QSettingBackend(QDir(QDir(QDir(QStandardPaths::standardLocations(QStandardPaths::ConfigLocation).first()).filePath(qApp->organizationName())).filePath(qApp->applicationName())).filePath("config.conf"));

    auto settings = Dtk::Core::DSettings::fromJsonFile(":/resource/settings.json");
    settings->setBackend(backend);
    
    connect(settings, &DSettings::valueChanged, this, 
            [=] (const QString &key, const QVariant &value) {
                qDebug() << key << value;
                settings->sync();
            });
    
    QFontDatabase fontDatabase;
    auto fontFamliy = settings->option("base.font.family");
    fontFamliy->setData("items", fontDatabase.families());
    fontFamliy->setValue(0);
    
    auto fontSize = settings->option("base.font.size");
    fontSize->setValue(12);
    
    auto keymap = settings->option("shortcuts.keymap.keymap");
    keymap->setData("items", QStringList() << "Standard" << "Emacs" << "Customize");
    
    auto windowSate = settings->option("advance.window.window_state");
    windowSate->setData("items", QStringList() << "Window Normal" << "Window Maximum" << "Fullscreen");
    
    auto tabSpace = settings->option("advance.editor.tab_space_number");
    tabSpace->setValue(4);

    DSettingsDialog dsd(this);
    dsd.setProperty("_d_dtk_theme", "light");
    dsd.setProperty("_d_QSSFilename", "DSettingsDialog");
    DThemeManager::instance()->registerWidget(&dsd);    
    dsd.updateSettings(settings);
    dtkThemeWorkaround(&dsd, "dlight");
    dsd.exec();
}

// This function is workaround, it will remove after DTK fixed SettingDialog theme bug.
void Window::dtkThemeWorkaround(QWidget *parent, const QString &theme)
{
    parent->setStyle(QStyleFactory::create(theme));
    for (auto obj : parent->children()) {
        
        auto w = qobject_cast<QWidget *>(obj);
        if (!w) {
            continue;
        }
        
        dtkThemeWorkaround(w, theme);
    }
}
