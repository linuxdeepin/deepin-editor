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
#include "wordcompletionwindow.h"
#include "dmainwindow.h"
#include "editor.h"
#include "findbar.h"
#include "jumplinebar.h"
#include "themebar.h"
#include "replacebar.h"
#include "settings.h"
#include "tabbar.h"
#include "dwindowmanager.h"

#include <QHBoxLayout>
#include <QResizeEvent>
#include <QStackedLayout>
#include <QVBoxLayout>
#include <QWidget>
#include <dimagebutton.h>
#include <QSqlDatabase>

DWIDGET_USE_NAMESPACE
DWM_USE_NAMESPACE

class Window : public DMainWindow
{
    Q_OBJECT
    
public:
    Window(DMainWindow *parent = 0);
    ~Window();
    
    int getTabIndex(QString file);
    void activeTab(int index);
    
    void addTab(QString file);
    void addTabWithContent(QString tabName, QString filepath, QString content, int index);
    void closeTab();
    void restoreTab();
    
    Editor* createEditor();
    Editor* getActiveEditor();
    TextEditor* getTextEditor(QString filepath);
    void focusActiveEditor();
    
    void openFile();
    bool saveFile();
    void saveAsFile();
    void saveFileAsAnotherPath(QString fromPath, QString toPath, QString encode, QString newline, bool deleteOldFile=false);
    
    void decrementFontSize();
    void incrementFontSize();
    void resetFontSize();
    void setFontSizeWithConfig(Editor *editor);
    
    void popupFindBar();
    void popupReplaceBar();
    void popupJumpLineBar();
    void popupThemeBar();
    
    void toggleFullscreen();
    
    QStringList getEncodeList();
    
    void remberPositionSave(bool notify=true);
    void remberPositionRestore();
    
    void updateFont(QString fontName);
    void updateFontSize(int size);
    void updateTabSpaceNumber(int number);
    
    void changeTitlebarBackground(QString startColor, QString endColor);
    
    bool wordCompletionWindowIsVisible();
    
    QString getSaveFilePath(QString &encode, QString &newline);
    
    void displayShortcuts();
    
protected:    
    void resizeEvent(QResizeEvent* event);
    void closeEvent(QCloseEvent *event);
    void keyPressEvent(QKeyEvent *keyEvent);
    
signals:
    void dropTabOut(QString tabName, QString filepath, QString content);
    void newWindow();
    void close();
    
public slots:
    void addBlankTab(QString blankFile="");
    void handleTabReleaseRequested(QString tabName, QString filepath, int index);
    void handleTabCloseRequested(int index);
    
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
    void handleUpdateSearchKeyword(QWidget *widget, QString file, QString keyword);
    
    void addBottomWidget(QWidget *widget);
    void removeBottomWidget();
    
    void popupPrintDialog();
    
    void handlePopupCompletionWindow(QString word, QPoint pos, QStringList words);
    void handleSelectNextCompletion();
    void handleSelectPrevCompletion();
    void handleSelectFirstCompletion();
    void handleSelectLastCompletion();
    void handleConfirmCompletion();
    
    void loadTheme(QString themeName);
    
private:
    DBusDaemon::dbus *autoSaveDBus;
    
    FindBar *findBar;
    JumpLineBar *jumpLineBar;
    QMap<QString, Editor*> editorMap;
    QStackedLayout *editorLayout;
    QString blankFileDir;
    QString readonlyFileDir;
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
    
    QString remberPositionFilePath;
    int remberPositionRow;
    int remberPositionColumn;
    int remberPositionScrollOffset;
    
    QString titlebarStyleSheet;
    
    bool windowShowFlag = false;
    
    void removeActiveBlankTab(bool needSaveBefore=false);
    void removeActiveReadonlyTab();
    
    void showNewEditor(Editor *editor);
    void showNotify(QString message);
    
    DDialog* createSaveFileDialog(QString title, QString content);
    
    QSqlDatabase wordsDB;
    
    WordCompletionWindow *wordCompletionWindow;
    
    DWindowManager *windowManager;
    
    QString readonlySeparator = " !_! ";
    
    ThemeBar *themeBar;
    
    QString themeName;
    QString lightTabBackgroundStartColor = "(255, 255, 255, 90%)";
    QString lightTabBackgroundEndColor = "(248, 248, 248, 90%)";
    QString darkTabBackgroundStartColor = "(16, 16, 16, 90%)";
    QString darkTabBackgroundEndColor = "(16, 16, 16, 90%)";
};

#endif
