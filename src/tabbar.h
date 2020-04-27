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

DWIDGET_USE_NAMESPACE

class Tabbar : public DTabBar
{
    Q_OBJECT

public:
    Tabbar(QWidget *parent = nullptr);
    ~Tabbar();

    void addTab(const QString &filePath, const QString &tabName);
    void addTabWithIndex(int index, const QString &filePath, const QString &tabName);
    void closeTab(int index);
    void closeCurrentTab();
    void closeOtherTabs();
    void closeOtherTabsExceptFile(const QString &filePath);
    void updateTab(int index, const QString &filePath, const QString &tabName);
    void previousTab();
    void nextTab();

    int indexOf(const QString &filePath);

    QString currentName() const;
    QString currentPath() const;
    QString fileAt(int index) const;
    QString textAt(int index) const;

    void setTabActiveColor(const QString &color);
    void setBackground(const QString &startColor, const QString &endColor);
    void setDNDColor(const QString &startColor, const QString &endColor);

signals:
    void requestHistorySaved(const QString &filePath);
    void closeTabs(const QStringList pathList);

protected:
    QPixmap createDragPixmapFromTab(int index, const QStyleOptionTab &option, QPoint *hotspot) const;
    QMimeData *createMimeDataFromTab(int index, const QStyleOptionTab &option) const;
    void insertFromMimeDataOnDragEnter(int index, const QMimeData *source);
    void insertFromMimeData(int index, const QMimeData *source);
    bool canInsertFromMimeData(int index, const QMimeData *source) const;
    void handleDragActionChanged(Qt::DropAction action);
    bool eventFilter(QObject *, QEvent *event);

private:
    void handleTabMoved(int fromIndex, int toIndex);
    void handleTabReleased(int index);
    void handleTabIsRemoved(int index);
    void handleTabDroped(int index, Qt::DropAction, QObject *target);
    QStringList readTabPaths() const;
    void writeTabPaths();
private:
    QStringList m_tabPaths;
    QString m_backgroundStartColor;
    QString m_backgroundEndColor;
    QString m_dndStartColor;
    QString m_dndEndColor;

    QAction *m_closeOtherTabAction;
    QAction *m_closeTabAction;
    DMenu *m_rightMenu;
    int m_rightClickTab;
};

#endif
