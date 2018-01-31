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

#include "window.h"
#include <DTitlebar>
#include <QLabel>
#include "dtoast.h"
#include <QDebug>
#include <QFileDialog>
#include <QScreen>
#include <QDir>
#include <QDateTime>
#include <QApplication>
#include <QSvgWidget>
#include "dthememanager.h"
#include "utils.h"
#include "danchors.h"

DWIDGET_USE_NAMESPACE

Window::Window(DMainWindow *parent) : DMainWindow(parent)
{
    DThemeManager::instance()->setTheme("dark");

    installEventFilter(this);   // add event filter

    layoutWidget = new QWidget();
    
    layout = new QVBoxLayout(layoutWidget);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->setSpacing(0);
    
    editorWidget = new QWidget();
    editorLayout = new QStackedLayout(editorWidget);
    editorLayout->setContentsMargins(0, 0, 0, 0);

    this->setCentralWidget(layoutWidget);
    
    layout->addWidget(editorWidget);
    
    findBar = new FindBar();
    
    connect(findBar, &FindBar::backToPosition, this, &Window::handleBackToPosition, Qt::QueuedConnection);
    connect(findBar, &FindBar::updateSearchKeyword, this, &Window::handleUpdateSearchKeyword, Qt::QueuedConnection);
    
    settings = new Settings();
    settings->init();

    tabbar = new Tabbar();
    
    jumpLineBar = new JumpLineBar(this);
    QTimer::singleShot(0, jumpLineBar, SLOT(hide()));
    
    connect(jumpLineBar, &JumpLineBar::jumpToLine, this, &Window::handleJumpToLine, Qt::QueuedConnection);
    connect(jumpLineBar, &JumpLineBar::tempJumpToLine, this, &Window::handleTempJumpToLine, Qt::QueuedConnection);
    connect(jumpLineBar, &JumpLineBar::backToLine, this, &Window::handleBackToLine, Qt::QueuedConnection);
    connect(jumpLineBar, &JumpLineBar::cancelJump, this, &Window::handleCancelJump, Qt::QueuedConnection);
    
    DAnchorsBase::setAnchor(jumpLineBar, Qt::AnchorTop, layoutWidget, Qt::AnchorTop);    
    DAnchorsBase::setAnchor(jumpLineBar, Qt::AnchorRight, layoutWidget, Qt::AnchorRight);    

    this->titlebar()->setCustomWidget(tabbar, Qt::AlignVCenter, false);
    this->titlebar()->setSeparatorVisible(true);
    this->titlebar()->setAutoHideOnFullscreen(true);

    connect(tabbar, &Tabbar::doubleClicked, this->titlebar(), &DTitlebar::doubleClicked, Qt::QueuedConnection);
    connect(tabbar, &Tabbar::switchToFile, this, &Window::handleSwitchToFile, Qt::QueuedConnection);
    connect(tabbar, &Tabbar::closeFile, this, &Window::handleCloseFile, Qt::QueuedConnection);
    connect(tabbar, &Tabbar::tabAddRequested, this, &Window::addBlankTab, Qt::QueuedConnection);
    connect(tabbar, &Tabbar::tabReleaseRequested, this, &Window::handleTabReleaseRequested, Qt::QueuedConnection);

    Utils::applyQss(this, "main.qss");
    Utils::applyQss(this->titlebar(), "main.qss");
}

Window::~Window()
{
    // We don't need clean pointers because application has exit here.
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
        tabbar->closeTab();
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
    }
}

int Window::isFileInTabs(QString file)
{
    return tabbar->isTabExist(file);
}

void Window::addTab(QString file)
{
    if (tabbar->isTabExist(file) == -1) {
        tabbar->addTab(file, QFileInfo(file).fileName());

        if (!editorMap.contains(file)) {
            Editor *editor = createEditor();
            editor->loadFile(file);

            editorMap[file] = editor;

            editorLayout->addWidget(editor);
            editorLayout->setCurrentWidget(editor);
        }
    }

    activateWindow();
}

void Window::addBlankTab()
{
    QString blankTabPath = QString("Blank Tab: %1").arg(QDateTime::currentDateTime().toString("yyyy-MM-dd hh:mm:ss")) ;

    tabbar->addTab(blankTabPath, "Blank document");
    Editor *editor = createEditor();
    editor->updatePath(blankTabPath);

    editorMap[blankTabPath] = editor;

    editorLayout->addWidget(editor);
    editorLayout->setCurrentWidget(editor);
}

