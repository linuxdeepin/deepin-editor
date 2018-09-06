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

#include "themepanel.h"
#include "../window.h"
#include <QVBoxLayout>
#include <QPainter>
#include <QTimer>

ThemePanel::ThemePanel(QWidget *parent)
    : QWidget(parent),
      m_themeView(new ThemeListView),
      m_themeModel(new ThemeListModel),
      m_animation(new QPropertyAnimation(this, "geometry"))
{
    // init view.
    m_themeView->setModel(m_themeModel);
    m_themeView->setItemDelegate(new ThemeItemDelegate);

    // init animation.
    m_animation->setDuration(250);
    m_animation->setEasingCurve(QEasingCurve::OutQuad);

    // init layout.
    QVBoxLayout *layout = new QVBoxLayout(this);
    layout->addWidget(m_themeView);

    layout->setMargin(0);
    layout->setSpacing(0);

    QWidget::hide();
    connect(m_themeView, &ThemeListView::focusOut, this, &ThemePanel::hide);
    connect(m_themeView, &ThemeListView::themeChanged, this, &ThemePanel::themeChanged);
    connect(m_themeModel, &ThemeListModel::requestCurrentIndex, this, [=] (const QModelIndex &idx) {
        m_themeView->setCurrentIndex(idx);
        m_themeView->scrollTo(idx);
    });
}

ThemePanel::~ThemePanel()
{
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
    m_backgroundColor = QColor(color);

    if (m_backgroundColor.lightness() < 128) {
        m_frameColor = m_frameDarkColor;
    } else {
        m_frameColor = m_frameLightColor;
    }

    update();
}

void ThemePanel::popup()
{
    QWidget::show();
    QWidget::raise();

    m_themeView->setFocus();

    QRect rect = geometry();
    m_animation->setStartValue(QRect(rect.x(), rect.y(), 0, rect.height()));
    m_animation->setEndValue(QRect(rect.x(), rect.y(), 250, rect.height()));
    m_animation->start();
}

void ThemePanel::hide()
{
    QRect rect = geometry();
    m_animation->setStartValue(QRect(rect.x(), rect.y(), 250, rect.height()));
    m_animation->setEndValue(QRect(rect.x(), rect.y(), 0, rect.height()));
    m_animation->start();

    QTimer::singleShot(250, this, [=] { QWidget::hide(); });
}

void ThemePanel::setFrameColor(const QString &selectedColor, const QString &normalColor)
{
    m_themeModel->setFrameColor(selectedColor, normalColor);
}

void ThemePanel::setSelectionTheme(const QString &path)
{
    m_themeModel->setSelection(path);
}
