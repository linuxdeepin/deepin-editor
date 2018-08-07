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

    int getTabIndex(const QString &file);
    void activeTab(int index);

    void addTab(const QString &file, bool activeTab=false);
    void addTabWithContent(const QString &tabName, const QString &filepath, const QString &content, int index=-1);
    void closeTab();
    void restoreTab();

    Editor* createEditor();
    Editor* getActiveEditor();
    TextEditor* getTextEditor(const QString &filepath);
    void focusActiveEditor();

    void openFile();
    bool saveFile();
    void saveAsFile();
    void saveFileAsAnotherPath(const QString &fromPath, const QString &toPath, const QString &encode, const QString &newline, bool deleteOldFile=false);

    void decrementFontSize();
    void incrementFontSize();
    void resetFontSize();
    void setFontSizeWithConfig(Editor *editor);

    void popupFindBar();
    void popupReplaceBar();
    void popupJumpLineBar();
    void popupThemeBar();

    void toggleFullscreen();

    const QStringList getEncodeList();

    void remberPositionSave(bool notify=true);
    void remberPositionRestore();

    void updateFont(const QString &fontName);
    void updateFontSize(int size);
    void updateTabSpaceNumber(int number);

    void changeTitlebarBackground(const QString &startColor, const QString &endColor);

    bool wordCompletionWindowIsVisible();

    const QString getSaveFilePath(QString &encode, QString &newline);

    void displayShortcuts();

protected:
    void resizeEvent(QResizeEvent* event) Q_DECL_OVERRIDE;
    void closeEvent(QCloseEvent *event);
    void keyPressEvent(QKeyEvent *keyEvent);
    void dragEnterEvent(QDragEnterEvent *e);
    void dropEvent(QDropEvent* event);
    bool eventFilter(QObject *, QEvent *event);

signals:
    void dropTabOut(QString tabName, QString filepath, QString content);
    void newWindow();
    void close();

public slots:
    void addBlankTab();
    void addBlankTab(const QString &blankFile);
    void handleTabReleaseRequested(const QString &tabName, const QString &filepath, int index);
    void handleTabCloseRequested(int index);

    void handleCloseFile(const QString &filepath);
    void handleSwitchToFile(const QString &filepath);

    void handleJumpLineBarExit();
    void handleJumpLineBarJumpToLine(const QString &filepath, int line, bool focusEditor);

    void handleBackToPosition(const QString &file, int row, int column, int scrollOffset);

    void handleFindNext();
    void handleFindPrev();

    void handleReplaceAll(const QString &replaceText, const QString &withText);
    void handleReplaceNext(const QString &replaceText, const QString &withText);
    void handleReplaceRest(const QString &replaceText, const QString &withText);
    void handleReplaceSkip();

    void handleRemoveSearchKeyword();
    void handleUpdateSearchKeyword(QWidget *widget, const QString &file, const QString &keyword);

    void addBottomWidget(QWidget *widget);
    void removeBottomWidget();

    void popupPrintDialog();

    void handlePopupCompletionWindow(const QString &word, const QPoint &pos, const QStringList &words);
    void handleSelectNextCompletion();
    void handleSelectPrevCompletion();
    void handleSelectFirstCompletion();
    void handleSelectLastCompletion();
    void handleConfirmCompletion();

    void loadTheme(const QString &themeName);

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

    int getBlankFileIndex();

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

    bool inCompleting = false;
    QTimer *inCompletingTimer;
};

#endif
