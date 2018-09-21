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

#include "tabbar.h"
#include "window.h"
#include "utils.h"
#include "editorbuffer.h"
#include "startmanager.h"

#include <QApplication>
#include <QGuiApplication>
#include <DPlatformWindowHandle>

Tabbar::Tabbar(QWidget *parent)
    : DTabBar(parent)
{
    installEventFilter(this);

    setMovable(true);
    setTabsClosable(true);
    setVisibleAddButton(true);
    setDragable(true);
    setStartDragDistance(40);
    setElideMode(Qt::ElideMiddle);

    setFocusPolicy(Qt::NoFocus);
    setFixedHeight(40);

    // set mask color.
    QColor dropColor("#333333");
    dropColor.setAlpha(0);
    setMaskColor(dropColor);

    connect(this, &DTabBar::tabIsRemoved, this, [=] (int index) {
        const QString filePath = m_tabPaths.at(index);
        Window *window = static_cast<Window *>(this->window());

        m_tabPaths.removeAt(index);
        window->removeBuffer(filePath, false);
    });

    connect(this, &DTabBar::tabMoved, this, &Tabbar::handleTabMoved);
    connect(this, &DTabBar::tabDroped, this, &Tabbar::handleTabDroped);
    connect(this, &DTabBar::tabReleaseRequested, this, &Tabbar::handleTabReleased);
    connect(this, &DTabBar::dragActionChanged, this, &Tabbar::handleDragActionChanged);
}

Tabbar::~Tabbar()
{
}

void Tabbar::addTab(const QString &filePath, const QString &tabName)
{
    addTabWithIndex(currentIndex() + 1, filePath, tabName);
}

void Tabbar::addTabWithIndex(int index, const QString &filePath, const QString &tabName)
{
    // FIXME(rekols): do not insert duplicate values.
    if (!m_tabPaths.contains(filePath)) {
        m_tabPaths.insert(index, filePath);
    }

    DTabBar::insertTab(index, tabName);
    DTabBar::setTabMaximumSize(index, QSize(300, 100));
    DTabBar::setCurrentIndex(index);
}

void Tabbar::closeTab(int index)
{
    emit requestHistorySaved(fileAt(index));

    DTabBar::removeTab(index);
}

void Tabbar::closeCurrentTab()
{
    closeTab(currentIndex());
}

void Tabbar::closeOtherTabs()
{
    closeOtherTabsExceptFile(currentPath());
}

void Tabbar::closeOtherTabsExceptFile(const QString &filePath)
{
    while (m_tabPaths.size() > 1) {
        const QString &firstPath = m_tabPaths.at(0);

        if (firstPath != filePath) {
            closeTab(0);
        } else {
            closeTab(1);
        }
    }
}

void Tabbar::updateTab(int index, const QString &filePath, const QString &tabName)
{
    DTabBar::setTabText(index, tabName);
    m_tabPaths[index] = filePath;
}

void Tabbar::previousTab()
{
    int currentIndex = DTabBar::currentIndex();

    if (currentIndex <= 0) {
        DTabBar::setCurrentIndex(DTabBar::count() - 1);
    } else {
        DTabBar::setCurrentIndex(currentIndex - 1);
    }
}

void Tabbar::nextTab()
{
    int currentIndex = DTabBar::currentIndex();

    if (currentIndex >= DTabBar::count() - 1) {
        DTabBar::setCurrentIndex(0);
    } else {
        DTabBar::setCurrentIndex(currentIndex + 1);
    }
}

int Tabbar::indexOf(const QString &filePath)
{
    return m_tabPaths.indexOf(filePath);
}

QString Tabbar::currentName() const
{
    return DTabBar::tabText(currentIndex());
}

QString Tabbar::currentPath() const
{
    return m_tabPaths.value(currentIndex());
}

QString Tabbar::fileAt(int index) const
{
    return m_tabPaths.value(index);
}

QString Tabbar::textAt(int index) const
{
    return DTabBar::tabText(index);
}

void Tabbar::setTabActiveColor(const QString &color)
{
    QPalette pa = this->palette();
    pa.setColor(QPalette::Active, QPalette::Text, QColor(color));
    setPalette(pa);
}

void Tabbar::setBackground(const QString &startColor, const QString &endColor)
{
    m_backgroundStartColor = startColor;
    m_backgroundEndColor = endColor;
}

void Tabbar::setDNDColor(const QString &startColor, const QString &endColor)
{
    m_dndStartColor = startColor;
    m_dndEndColor = endColor;
}

