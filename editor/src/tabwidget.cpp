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
#include "texteditor.h"
#include "window.h"

#include <QDebug>
#include <QStyleFactory>

TabWidget::TabWidget()
{
    // Init.
    installEventFilter(this);   // add event filter
    setMovable(true);
    setTabsClosable(true);
    setVisibleAddButton(true);
    setDragable(true);
    setStartDragDistance(20);   // set drag drop distance

    // Set mask color.
    QColor dropColor("#333333");
    dropColor.setAlpha(0);
    setMaskColor(dropColor);

    rightClickTab = -1;
    
    setFixedHeight(40);
}

QMimeData* TabWidget::createMimeDataFromTab(int index, const QStyleOptionTab &) const
{
    // Get tab name, path, and content.
    QString tabPath = tabFiles[index];
    QString tabName = tabText(index);
    TextEditor *textEditor = static_cast<Window*>(this->window())->getTextEditor(tabFiles[index]);
    QString tabContent = textEditor->toPlainText();

    // Add tab info in DND data.
    QMimeData *mimeData = new QMimeData;
    mimeData->setData("tabInfo", (QString("%1\n%2\n%3").arg(tabName, tabPath, tabContent)).toUtf8());
    
    // Remove text/plain format, avoid drop tab text to other applications
    mimeData->removeFormat("text/plain");

    return mimeData;
}

QPixmap TabWidget::createDragPixmapFromTab(int index, const QStyleOptionTab &, QPoint *) const
{
    // Take editor's screenshot as drag image.
    TextEditor *textEditor = static_cast<Window*>(this->window())->getTextEditor(tabFiles[index]);
    int width = textEditor->width();
    int height = textEditor->height();
    QPixmap pixmap(width, height);
    textEditor->render(&pixmap, QPoint(), QRegion(0, 0, width, height));

    // We need make editor screenshot smaller.
    return pixmap.scaled(width / 5, height / 5);
}

bool TabWidget::canInsertFromMimeData(int, const QMimeData *) const
{
    // Any index can insert.
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

            // Poup right menu on tab.
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
    } else if (event->type() == QEvent::DragEnter) {
        static_cast<Window*>(this->window())->changeTitlebarBackground("#333333");
    } else if (event->type() == QEvent::DragLeave) {
        static_cast<Window*>(this->window())->changeTitlebarBackground("#202020");
    }

    return false;
}
