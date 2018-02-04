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

#include "tabwidget.h"
#include "window.h"
#include "texteditor.h"
#include <QDebug>
#include <QStyleFactory>

TabWidget::TabWidget()
{
    installEventFilter(this);   // add event filter

    rightClickTab = -1;

    setMovable(true);
    setTabsClosable(true);
    setVisibleAddButton(true);
    setDragable(true);

    QColor dropColor("#333333");
    dropColor.setAlpha(128);
    setMaskColor(dropColor);

    setStartDragDistance(20);
}

QPixmap TabWidget::createDragPixmapFromTab(int index, const QStyleOptionTab &, QPoint *) const
{
    TextEditor *textEditor = static_cast<Window*>(this->window())->getTextEditor(tabFiles[index]);

    int width = textEditor->width();
    int height = textEditor->height();
    QPixmap pixmap(width, height);
    textEditor->render(&pixmap, QPoint(), QRegion(0, 0, width, height));

    return pixmap.scaled(width / 5, height / 5);
}

QMimeData* TabWidget::createMimeDataFromTab(int index, const QStyleOptionTab &) const
{
    QString tabPath = tabFiles[index];
    QString tabName = tabText(index);
    TextEditor *textEditor = static_cast<Window*>(this->window())->getTextEditor(tabFiles[index]);
    QString tabContent = textEditor->toPlainText();

    QMimeData *mimeData = new QMimeData;
    mimeData->setData("tabInfo", (QString("%1\n%2\n%3").arg(tabName, tabPath, tabContent)).toUtf8());
    mimeData->removeFormat("text/plain"); // avoid drop tab text to other applications

    return mimeData;
}

bool TabWidget::canInsertFromMimeData(int, const QMimeData *) const
{
    return true;
}

void TabWidget::insertFromMimeData(int, const QMimeData *source)
{
    // Create new tab create drop from other deepin-editor.
    QString content = QString::fromUtf8(source->data("tabInfo"));
    QStringList dropContent = content.split("\n");
    QString tabName = dropContent[0];
    QString tabPath = dropContent[1];
    QString tabContent = content.remove(0, tabName.size() + tabPath.size() + 2); // 2 mean two \n char

    static_cast<Window*>(this->window())->addTabWithContent(tabName, tabPath, tabContent);
}

bool TabWidget::eventFilter(QObject *, QEvent *event)
{
    if (event->type() == QEvent::MouseButtonPress) {
        QMouseEvent *mouseEvent = static_cast<QMouseEvent *>(event);
        if (mouseEvent->button() == Qt::RightButton) {
            QPoint position = mouseEvent->pos();
            rightClickTab = -1;

            for (int i = 0; i < count(); i++) {
                if (tabRect(i).contains(position)) {
                    rightClickTab = i;
                    break;
                }
            }

            if (rightClickTab >= 0) {
                menu = new QMenu();
                menu->setStyle(QStyleFactory::create("dlight"));
                closeTabAction = new QAction("Close Tab", this);
                closeOtherTabAction = new QAction("Close Other Tabs", this);
                
                connect(closeTabAction, &QAction::triggered, this, &TabWidget::handleCloseTab);
                connect(closeOtherTabAction, &QAction::triggered, this, &TabWidget::handleCloseOtherTabs);
                
                menu->addAction(closeTabAction);
                menu->addAction(closeOtherTabAction);
                
                menu->exec(this->mapToGlobal(position));

                return true;
            }
        }
    }

    return false;
}

void TabWidget::handleCloseTab()
{
    if (rightClickTab >= 0) {
        closeTab(rightClickTab);
    }
}

void TabWidget::handleCloseOtherTabs()
{
    if (rightClickTab >= 0) {
        closeOtherTabs(rightClickTab);
    }
}