QPixmap Tabbar::createDragPixmapFromTab(int index, const QStyleOptionTab &option, QPoint *hotspot) const
{
    const qreal ratio = qApp->devicePixelRatio();

    DTextEdit *textEdit = static_cast<Window *>(this->window())->getTextEditor(fileAt(index));
    int width = textEdit->width() * ratio;
    int height = textEdit->height() * ratio;
    QImage screenshotImage(width, height, QImage::Format_ARGB32_Premultiplied);
    screenshotImage.setDevicePixelRatio(ratio);
    textEdit->render(&screenshotImage, QPoint(), QRegion(0, 0, width, height));

    // scaled image to smaller.
    int scaledWidth = width * ratio / 5;
    int scaledHeight = height * ratio / 5;
    auto scaledImage = screenshotImage.scaled(scaledWidth, scaledHeight, Qt::IgnoreAspectRatio, Qt::SmoothTransformation);

    // clip screenshot image with window radius.
    QPainter painter(&scaledImage);
    painter.setRenderHint(QPainter::Antialiasing, true);

    QPainterPath rectPath;
    QPainterPath roundedRectPath;

    rectPath.addRect(0, 0, scaledWidth, scaledHeight);
    roundedRectPath.addRoundedRect(QRect(0, 0, scaledWidth / ratio, scaledHeight / ratio), 6, 6);

    rectPath -= roundedRectPath;

    painter.setCompositionMode(QPainter::CompositionMode_Source);
    painter.fillPath(rectPath, Qt::transparent);
    painter.end();

    if (count() == 1) {
        this->window()->hide();
    }

    // adjust offset.
    hotspot->setX(20);
    hotspot->setY(20);

    QColor shadowColor = QColor("#000000");
    shadowColor.setAlpha(80);

    return Utils::dropShadow(QPixmap::fromImage(scaledImage), 40, shadowColor, QPoint(0, 8));
}

QMimeData* Tabbar::createMimeDataFromTab(int index, const QStyleOptionTab &option) const
{
    const QString tabName = textAt(index);

    Window *window = static_cast<Window *>(this->window());
    EditorBuffer *buffer = window->buffer(fileAt(index));
    QMimeData *mimeData = new QMimeData;

    mimeData->setData("dedit/tabbar", tabName.toUtf8());
    mimeData->setUserData(0, (QObjectUserData *)buffer);
    mimeData->removeFormat("text/plain");

    return mimeData;
}

void Tabbar::insertFromMimeDataOnDragEnter(int index, const QMimeData *source)
{
    const QString tabName = QString::fromUtf8(source->data("dedit/tabbar"));

    EditorBuffer *buffer = (EditorBuffer *)source->userData(0);
    Window *window = static_cast<Window *>(this->window());

    window->addTabWithBuffer(buffer, buffer->textEditor()->filepath, tabName, index);
    window->focusActiveEditor();
}

void Tabbar::insertFromMimeData(int index, const QMimeData *source)
{
    const QString tabName = QString::fromUtf8(source->data("dedit/tabbar"));

    EditorBuffer *buffer = (EditorBuffer *)source->userData(0);
    Window *window = static_cast<Window *>(this->window());

    window->addTabWithBuffer(buffer, buffer->textEditor()->filepath, tabName, index);
    window->focusActiveEditor();
}

bool Tabbar::canInsertFromMimeData(int index, const QMimeData *source) const
{
    return source->hasFormat("dedit/tabbar");
}

void Tabbar::handleDragActionChanged(Qt::DropAction action)
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
}

bool Tabbar::eventFilter(QObject *, QEvent *event)
{
    if (event->type() == QEvent::DragEnter) {
        const QDragEnterEvent *e = static_cast<QDragEnterEvent*>(event);
        const QMimeData* mimeData = e->mimeData();

        if ((!e->source() || e->source()->parent() != this) && mimeData->data("dedit/tabbar") != "") {
            static_cast<Window*>(this->window())->changeTitlebarBackground(m_dndStartColor, m_dndEndColor);
        }
    } else if (event->type() == QEvent::DragLeave) {
        static_cast<Window*>(this->window())->changeTitlebarBackground(m_backgroundStartColor, m_backgroundEndColor);
    } else if (event->type() == QEvent::Drop) {
        static_cast<Window*>(this->window())->changeTitlebarBackground(m_backgroundStartColor, m_backgroundEndColor);
    } else if (event->type() == QEvent::DragMove) {
        event->accept();
    }

    return false;
}

void Tabbar::handleTabMoved(int fromIndex, int toIndex)
{
    m_tabPaths.swap(fromIndex, toIndex);
}

void Tabbar::handleTabReleased(int index)
{
    const QString tabPath = fileAt(index);
    const QString tabName = textAt(index);

    Window *window = static_cast<Window *>(this->window());
    EditorBuffer *buffer = window->buffer(tabPath);
    StartManager::instance()->createWindowFromBuffer(tabName, tabPath, buffer);

    closeTab(index);

    // remove buffer from window, not delete.
    window->removeBuffer(tabPath, false);
}

void Tabbar::handleTabDroped(int index, Qt::DropAction, QObject *target)
{
    Tabbar *tabbar = qobject_cast<Tabbar *>(target);

    if (tabbar == nullptr) {
        QWidget *window = this->window();
        window->show();
        //window->move(QCursor::pos() - window->topLevelWidget()->pos());
    } else {
        closeTab(index);
    }
}
