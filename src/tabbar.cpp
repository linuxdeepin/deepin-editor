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
#include "editwrapper.h"
#include "startmanager.h"

#include <QApplication>
#include <QStyleFactory>
#include <QGuiApplication>
#include <DPlatformWindowHandle>
#include <DWindowManagerHelper>

Tabbar::Tabbar(QWidget *parent)
    : DTabBar(parent)
{
    m_rightClickTab = -1;

    installEventFilter(this);

    setMovable(true);
    setTabsClosable(true);
    setVisibleAddButton(true);
    setDragable(true);
   // setStartDragDistance(40);
    setElideMode(Qt::ElideMiddle);

    setFocusPolicy(Qt::NoFocus);

    connect(this, &DTabBar::tabMoved, this, &Tabbar::handleTabMoved);
    connect(this, &DTabBar::tabDroped, this, &Tabbar::handleTabDroped);
    connect(this, &DTabBar::tabIsRemoved, this, &Tabbar::handleTabIsRemoved);
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

 //   if (!m_tabPaths.contains(filePath)) {
        m_tabPaths.insert(index, filePath);
//    }

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
    QStringList closePathList;

    for (const QString &path : m_tabPaths) {
        if (filePath != path) {
            closePathList << path;
        }
    }

    emit closeTabs(closePathList);
}

void Tabbar::closeLeftTabs(const QString &filePath)
{
    QStringList closePathList;

    for (const QString &path :m_tabPaths) {
        if(filePath == path) {
            break;
        }
        closePathList << path;
    }
    emit closeTabs(closePathList);

}

void Tabbar::closeRightTabs(const QString &filePath)
{

    QStringList closePathlist;


    for (int i=m_tabPaths.count()-1;i >= 0;i--) {
        m_tabPaths.value(i);

        if(filePath == m_tabPaths.value(i)) {
            break;
        }
        closePathlist << m_tabPaths.value(i);
    }
    emit closeTabs(closePathlist);


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

    TextEdit *textEdit = static_cast<Window *>(this->window())->getTextEditor(fileAt(index));
    int width = textEdit->width() * ratio;
    int height = textEdit->height() * ratio;
    QImage screenshotImage(width, height, QImage::Format_ARGB32_Premultiplied);
    screenshotImage.setDevicePixelRatio(ratio);
    textEdit->render(&screenshotImage, QPoint(), QRegion(0, 0, width, height));

    // scaled image to smaller.
    int scaledWidth = width * ratio / 5;
    int scaledHeight = height * ratio / 5;
    auto scaledImage = screenshotImage.scaled(scaledWidth, scaledHeight, Qt::IgnoreAspectRatio, Qt::SmoothTransformation);

    QImage backgroundImage(scaledWidth + 10, scaledHeight + 10, QImage::Format_ARGB32_Premultiplied);
    backgroundImage.fill(QColor(palette().color(QPalette::Base)));
    // clip screenshot image with window radius.
    QPainter painter(&backgroundImage);
    painter.drawImage(5,5,scaledImage);
    painter.setRenderHint(QPainter::Antialiasing, true);

    if (count() == 1) {
        this->window()->hide();
    }

    // adjust offset.
    hotspot->setX(20);
    hotspot->setY(20);

    QPainterPath rectPath;

    if(DWindowManagerHelper::instance()->hasComposite())
    {
        QPainterPath roundedRectPath;

        rectPath.addRect(0, 0, scaledWidth + 10, scaledHeight + 10);
        roundedRectPath.addRoundedRect(QRect(0, 0, scaledWidth / ratio + 10, scaledHeight / ratio + 10), 6, 6);

        rectPath -= roundedRectPath;

        painter.setCompositionMode(QPainter::CompositionMode_Source);
        painter.fillPath(rectPath, Qt::transparent);

        QColor shadowColor = QColor(palette().color(QPalette::BrightText));
        shadowColor.setAlpha(80);

        painter.end();

        return Utils::dropShadow(QPixmap::fromImage(backgroundImage), 5, shadowColor, QPoint(0, 0));
    } else {
          painter.end();

          return QPixmap::fromImage(backgroundImage);
    }

}

