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

#ifndef WINDOW_H
#define WINDOW_H

#include "dmainwindow.h"
#include "editor.h"
#include "findbar.h"
#include "jumplinebar.h"
#include "replacebar.h"
#include "settings.h"
#include "tabbar.h"

#include <QHBoxLayout>
#include <QResizeEvent>
#include <QStackedLayout>
#include <QVBoxLayout>
#include <QWidget>
#include <dimagebutton.h>

DWIDGET_USE_NAMESPACE

class Window : public DMainWindow
{
    Q_OBJECT
    
public:
    Window(DMainWindow *parent = 0);
    ~Window();
    
    Editor* createEditor();
    Editor* getActiveEditor();
    TextEditor* getTextEditor(QString filepath);
    
    bool saveFile();
    int getTabIndex(QString file);
    void activeTab(int index);
    void addBottomWidget(QWidget *widget);
    void addTab(QString file);
    void addTabWithContent(QString tabName, QString filepath, QString content);
    void closeTab();
    void decrementFontSize();
    void incrementFontSize();
    void keyPressEvent(QKeyEvent *keyEvent);
    void openFile();
    void popupFindBar();
    void popupReplaceBar();
    void resetFontSize();
    void saveAsFile();
    void saveFileAsAnotherPath(QString fromPath, QString toPath, bool deleteOldFile=false);
    void saveFontSize(int size);
    void setFontSizeWithConfig(Editor *editor);
    void toggleFullscreen();
    void tryCleanLayout();
    void showNewEditor(Editor *editor);
    
signals:
    void dropTabOut(QString tabName, QString filepath, QString content);
    
public slots:
    void addBlankTab(QString blankFile="");
    void cleanKeywords();
    void handleBackToLine(QString filepath, int line, int scrollOffset);
    void handleBackToPosition(QString file, int row, int column, int scrollOffset);
    void handleCancelJump();
    void handleCloseFile(QString filepath);
    void handleFindNext();
    void handleFindPrev();
    void handleJumpLine(QString filepath, int line, int lineCount, int scrollOffset);
    void handleJumpToLine(QString filepath, int line);
    void handleReplaceAll(QString replaceText, QString withText);
    void handleReplaceNext(QString replaceText, QString withText);
    void handleReplaceRest(QString replaceText, QString withText);
    void handleReplaceSkip();
    void handleSwitchToFile(QString filepath);
    void handleTabReleaseRequested(QString tabName, QString filepath, int index);
    void handleTempJumpToLine(QString filepath, int line);
    void handleUpdateSearchKeyword(QString file, QString keyword);
    void removeBottomWidget();
    
private:
    FindBar *findBar;
    JumpLineBar *jumpLineBar;
    QMap<QString, Editor*> editorMap;
    QStackedLayout *editorLayout;
    QString blankFileDir;
    QVBoxLayout *layout;
    QWidget *editorWidget;
    QWidget *layoutWidget;
    ReplaceBar *replaceBar;
    Settings *settings;
    Tabbar *tabbar;
    int fontSize;
    int autoSaveTooltipPaddingBottom = 20;
};

#endif
