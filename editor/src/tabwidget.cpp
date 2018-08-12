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
#include "utils.h"
#include "texteditor.h"
#include "window.h"

#include <QDebug>
#include <QStyleFactory>
#include <QGuiApplication>
#include <DPlatformWindowHandle>

TabWidget::TabWidget()
{
    // Init.
    installEventFilter(this);   // add event filter
    setMovable(true);
    setTabsClosable(true);
    setVisibleAddButton(true);
    setDragable(true);
    setStartDragDistance(20);   // set drag drop distance
    setElideMode(Qt::ElideMiddle);

    // Set mask color.
    QColor dropColor("#333333");
    dropColor.setAlpha(0);
    setMaskColor(dropColor);

    m_rightClickTab = -1;

    setFixedHeight(40);

    connect(this, &DTabBar::tabReleaseRequested, this, &TabWidget::handleTabReleaseRequested);
    connect(this, &DTabBar::dragActionChanged, this, &TabWidget::handleDragActionChanged);
    connect(this, &DTabBar::tabIsRemoved, this, &TabWidget::handleTabRemoved);
}

QMimeData* TabWidget::createMimeDataFromTab(int index, const QStyleOptionTab &) const
{
    // Get tab name, path, and content.
    QString tabPath = tabFiles[index];
    QString tabName = tabText(index);
    TextEditor *textEditor = static_cast<Window *>(this->window())->getTextEditor(tabFiles[index]);
    QString tabContent = textEditor->toPlainText();

    // Add tab info in DND data.
    QMimeData *mimeData = new QMimeData;
    mimeData->setData("tabInfo", (QString("%1\n%2\n%3").arg(tabName, tabPath, tabContent)).toUtf8());

    // Remove text/plain format, avoid drop tab text to other applications
    mimeData->removeFormat("text/plain");

    qDebug() << "### createMimeDataFromTab: " << index;

    return mimeData;
}

QPixmap TabWidget::createDragPixmapFromTab(int index, const QStyleOptionTab &, QPoint *offset) const
{
    const qreal screenScale = qApp->devicePixelRatio();

    // Take editor's screenshot as drag image.
    TextEditor *textEditor = static_cast<Window*>(this->window())->getTextEditor(tabFiles[index]);
    int width = textEditor->width() * screenScale;
    int height = textEditor->height() * screenScale;
    QImage screenshotImage(width, height, QImage::Format_ARGB32_Premultiplied);
    screenshotImage.setDevicePixelRatio(screenScale);
    textEditor->render(&screenshotImage, QPoint(), QRegion(0, 0, width, height));

    // Scaled image to smaller.
    int scaledWidth = width * screenScale / 5;
    int scaledHeight = height * screenScale / 5;
    auto scaledImage = screenshotImage.scaled(scaledWidth, scaledHeight, Qt::IgnoreAspectRatio, Qt::SmoothTransformation);

    // Clip screenshot image with window radius.
    QPainter painter(&scaledImage);
    painter.setRenderHint(QPainter::Antialiasing, true);

    QPainterPath rectPath;
    QPainterPath roundedRectPath;

    rectPath.addRect(0, 0, scaledWidth, scaledHeight);
    roundedRectPath.addRoundedRect(QRect(0, 0, scaledWidth / screenScale, scaledHeight / screenScale), 6, 6);

    rectPath -= roundedRectPath;

    painter.setCompositionMode(QPainter::CompositionMode_Source);
    painter.fillPath(rectPath, Qt::transparent);
    painter.end();

    // Hide window when drag start, just hide if only one tab in current window.
    if (count() == 1) {
        static_cast<Window*>(this->window())->hide();
    }

    // Adjust offset.
    offset->setX(20);
    offset->setY(20);

    // Return image composited with shadow.
    QColor shadowColor = QColor("#000000");
    shadowColor.setAlpha(80);

    qDebug() << "### createDragPixmapFromTab: " << index;

    return Utils::dropShadow(QPixmap::fromImage(scaledImage), 40, shadowColor, QPoint(0, 8));
}

bool TabWidget::canInsertFromMimeData(int index, const QMimeData *source) const
{
    qDebug() << "### canInsertFromMimeData: " << index;

    return source->hasFormat("tabInfo");
}

void TabWidget::insertFromMimeData(int index, const QMimeData *source)
{
    // Create new tab create drop from other deepin-editor.
    QString content = QString::fromUtf8(source->data("tabInfo"));
    QStringList dropContent = content.split("\n");
    QString tabName = dropContent[0];
    QString tabPath = dropContent[1];
    QString tabContent = content.remove(0, tabName.size() + tabPath.size() + 2); // 2 mean two \n char

    Window* window = static_cast<Window *>(this->window());

    window->addTabWithContent(tabName, tabPath, tabContent, index);
    window->activeTab(window->getTabIndex(tabPath));

    qDebug() << "### insertFromMimeData: " << index;
}