void Window::handleSwitchToFile(QString filepath)
{
    if (editorMap.contains(filepath)) {
        editorLayout->setCurrentWidget(editorMap[filepath]);
    }
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

void Window::activeTab(int index)
{
    activateWindow();
    tabbar->activeTab(index);
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

void Window::saveFile()
{
    if (tabbar->getActiveTabName() == "Blank document") {
        QString filepath = QFileDialog::getSaveFileName(this, "Save File", QDir::homePath());

        if (filepath != "") {
            QString tabPath = tabbar->getActiveTabPath();

            saveFileAsAnotherPath(tabPath, filepath);
        }
    } else {
        auto toast = new DToast(this);

        toast->setText("文件已自动保存");
        toast->setIcon(QIcon(Utils::getQrcPath("logo_24.svg")));
        toast->pop();

        toast->move((width() - toast->width()) / 2,
                    height() - toast->height() - notifyPadding);
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

void Window::toggleFullscreen()
{
    if (isFullScreen()) {
        showNormal();
    }  else {
        showFullScreen();

        QScreen *screen = QGuiApplication::primaryScreen();
        QRect screenGeometry = screen->geometry();

        auto toast = new DToast(this);

        toast->setText("按F11或Esc退出全屏");
        toast->setIcon(QIcon(Utils::getQrcPath("logo_24.svg")));
        toast->pop();

        toast->move((screenGeometry.width() - toast->width()) / 2, notifyPadding);
    }

}

Editor* Window::getActiveEditor()
{
    QString tabPath = tabbar->getActiveTabPath();

    return editorMap[tabPath];
}

void Window::saveFileAsAnotherPath(QString fromPath, QString toPath)
{
    tabbar->updateTab(tabbar->currentIndex(), toPath, QFileInfo(toPath).fileName());

    editorMap[toPath] = editorMap.take(fromPath);

    editorMap[toPath]->updatePath(toPath);
    editorMap[toPath]->saveFile();
}

void Window::handleTabReleaseRequested(QString tabName, QString filepath, int index)
{
    tabbar->closeTabWithIndex(index);

    QString content = editorMap[filepath]->textEditor->toPlainText();
    popTab(tabName, filepath, content);
}

void Window::addTabWithContent(QString tabName, QString filepath, QString content)
{
    tabbar->addTab(filepath, tabName);

    Editor *editor = createEditor();
    editor->updatePath(filepath);
    editor->textEditor->setPlainText(content);

    editorMap[filepath] = editor;

    editorLayout->addWidget(editor);
    editorLayout->setCurrentWidget(editor);
}

TextEditor* Window::getTextEditor(QString filepath)
{
    return editorMap[filepath]->textEditor;
}

Editor* Window::createEditor()
{
    Editor *editor = new Editor();
    setFontSizeWithConfig(editor);

    connect(editor, &Editor::jumpLine, this, &Window::handleJumpLine, Qt::QueuedConnection);
    
    return editor;
}

void Window::handleJumpLine(QString filepath, int line, int lineCount, int scrollOffset)
{
    jumpLineBar->activeInput(filepath, line, lineCount, scrollOffset);
}

void Window::handleBackToLine(QString filepath, int line, int scrollOffset)
{
    if (editorMap.contains(filepath)) {
        editorMap[filepath]->textEditor->scrollToLine(scrollOffset, line, 0);
        
        QTimer::singleShot(0, editorMap[filepath]->textEditor, SLOT(setFocus()));
    }
}

void Window::handleJumpToLine(QString filepath, int line)
{
    if (editorMap.contains(filepath)) {
        editorMap[filepath]->textEditor->jumpToLine(line, true);
        
        QTimer::singleShot(0, editorMap[filepath]->textEditor, SLOT(setFocus()));
    }
}

void Window::handleTempJumpToLine(QString filepath, int line)
{
    if (editorMap.contains(filepath)) {
        editorMap[filepath]->textEditor->jumpToLine(line, true);
    }
}

void Window::handleCancelJump()
{
    QTimer::singleShot(0, getActiveEditor()->textEditor, SLOT(setFocus()));
}

void Window::incrementFontSize()
{
    foreach (Editor *editor, editorMap.values()) {
        int size = std::min(fontSize + 1, settings->maxFontSize);
        editor->textEditor->setFontSize(size);
        saveFontSize(size);
    }
}

void Window::decrementFontSize()
{
    foreach (Editor *editor, editorMap.values()) {
        int size = std::max(fontSize - 1, settings->minFontSize);
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

void Window::setFontSizeWithConfig(Editor *editor)
{
    int size =  settings->getOption("default_font_size").toInt();
    editor->textEditor->setFontSize(size);
    
    fontSize = size;
}

void Window::saveFontSize(int size)
{
    fontSize = size;
    
    settings->setOption("default_font_size", fontSize);
}

void Window::addBottomWidget(QWidget *widget)
{
    layout->addWidget(widget);
}

void Window::removeBottomWidget()
{
    layout->takeAt(1);
}
    
void Window::popupFindBar()
{
    addBottomWidget(findBar);
    
    QString tabPath = tabbar->getActiveTabPath();
    Editor *editor = getActiveEditor();
    QString text = editor->textEditor->textCursor().selectedText();
    int row = editor->textEditor->getCurrentLine();
    int column = editor->textEditor->getCurrentColumn();
    int scrollOffset = editor->textEditor->getScrollOffset();
    
    findBar->activeInput(text, tabPath, row, column, scrollOffset);
}

void Window::handleBackToPosition(QString file, int row, int column, int scrollOffset)
{
    if (editorMap.contains(file)) {
        editorMap[file]->textEditor->scrollToLine(scrollOffset, row, column);
        
        QTimer::singleShot(0, editorMap[file]->textEditor, SLOT(setFocus()));
    }
}

void Window::handleUpdateSearchKeyword(QString file, QString keyword)
{
    QString tabPath = tabbar->getActiveTabPath();

    if (file == tabPath && editorMap.contains(file)) {
        editorMap[file]->textEditor->highlightKeyword(keyword);
    }
}