QMimeData* Tabbar::createMimeDataFromTab(int index, const QStyleOptionTab &option) const
{
    const QString tabName = textAt(index);

    Window *window = static_cast<Window *>(this->window());
    EditWrapper *wrapper = window->wrapper(fileAt(index));
    QMimeData *mimeData = new QMimeData;

    if (!wrapper) {
        //m_tabbar->closeCurrentTab();
        return nullptr;
    }

    mimeData->setProperty("wrapper", QVariant::fromValue(static_cast<void *>(wrapper)));
    mimeData->setProperty("isModified", wrapper->textEditor()->document()->isModified());
    mimeData->setData("dedit/tabbar", tabName.toUtf8());
    mimeData->removeFormat("text/plain");

    return mimeData;
}

void Tabbar::insertFromMimeDataOnDragEnter(int index, const QMimeData *source)
{
    if (source == nullptr) {
        return;
    }

    const QString tabName = QString::fromUtf8(source->data("dedit/tabbar"));

    QVariant pVar = source->property("wrapper");
    EditWrapper *wrapper = static_cast<EditWrapper *>(pVar.value<void *>());
    Window *window = static_cast<Window *>(this->window());

    if (!wrapper) {
        return;
    }

    window->addTabWithWrapper(wrapper, wrapper->textEditor()->filepath, tabName, index);
    //window->currentWrapper()->textEditor()->setModified(source->property("isModified").toBool());
    wrapper->textEditor()->setModified(source->property("isModified").toBool());
    window->focusActiveEditor();
}

