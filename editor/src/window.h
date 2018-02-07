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

#include "ddialog.h"
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
    
    int getTabIndex(QString file);
    void activeTab(int index);
    
    void addTab(QString file);
    void addTabWithContent(QString tabName, QString filepath, QString content);
    void closeTab();
    void restoreTab();
    
    Editor* createEditor();
    Editor* getActiveEditor();
    TextEditor* getTextEditor(QString filepath);
    void focusActiveEditor();
    
    void openFile();
    bool saveFile();
    void saveAsFile();
    void saveFileAsAnotherPath(QString fromPath, QString toPath, bool deleteOldFile=false);
    
    void decrementFontSize();
    void incrementFontSize();
    void resetFontSize();
    void saveFontSize(int size);
    void setFontSizeWithConfig(Editor *editor);
    
    void popupFindBar();
    void popupReplaceBar();
    void popupJumpLineBar();
    
    void toggleFullscreen();
    
    void keyPressEvent(QKeyEvent *keyEvent);
    
    QStringList getEncodeList();
    
signals:
    void dropTabOut(QString tabName, QString filepath, QString content);
    void newWindow();
    
public slots:
    void addBlankTab(QString blankFile="");
    void handleTabReleaseRequested(QString tabName, QString filepath, int index);
    
    void handleCloseFile(QString filepath);
    void handleSwitchToFile(QString filepath);
    
    void handleJumpLineBarExit();
    void handleJumpLineBarJumpToLine(QString filepath, int line, bool focusEditor);
    
    void handleBackToPosition(QString file, int row, int column, int scrollOffset);
    
    void handleFindNext();
    void handleFindPrev();
    
    void handleReplaceAll(QString replaceText, QString withText);
    void handleReplaceNext(QString replaceText, QString withText);
    void handleReplaceRest(QString replaceText, QString withText);
    void handleReplaceSkip();
    
    void handleRemoveSearchKeyword();
    void handleUpdateSearchKeyword(QString file, QString keyword);
    
    void addBottomWidget(QWidget *widget);
    void removeBottomWidget();
    
    void popupPrintDialog();
    void popupSettingDialog();
    
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
    int toastPaddingBottom = 100;
    
    QMenu *menu;
    QAction *newWindowAction;
    QAction *newTabAction;
    QAction *openFileAction;
    QAction *saveAction;
    QAction *saveAsAction;
    QAction *printAction;
    QAction *switchThemeAction;
    QAction *settingAction;
    
    QStringList closeFileHistory;
    
    void removeActiveBlankTab(bool needSaveBefore=false);
    
    void showNewEditor(Editor *editor);
    
    DDialog* createSaveBlankFileDialog();
    void dtkThemeWorkaround(QWidget *parent, const QString &theme);
};

#endif
