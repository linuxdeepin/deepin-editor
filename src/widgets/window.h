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

#include "../controls/toolbar.h"
#include "../common/utils.h"
#include "../startmanager.h"
#include "../common/performancemonitor.h"
#include "../editor/editwrapper.h"
#include "../controls/findbar.h"
#include "../controls/jumplinebar.h"
#include "../controls/replacebar.h"
#include "../common/settings.h"
#include "../controls/tabbar.h"
#include "../thememodule/themepanel.h"
#include "../common/performancemonitor.h"
#include "../common/dbusinterface.h"
#include <DMainWindow>
#include <DStackedWidget>
#include <qprintpreviewdialog.h>
#include <dprintpreviewdialog.h>

DWIDGET_USE_NAMESPACE

class Window : public DMainWindow
{
    Q_OBJECT

public:
    explicit Window(DMainWindow *parent = nullptr);
    ~Window() override;

    //跟新文件修改状态
    void updateModifyStatus(const QString &path, bool isModified);
    //跟新tab文件名称
    void updateSaveAsFileName(QString strOldFilePath, QString strNewFilePath);

    void showCenterWindow(bool bIsCenter);
    void initTitlebar();
    bool checkBlockShutdown();

    int getTabIndex(const QString &file);
    void activeTab(int index);

    Tabbar *getTabbar();

    void addTab(const QString &filepath, bool activeTab = false);
    void addTabWithWrapper(EditWrapper *wrapper, const QString &filepath, const QString &qstrTruePath,
                           const QString &tabName, int index = -1);
    bool closeTab();
    void restoreTab();

    void clearBlack();

    EditWrapper *createEditor();
    EditWrapper *currentWrapper();
    EditWrapper *wrapper(const QString &filePath);
    TextEdit *getTextEditor(const QString &filepath);
    void focusActiveEditor();
    void removeWrapper(const QString &filePath, bool isDelete = false);

    void openFile();
    bool saveFile();
    bool saveAsFile();
    QString saveAsFileToDisk();
    QString saveBlankFileToDisk();
    bool saveAsOtherTabFile(EditWrapper *wrapper);

    void changeSettingDialogComboxFontNumber(int fontNumber);
    void decrementFontSize();
    void incrementFontSize();
    void resetFontSize();
    void setFontSizeWithConfig(EditWrapper *editor);

    void popupFindBar();
    void popupReplaceBar();
    void popupJumpLineBar();
    void updateJumpLineBar(TextEdit *editor);
    void popupSettingsDialog();
    void popupPrintDialog();
    void popupThemePanel();

    void toggleFullscreen();

    void remberPositionSave();
    void remberPositionRestore();

    void displayShortcuts();
    void doPrint(DPrinter *printer, const QVector<int> &pageRange);
    void asynPrint(QPainter &p, DPrinter *printer, const QVector<int> &pageRange);

    /**
     * @brief backupFile 备份文件
     */
    void backupFile();

    /**
     * @brief closeAllFiles 关闭当前窗口所有文件
     * @return
     */
    bool closeAllFiles();

    /**
     * @brief addTemFileTab　恢复备份文件标签页
     * @param qstrPath　打开文件路径
     * @param qstrName　真实文件名
     * @param qstrTruePath　真实文件路径
     * @param bIsTemFile　是否修改
     */
    void addTemFileTab(QString qstrPath, QString qstrName, QString qstrTruePath, bool bIsTemFile = false);

    QMap<QString, EditWrapper *> getWrappers();

    //设置显示清除焦点
    void setChildrenFocus(bool ok);

    bool findBarIsVisiable();
    QString getKeywordForSearchAll();
    QString getKeywordForSearch();

signals:
    void themeChanged(const QString themeName);
    void requestDragEnterEvent(QDragEnterEvent *);
    void requestDropEvent(QDropEvent *);
    void newWindow();
    void closeWindow();
    void sigJudgeBlockShutdown();
    void pressEsc();
public slots:
    void addBlankTab();
    void addBlankTab(const QString &blankFile);
    void handleTabCloseRequested(int index);
    void handleTabsClosed(QStringList tabList);
    void handleCurrentChanged(const int &index);

    void handleJumpLineBarExit();
    void handleJumpLineBarJumpToLine(const QString &filepath, int line, bool focusEditor);

    void handleBackToPosition(const QString &file, int row, int column, int scrollOffset);

    void handleFindNextSearchKeyword(const QString &keyword);
    void handleFindPrevSearchKeyword(const QString &keyword);
    void slotFindbarClose();
    void slotReplacebarClose();

    void handleReplaceAll(const QString &replaceText, const QString &withText);
    void handleReplaceNext(const QString &replaceText, const QString &withText);
    void handleReplaceRest(const QString &replaceText, const QString &withText);
    void handleReplaceSkip();

    void handleRemoveSearchKeyword();
    void handleUpdateSearchKeyword(QWidget *widget, const QString &file, const QString &keyword);

    void loadTheme(const QString &path);

    void showNewEditor(EditWrapper *wrapper);
    void showNotify(const QString &message);
    int getBlankFileIndex();

    DDialog *createDialog(const QString &title, const QString &content);

    void slotLoadContentTheme(DGuiApplicationHelper::ColorType themeType);
    void slotSettingResetTheme(const QString &path);

    void slot_saveReadingPath();
    void slot_beforeReplace(QString _);
    void slot_setTitleFocus();
    //清除不支持双字节字符集符号
    void slotClearDoubleCharaterEncode();

private:
    void handleFocusWindowChanged(QWindow *w);
    void updateThemePanelGeomerty();
    void checkTabbarForReload();

protected:
    void resizeEvent(QResizeEvent *event) override;
    void closeEvent(QCloseEvent *event) override;
    void hideEvent(QHideEvent *event) override;
    void keyPressEvent(QKeyEvent *keyEvent) override;
    void keyReleaseEvent(QKeyEvent *keyEvent) override;
    void dragEnterEvent(QDragEnterEvent *e) override;
    void dropEvent(QDropEvent *event) override;

private:
    DBusDaemon::dbus *m_rootSaveDBus;

    QWidget *m_centralWidget;
    DStackedWidget *m_editorWidget;
    QVBoxLayout *m_centralLayout;
    Tabbar *m_tabbar;

    JumpLineBar *m_jumpLineBar;
    ReplaceBar *m_replaceBar;
    ThemePanel *m_themePanel;
    FindBar *m_findBar;
    Settings *m_settings;

    QMap<QString, EditWrapper *> m_wrappers;

    DMenu *m_menu;

    QStringList m_closeFileHistory;

    QString m_remberPositionFilePath;
    int m_remberPositionRow;
    int m_remberPositionColumn;
    int m_remberPositionScrollOffset;

    QString m_blankFileDir;
    QString m_backupDir;
    QString m_autoBackupDir;
    int m_fontSize = 0;

    QString m_titlebarStyleSheet;

    QString m_keywordForSearch;
    QString m_keywordForSearchAll;

    QString m_themePath;
    QString m_tabbarActiveColor;
    QList <TextEdit *> m_reading_list;
    QStringList m_qlistTemFile;///<临时文件列表

    QProcess *m_shortcutViewProcess = nullptr;
    bool m_needMoveToCenter = false;
    DPrintPreviewDialog *m_pPreview {nullptr};
    //打印文本
    QTextDocument *printDoc = nullptr;
    //打印布局
    QPageLayout lastLayout;
    //判断是否是新的打印文档
    bool newPrint = false;
};

#endif
