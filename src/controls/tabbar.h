/*
 * Copyright (C) 2017 ~ 2018 Deepin Technology Co., Ltd.
 *
 * Author:     rekols <rekols@foxmail.com>
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

#ifndef TABBAR_H
#define TABBAR_H

#include <DTabBar>
#include <DMenu>
#include <QMouseEvent>
#include <DScrollBar>

DWIDGET_USE_NAMESPACE

class EditWrapper;
class Tabbar : public DTabBar
{
    Q_OBJECT

public:
    explicit Tabbar(QWidget *parent = nullptr);
    ~Tabbar();

    void addTab(const QString &filePath, const QString &tabName, const QString &tipPath = QString::null);
    void addTabWithIndex(int index, const QString &filePath, const QString &tabName, const QString &tipPath = QString::null);
    void closeTab(int index);
    void closeCurrentTab();
    void closeOtherTabs();

    void closeLeftTabs(const QString &filePath);
    void closeRightTabs(const QString &filePath);

    void closeOtherTabsExceptFile(const QString &filePath);
    void updateTab(int index, const QString &filePath, const QString &tabName);
    void previousTab();
    void nextTab();

    int indexOf(const QString &filePath);

    QString currentName() const;
    QString currentPath() const;
    QString truePathAt(int index) const;
    QString fileAt(int index) const;
    QString textAt(int index) const;

    void setTabPalette(const QString &activeColor, const QString &inactiveColor);
    void setBackground(const QString &startColor, const QString &endColor);
    void setDNDColor(const QString &startColor, const QString &endColor);
    void showTabs();

signals:
    void requestHistorySaved(const QString &filePath);
    void closeTabs(const QStringList pathList);

protected:
    #ifdef TABLET
    QPixmap createDragPixmapFromTab(int index, const QStyleOptionTab &option, QPoint *hotspot) const;
    QMimeData *createMimeDataFromTab(int index, const QStyleOptionTab &option) const;
    void insertFromMimeDataOnDragEnter(int index, const QMimeData *source);
    void insertFromMimeData(int index, const QMimeData *source);
    bool canInsertFromMimeData(int index, const QMimeData *source) const;
    #endif
    bool eventFilter(QObject *, QEvent *event);

    QSize tabSizeHint(int index) const;
    QSize minimumTabSizeHint(int index) const;
    QSize maximumTabSizeHint(int index) const;
    void mousePressEvent(QMouseEvent *e);
    #ifdef TABLET
    void dropEvent(QDropEvent *e);
    #endif
    void resizeEvent(QResizeEvent *event);
private:
    void handleTabMoved(int fromIndex, int toIndex);
    void handleTabReleased(int index);
    void handleTabIsRemoved(int index);
    void handleTabDroped(int index, Qt::DropAction, QObject *target);
    void handleDragActionChanged(Qt::DropAction action);
    void onTabDrapStart();

private:
    QStringList m_tabPaths;
    QStringList m_tabTruePaths;
    QStringList m_listOldTabPath;
    QString m_backgroundStartColor;
    QString m_backgroundEndColor;
    QString m_dndStartColor;
    QString m_dndEndColor;

    QAction *m_closeOtherTabAction;
    QAction *m_closeTabAction;
    QAction *m_closeLeftTabAction;
    QAction *m_closeRightTabAction;
    QAction *m_closeAllunModifiedTabAction;
    DMenu   *m_rightMenu;
    DMenu   *m_moreWaysCloseMenu;
    int m_rightClickTab;
    int m_nDragIndex;
    QString m_qstrDragName;
    QString m_qstrDragPath;
    EditWrapper *m_pWrapper = nullptr;

public:
    static QPixmap *sm_pDragPixmap;
};

#endif
