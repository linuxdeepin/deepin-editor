// SPDX-FileCopyrightText: 2017 - 2022 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include "themepanel.h"
#include "../widgets/window.h"

#include <QPropertyAnimation>
#include <QScroller>
#include <QVBoxLayout>
#include <QPainter>
#include <QTimer>

ThemePanel::ThemePanel(QWidget *parent)
    : QWidget(parent),
      m_themeView(new ThemeListView),
      m_themeModel(new ThemeListModel),
      m_window(static_cast<Window *>(parent))
{
    qDebug() << "ThemePanel constructor";
    // init view.
    m_themeView->setModel(m_themeModel);
    m_themeView->setItemDelegate(new ThemeItemDelegate(this));

    QScroller::grabGesture(m_themeView, QScroller::TouchGesture);

    // init layout.
    QVBoxLayout *layout = new QVBoxLayout(this);
    layout->addWidget(m_themeView);

    layout->setContentsMargins(0, 0, 0, 0);
    layout->setSpacing(0);

    setFixedWidth(250);

    QWidget::hide();
    connect(m_themeView, &ThemeListView::focusOut, this, &ThemePanel::hide);
    connect(m_themeView, &ThemeListView::themeChanged, this, &ThemePanel::themeChanged);
    connect(m_themeModel, &ThemeListModel::requestCurrentIndex, this, [=] (const QModelIndex &idx) {
        m_themeView->setCurrentIndex(idx);
        m_themeView->scrollTo(idx);
    });
    qDebug() << "ThemePanel initialized";
}

ThemePanel::~ThemePanel()
{
    qDebug() << "ThemePanel destructor";
}

void ThemePanel::paintEvent(QPaintEvent *e)
{
    QPainter painter(this);

    QPainterPath backgroundPath;
    backgroundPath.addRect(QRect(rect().x() + 1, rect().y(), rect().width() - 1, rect().height()));
    painter.setOpacity(0.9);
    painter.fillPath(backgroundPath, m_backgroundColor);

    QPainterPath separatorPath;
    separatorPath.addRect(QRect(rect().x(), rect().y(), 1, rect().height()));
    painter.setOpacity(0.1);
    painter.fillPath(separatorPath, m_frameColor);
}
void ThemePanel::setBackground(const QString &color)
{
    qDebug() << "Setting panel background color:" << color;
    m_backgroundColor = QColor(color);

    if (m_backgroundColor.lightness() < 128) {
        qDebug() << "Background color is dark";
        m_frameColor = m_frameDarkColor;
    } else {
        qDebug() << "Background color is light";
        m_frameColor = m_frameLightColor;
    }

    update();
    qDebug() << "Panel background color set";
}

void ThemePanel::popup()
{
    qDebug() << "Showing theme panel with animation";
    QWidget::show();
    QWidget::raise();

    m_themeView->setFocus();

    QRect rect = geometry();
    QRect windowRect = m_window->geometry();
    QPropertyAnimation *animation = new QPropertyAnimation(this, "geometry");
    animation->setDuration(250);
    animation->setEasingCurve(QEasingCurve::OutQuad);
    animation->setStartValue(QRect(windowRect.width(), rect.y(), rect.width(), rect.height()));
    animation->setEndValue(QRect(windowRect.width() - rect.width(), rect.y(), rect.width(), rect.height()));
    animation->start();

    connect(animation, &QPropertyAnimation::valueChanged, this, [=] { m_themeView->adjustScrollbarMargins(); });
    connect(animation, &QPropertyAnimation::finished, animation, &QPropertyAnimation::deleteLater);
    qDebug() << "popup end";
}

void ThemePanel::hide()
{
    qDebug() << "Hiding theme panel with animation";
    QRect rect = geometry();
    QRect windowRect = m_window->geometry();
    QPropertyAnimation *animation = new QPropertyAnimation(this, "geometry");
    animation->setDuration(250);
    animation->setEasingCurve(QEasingCurve::OutQuad);
    animation->setStartValue(QRect(windowRect.width() - rect.width(), rect.y(), rect.width(), rect.height()));
    animation->setEndValue(QRect(windowRect.width(), rect.y(), rect.width(), rect.height()));

    animation->start();

    connect(animation, &QPropertyAnimation::finished, this, &QWidget::hide);
    connect(animation, &QPropertyAnimation::finished, animation, &QPropertyAnimation::deleteLater);
    qDebug() << "hide end";
}

void ThemePanel::setFrameColor(const QString &selectedColor, const QString &normalColor)
{
    qDebug() << "Setting frame color:" << selectedColor << normalColor;
    m_themeModel->setFrameColor(selectedColor, normalColor);
}

void ThemePanel::setSelectionTheme(const QString &path)
{
    qDebug() << "Setting selected theme path:" << path;
    m_themeModel->setSelection(path);
}
