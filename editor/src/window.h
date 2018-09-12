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
#include "theme_module/themepanel.h"

#include <QWidget>
#include <QStackedWidget>
#include <QResizeEvent>
#include <QVBoxLayout>

#include "dwindowmanager.h"
#include "dimagebutton.h"

DWIDGET_USE_NAMESPACE
DWM_USE_NAMESPACE

class Window : public DMainWindow
{
    Q_OBJECT

public:
    Window(DMainWindow *parent = 0);
    ~Window();

    void initTitlebar();

    int getTabIndex(const QString &file);
    void activeTab(int index);

    void addTab(const QString &filepath, bool activeTab = false);
    void addTabWithContent(const QString &tabName,
                           const QString &filepath,
                           const QString &content,
                           bool isModified = false,
                           int index = -1);
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
    void popupSettingsDialog();

    void toggleFullscreen();

    const QStringList getEncodeList();

    void remberPositionSave(bool notify = true);
    void remberPositionRestore();

    void updateFont(const QString &fontName);
    void updateFontSize(int size);
    void updateTabSpaceNumber(int number);

    void changeTitlebarBackground(const QString &color);
    void changeTitlebarBackground(const QString &startColor, const QString &endColor);

    const QString getSaveFilePath(QString &encode, QString &newline);

    void displayShortcuts();

signals:
    void dropTabOut(QString tabName, QString filepath, QString content, bool isModified);
    void themeChanged(const QString themeName);
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
    void handleCurrentChanged(const int &index);

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

    void loadTheme(const QString &path);

private:
    void removeActiveBlankTab(bool needSaveBefore = false);
    void removeActiveReadonlyTab();
    void showNewEditor(Editor *editor);
    void showNotify(QString message);
    DDialog* createSaveFileDialog(QString title, QString content);
    int getBlankFileIndex();

private:
    DBusDaemon::dbus *m_rootSaveDBus;

    QWidget *m_centralWidget;
    QStackedWidget *m_editorWidget;
    QVBoxLayout *m_centralLayout;
    Tabbar *m_tabbar;

    JumpLineBar *m_jumpLineBar;
    ReplaceBar *m_replaceBar;
    ThemePanel *m_themePanel;
    FindBar *m_findBar;
    Settings *m_settings;
    DWindowManager *m_windowManager;

    QMap<QString, Editor *> m_editorMap;

    QMenu *m_menu;

    QStringList m_closeFileHistory;

    QString m_remberPositionFilePath;
    int m_remberPositionRow;
    int m_remberPositionColumn;
    int m_remberPositionScrollOffset;

    QString m_blankFileDir;
    int m_fontSize;

    QString m_titlebarStyleSheet;

    bool m_windowShowFlag = false;

    QString m_readonlySeparator = " !_! ";

    QString m_themePath;
    QString m_tabbarActiveColor;
};

#endif
