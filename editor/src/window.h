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

    void addTab(const QString &file, bool activeTab = false);
    void addTabWithContent(const QString &tabName, const QString &filepath, const QString &content, int index = -1);
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

signals:
    void dropTabOut(QString tabName, QString filepath, QString content);
    void requestDragEnterEvent(QDragEnterEvent *);
    void requestDropEvent(QDropEvent *);
    void newWindow();
    void close();

protected:
    void resizeEvent(QResizeEvent* event) override;
    void closeEvent(QCloseEvent *event) override;
    void keyPressEvent(QKeyEvent *keyEvent) override;
    void dragEnterEvent(QDragEnterEvent *e) override;
    void dropEvent(QDropEvent* event) override;
    bool eventFilter(QObject *, QEvent *event) override;

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
    void removeActiveBlankTab(bool needSaveBefore=false);
    void removeActiveReadonlyTab();
    void showNewEditor(Editor *editor);
    void showNotify(QString message);
    DDialog* createSaveFileDialog(QString title, QString content);
    int getBlankFileIndex();

private:
    DBusDaemon::dbus *autoSaveDBus;

    FindBar *m_findBar;
    JumpLineBar *m_jumpLineBar;
    QMap<QString, Editor*> m_editorMap;
    QStackedLayout *m_editorLayout;
    QString m_blankFileDir;
    QString m_readonlyFileDir;
    QVBoxLayout *m_layout;
    QWidget *m_editorWidget;
    QWidget *m_layoutWidget;
    ReplaceBar *m_replaceBar;
    Settings *m_settings;
    Tabbar *m_tabbar;
    int m_fontSize;
    int m_toastPaddingBottom = 100;

    QMenu *m_menu;
    QAction *m_newWindowAction;
    QAction *m_newTabAction;
    QAction *m_openFileAction;
    QAction *m_saveAction;
    QAction *m_saveAsAction;
    QAction *m_printAction;
    QAction *m_switchThemeAction;
    QAction *m_settingAction;

    QStringList m_closeFileHistory;

    QString m_remberPositionFilePath;
    int m_remberPositionRow;
    int m_remberPositionColumn;
    int m_remberPositionScrollOffset;

    QString m_titlebarStyleSheet;

    bool m_windowShowFlag = false;


    QSqlDatabase m_wordsDB;

    WordCompletionWindow *m_wordCompletionWindow;

    DWindowManager *m_windowManager;

    QString m_readonlySeparator = " !_! ";

    ThemeBar *m_themeBar;

    QString m_themeName;
    QString m_lightTabBackgroundStartColor = "(255, 255, 255, 90%)";
    QString m_lightTabBackgroundEndColor = "(248, 248, 248, 90%)";
    QString m_darkTabBackgroundStartColor = "(16, 16, 16, 90%)";
    QString m_darkTabBackgroundEndColor = "(16, 16, 16, 90%)";

    bool m_inCompleting = false;
    QTimer *m_inCompletingTimer;
};

#endif