void Tabbar::insertFromMimeData(int index, const QMimeData *source)
{
    if (source == nullptr) {
        return;
    }

    const QString tabName = QString::fromUtf8(source->data("dedit/tabbar"));

    QVariant pVar = source->property("wrapper");
    EditWrapper *wrapper = static_cast<EditWrapper *>(pVar.value<void *>());
    Window *window = static_cast<Window *>(this->window());

    if (!wrapper) {
        return;
    }

    window->addTabWithWrapper(wrapper, wrapper->textEditor()->filepath, tabName, index);
    //window->currentWrapper()->textEditor()->setModified(source->property("isModified").toBool());
    wrapper->textEditor()->setModified(source->property("isModified").toBool());
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

            // popup right menu on tab.
            if (m_rightClickTab >= 0) {
                m_rightMenu = new DMenu;
                //m_rightMenu->setStyle(QStyleFactory::create("dlight"));

                m_closeTabAction = new QAction(tr("Close tab"), this);
                m_closeOtherTabAction = new QAction(tr("Close other tabs"), this);
                m_moreWaysCloseMenu = new DMenu(tr("More options"),this);
                m_closeLeftTabAction = new QAction(tr("Close tabs to the left"),this);
                m_closeRightTabAction = new QAction(tr("Close tabs to the right"),this);
                m_closeAllunModifiedTabAction = new QAction(tr("Close unmodified tabs"),this);

                m_moreWaysCloseMenu->addAction(m_closeLeftTabAction);
                m_moreWaysCloseMenu->addAction(m_closeRightTabAction);
                m_moreWaysCloseMenu->addAction(m_closeAllunModifiedTabAction);

                if(m_tabPaths.length()<2) {
                    m_closeOtherTabAction->setEnabled(false);
                    m_closeLeftTabAction->setEnabled(false);
                    m_closeRightTabAction->setEnabled(false);
                }

                showTabs();
                connect(m_closeTabAction, &QAction::triggered, this, [=] {
                    Q_EMIT tabCloseRequested(m_rightClickTab);
                });

                connect(m_closeOtherTabAction, &QAction::triggered, this, [=] {
                    closeOtherTabsExceptFile(fileAt(m_rightClickTab));
                });

                connect(m_closeLeftTabAction, &QAction::triggered,this,[=]{
                    closeLeftTabs(fileAt(m_rightClickTab));
//                    int currentIndex = DTabBar::currentIndex();
//                    if(currentIndex>=0) {
//                        for (int i=0;i<currentIndex;i++) {
//                            closeTab(0);
//                        }
//                    }
                });

                connect(m_closeRightTabAction,&QAction::triggered,this,[=]{
                    closeRightTabs(fileAt(m_rightClickTab));
//                    int currentIndex = DTabBar::currentIndex();
//                    int count = DTabBar::count();
//                    if(currentIndex>=0){
//                        for (int i=count;i>currentIndex;i--) {
//                            closeTab(currentIndex+1);
//                        }
//                    }
                });

                connect(m_closeAllunModifiedTabAction,&QAction::triggered,this,[=] {
                    Window *window = static_cast<Window *>(this->window());//确定在哪个窗口关闭
//                    for (int i=0;i<m_tabPaths.count();i++) {
//                        EditWrapper *wrapper = window->wrapper(m_tabPaths.value(i));
//                        if (!wrapper->textEditor()->document()->isModified()){
//                            closeTab(this->indexOf(m_tabPaths.value(i)));
//                        }
//                    }
                                        for ( auto path : m_tabPaths) {
                                            EditWrapper *wrapper = window->wrapper(path);//路径获取文件
                                            if (!wrapper->textEditor()->document()->isModified()){
                                                window->removeWrapper(path,true);
                                                closeTab(this->indexOf(path));
                                            }
                                        }

                });

                m_rightMenu->addAction(m_closeTabAction);
                m_rightMenu->addAction(m_closeOtherTabAction);
                m_rightMenu->addMenu(m_moreWaysCloseMenu);
                //yanyuhan 只有一个标签页时不显示更多方式关闭
                if(m_tabPaths.size() > 1) {
                    m_moreWaysCloseMenu->setEnabled(true);
                } else {
                    m_moreWaysCloseMenu->setEnabled(false);
                }

                m_rightMenu->exec(mapToGlobal(position));

                return true;
            }
        }

    } else if (event->type() == QEvent::DragEnter) {
        const QDragEnterEvent *e = static_cast<QDragEnterEvent*>(event);
        const QMimeData* mimeData = e->mimeData();

        if ((!e->source() || e->source()->parent() != this) &&
            mimeData->hasFormat("dedit/tabbar")) {
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

void Tabbar::mousePressEvent(QMouseEvent *e)
{
    if(e->button()==Qt::MidButton)
    {
        emit tabCloseRequested(tabAt(QPoint(e->x(), e->y())));
    }
    DTabBar::mousePressEvent(e);
}

void Tabbar::handleTabMoved(int fromIndex, int toIndex)
{  
    if (m_tabPaths.count() > fromIndex && m_tabPaths.count() > toIndex && fromIndex >= 0 && toIndex >= 0) {
        m_tabPaths.swap(fromIndex, toIndex);
    }
}
void Tabbar::showTabs()
{
    int currentIndex  =  DTabBar::currentIndex();
    if (currentIndex<=0)  {
        m_closeLeftTabAction->setEnabled(false);
    }
    if (currentIndex >= DTabBar::count() - 1) {
        m_closeRightTabAction->setEnabled(false);
    }
}

void Tabbar::handleTabReleased(int index)
{
    const QString tabPath = fileAt(index);
    const QString tabName = textAt(index);

    Window *window = static_cast<Window *>(this->window());
    EditWrapper *wrapper = window->wrapper(tabPath);
    StartManager::instance()->createWindowFromWrapper(tabName, tabPath, wrapper, wrapper->textEditor()->document()->isModified());

    closeTab(index);

    // remove wrapper from window, not delete.
    window->removeWrapper(tabPath, false);
}

void Tabbar::handleTabIsRemoved(int index)
{
    const QString filePath = m_tabPaths.at(index);
    Window *window = static_cast<Window *>(this->window());

    m_tabPaths.removeAt(index);
    window->removeWrapper(filePath, false);
}

void Tabbar::handleTabDroped(int index, Qt::DropAction, QObject *target)
{
    Tabbar *tabbar = qobject_cast<Tabbar *>(target);

    if (tabbar == nullptr) {
        Window *window = static_cast<Window *>(this->window());
        window->move(QCursor::pos() - window->topLevelWidget()->pos());
        window->show();
        window->activateWindow();
    } else {
        closeTab(index);
    }
}