void TabWidget::insertFromMimeDataOnDragEnter(int index, const QMimeData *source)
{
    QString content = QString::fromUtf8(source->data("tabInfo"));
    QStringList dropContent = content.split("\n");
    QString tabName = dropContent[0];
    QString tabPath = dropContent[1];
    QString tabContent = content.remove(0, tabName.size() + tabPath.size() + 2); // 2 mean two \n char

    Window* window = static_cast<Window *>(this->window());

    window->addTabWithContent(tabName, tabPath, tabContent, index);
    window->activeTab(window->getTabIndex(tabPath));

    qDebug() << "### insertFromMimeDataOnDragEnter: " << index;
}

void TabWidget::handleCloseTab()
{
    if (m_rightClickTab >= 0) {
        closeTab(m_rightClickTab);
    }
}

void TabWidget::handleCloseOtherTabs()
{
    if (m_rightClickTab >= 0) {
        closeOtherTabs(m_rightClickTab);
    }
}

bool TabWidget::eventFilter(QObject *, QEvent *event)
{
    if (event->type() == QEvent::MouseButtonPress) {
        QMouseEvent *mouseEvent = static_cast<QMouseEvent *>(event);
        if (mouseEvent->button() == Qt::RightButton) {
            QPoint position = mouseEvent->pos();
            m_rightClickTab = -1;

            for (int i = 0; i < count(); i++) {
                if (tabRect(i).contains(position)) {
                    m_rightClickTab = i;
                    break;
                }
            }

            // Poup right menu on tab.
            if (m_rightClickTab >= 0) {
                m_menu = new QMenu();
                m_menu->setStyle(QStyleFactory::create("dlight"));

                m_closeTabAction = new QAction("Close Tab", this);
                m_closeOtherTabAction = new QAction("Close Other Tabs", this);

                connect(m_closeTabAction, &QAction::triggered, this, &TabWidget::handleCloseTab);
                connect(m_closeOtherTabAction, &QAction::triggered, this, &TabWidget::handleCloseOtherTabs);

                m_menu->addAction(m_closeTabAction);
                m_menu->addAction(m_closeOtherTabAction);

                m_menu->exec(this->mapToGlobal(position));

                return true;
            }
        }
    } else if (event->type() == QEvent::DragEnter) {
        const QDragEnterEvent *e = static_cast<QDragEnterEvent*>(event);
        const QMimeData* mimeData = e->mimeData();

        if ((!e->source() || e->source()->parent() != this) && mimeData->data("tabInfo") != "") {
            static_cast<Window*>(this->window())->changeTitlebarBackground(m_dndStartColor, m_dndEndColor);
        }

        qDebug() << "### eventFilter DragEnter";
    } else if (event->type() == QEvent::DragLeave) {
        static_cast<Window*>(this->window())->changeTitlebarBackground(m_backgroundStartColor, m_backgroundEndColor);

        qDebug() << "### eventFilter DragLeave";
    } else if (event->type() == QEvent::Drop) {
        static_cast<Window*>(this->window())->changeTitlebarBackground(m_backgroundStartColor, m_backgroundEndColor);

        qDebug() << "### eventFilter Drop";
    } else if (event->type() == QEvent::DragMove) {
        event->accept();
    }

    return false;
}

void TabWidget::handleTabReleaseRequested()
{
    // Show window agian if tab drop failed and only one tab in current window.
    if (count() == 1) {
        static_cast<Window*>(this->window())->show();
    }

    qDebug() << "### handleTabReleaseRequested";
}

void TabWidget::handleDragActionChanged(Qt::DropAction action)
{
    // Reset cursor to Qt::ArrowCursor if drag tab to TextEditor widget.
    if (action == Qt::IgnoreAction) {
        if (dragIconWindow()) {
            QGuiApplication::changeOverrideCursor(Qt::ArrowCursor);
            DPlatformWindowHandle::setDisableWindowOverrideCursor(dragIconWindow(), true);
        }
    } else if (dragIconWindow()) {
        DPlatformWindowHandle::setDisableWindowOverrideCursor(dragIconWindow(), false);
        if (QGuiApplication::overrideCursor())
            QGuiApplication::changeOverrideCursor(QGuiApplication::overrideCursor()->shape());
    }

    qDebug() << "### handleDragActionChanged";
}

void TabWidget::setBackground(QString startColor, QString endColor)
{
    m_backgroundStartColor = startColor;
    m_backgroundEndColor = endColor;
}

void TabWidget::setDNDColor(QString startColor, QString endColor)
{
    m_dndStartColor = startColor;
    m_dndEndColor = endColor;
}

void TabWidget::handleTabRemoved(int index)
{
    QString filepath = tabFiles[index];

    tabFiles.takeAt(index);
    closeFile(filepath);
    qDebug() << "*** Remove tab " << index;
}
