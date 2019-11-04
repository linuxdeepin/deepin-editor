/* -*- Mode: C++; indent-tabs-mode: nil; tab-width: 4 -*-
 * -*- coding: utf-8 -*-
 *
 * Copyright (C) 2011 ~ 2018 Deepin, Inc.
 *
 * Author:     Wang Yong <wangyong@deepin.com>
 * Maintainer: Rekols    <rekols@foxmail.com>
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
#include "editwrapper.h"
#include "findbar.h"
#include "jumplinebar.h"
#include "replacebar.h"
#include "settings.h"
#include "tabbar.h"
#include "thememodule/themepanel.h"

#include <QWidget>
#include <QStackedWidget>
#include <QResizeEvent>
#include <QVBoxLayout>

DWIDGET_USE_NAMESPACE

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
    void addTabWithWrapper(EditWrapper *wrapper, const QString &filepath,
                           const QString &tabName, int index = -1);
    void closeTab();
    void restoreTab();

    EditWrapper* createEditor();
    EditWrapper* currentWrapper();
    EditWrapper* wrapper(const QString &filePath);
    DTextEdit* getTextEditor(const QString &filepath);
    void focusActiveEditor();
    void removeWrapper(const QString &filePath, bool isDelete = false);

    void openFile();
    bool saveFile();
    bool saveAsFile();

    void decrementFontSize();
    void incrementFontSize();
    void resetFontSize();
    void setFontSizeWithConfig(EditWrapper *editor);

    void popupFindBar();
    void popupReplaceBar();
    void popupJumpLineBar();
    void popupSettingsDialog();
    void popupPrintDialog();
    void popupThemePanel();

    void toggleFullscreen();

    void remberPositionSave();
    void remberPositionRestore();

    void updateFont(const QString &fontName);
    void updateFontSize(int size);
    void updateTabSpaceNumber(int number);
    void updateUseTab(bool useTab);
    void updateAutoIndent(bool autoIndent);

    void changeTitlebarBackground(const QString &color);
    void changeTitlebarBackground(const QString &startColor, const QString &endColor);

    void displayShortcuts();

signals:
    void themeChanged(const QString themeName);
    void requestDragEnterEvent(QDragEnterEvent *);
    void requestDropEvent(QDropEvent *);
    void newWindow();
    void close();

public slots:
    void addBlankTab();
    void addBlankTab(const QString &blankFile);
    void handleTabCloseRequested(int index);
    void handleTabsClosed(const QStringList &tabList);
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

    void loadTheme(const QString &path);

    void showNewEditor(EditWrapper *wrapper);
    void showNotify(const QString &message);
    int getBlankFileIndex();

    DDialog *createDialog(const QString &title, const QString &content);

private:
    void handleFocusWindowChanged(QWindow *w);
    void updateThemePanelGeomerty();
    void checkTabbarForReload();

protected:
    void resizeEvent(QResizeEvent* event) override;
    void closeEvent(QCloseEvent *event) override;
    void keyPressEvent(QKeyEvent *keyEvent) override;
    void dragEnterEvent(QDragEnterEvent *e) override;
    void dropEvent(QDropEvent* event) override;

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

    QMap<QString, EditWrapper *> m_wrappers;

    QMenu *m_menu;

    QStringList m_closeFileHistory;

    QString m_remberPositionFilePath;
    int m_remberPositionRow;
    int m_remberPositionColumn;
    int m_remberPositionScrollOffset;

    QString m_blankFileDir;
    int m_fontSize = 0;

    QString m_titlebarStyleSheet;

    QString m_themePath;
    QString m_tabbarActiveColor;
};

#endif
