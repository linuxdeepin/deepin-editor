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

#include "toast.h"

Toast::Toast(QWidget *parent)
    : QFrame(parent)
{
    setWindowFlags(windowFlags() | Qt::WindowStaysOnTopHint);

    m_layout = new QHBoxLayout(this);
    m_layout->setContentsMargins(10, 10, 10, 10);
    m_layout->setSpacing(12);

    m_duration = 2000;
    m_onlyshow = false;

    m_iconLabel = new QLabel;
    m_textLabel = new QLabel;

    m_reloadBtn = new QPushButton(tr("Reload"));

    m_layout->addWidget(m_iconLabel);
    m_layout->addWidget(m_textLabel);

    m_effect = new DGraphicsGlowEffect(this);
    m_effect->setBlurRadius(20.0);
    m_effect->setColor(QColor(0, 0, 0, 255 / 10));
    m_effect->setOffset(0, 0);
    setGraphicsEffect(m_effect);
    hide();

    setTheme(DThemeManager::instance()->theme());
    connect(DThemeManager::instance(), &DThemeManager::themeChanged, this, &Toast::setTheme);
    connect(m_reloadBtn, &QPushButton::clicked, this, [=] {
        hideAnimation();
        emit reloadBtnClicked();
    });
}

Toast::~Toast()
{
}

void Toast::pop()
{
    QWidget::adjustSize();
    QWidget::show();
    QWidget::raise();

    if (m_animation) {
        return;
    }

    int _duration = m_duration < 0 ? 2000 : m_duration;

    m_animation = new QPropertyAnimation(this, "opacity");
    m_animation->setDuration(_duration);
    m_animation->setStartValue(0);
    m_animation->setKeyValueAt(0.4, 1.0);
    m_animation->setKeyValueAt(0.8, 1.0);
    m_animation->setEndValue(0);
    m_animation->start();

    connect(m_animation, &QPropertyAnimation::finished, this, [=] {
        hide();
        m_animation->deleteLater();
        m_animation = nullptr;
    });
}

void Toast::pack()
{
    hide();

    if (m_animation) {
        m_animation->stop();
        m_animation->deleteLater();
        m_animation = nullptr;
    }
}

void Toast::setOnlyShow(bool onlyshow)
{
    m_onlyshow = onlyshow;

    if (m_onlyshow) {
        m_closeBtn = new DImageButton;
        m_closeBtn->setFixedSize(14, 14);
        m_layout->addWidget(m_reloadBtn);
        m_layout->addWidget(m_closeBtn);
        initCloseBtn(DThemeManager::instance()->theme());

        connect(DThemeManager::instance(), &DThemeManager::themeChanged, this, &Toast::initCloseBtn);
        connect(m_closeBtn, &DImageButton::clicked, this, &Toast::hideAnimation);
        connect(m_closeBtn, &DImageButton::clicked, this, &Toast::closeBtnClicked);
    }
}

void Toast::setText(QString text)
{
    m_textLabel->setText(text);
}

void Toast::setIcon(QString icon)
{
    m_iconLabel->setPixmap(DHiDPIHelper::loadNxPixmap(icon));
}

void Toast::showAnimation()
{
    QWidget::adjustSize();
    QWidget::show();
    QWidget::raise();

    QPropertyAnimation *animation = new QPropertyAnimation(this, "opacity");
    animation->setDuration(500);
    animation->setStartValue(0);
    animation->setKeyValueAt(0.4, 1.0);
    animation->setKeyValueAt(0.8, 1.0);
    animation->setEndValue(1.0);
    animation->start();

    connect(animation, &QPropertyAnimation::finished, this, [=] {
        m_animation->deleteLater();
    });
}

void Toast::hideAnimation()
{
    QPropertyAnimation *animation = new QPropertyAnimation(this, "opacity");
    animation->setDuration(500);
    animation->setStartValue(1.0);
    animation->setKeyValueAt(0.8, 0.4);
    animation->setEndValue(0);
    animation->start();

    connect(animation, &QPropertyAnimation::finished, this, [=] {
        animation->deleteLater();
        QWidget::hide();
    });
}

void Toast::setTheme(QString theme)
{
    if (theme == "light") {
        setStyleSheet("Toast {"
                      "background: rgba(255,255,255,100%);"
                      "border: 1px solid rgba(0,0,0,10%);"
                      "border-radius: 4px;"
                      "}");
    } else {
        setStyleSheet("Toast {"
                      "background: rgba(49,49,49,100%);"
                      "border: 1px solid rgba(0,0,0,30%);"
                      "border-radius: 4px;"
                      "}");
    }
}

qreal Toast::opacity() const
{
    return m_effect->opacity();
}

void Toast::setOpacity(qreal opacity)
{
    m_effect->setOpacity(opacity);
    update();
}

void Toast::initCloseBtn(QString theme)
{
    m_closeBtn->setNormalPic(QString(":/images/close_%1_%2.svg").arg("normal", theme));
    m_closeBtn->setHoverPic(QString(":/images/close_%1_%2.svg").arg("hover", theme));
    m_closeBtn->setPressPic(QString(":/images/close_%1_%2.svg").arg("press", theme));
}

void Toast::showEvent(QShowEvent *e)
{
    emit visibleChanged(true);
    QFrame::showEvent(e);
}

void Toast::hideEvent(QHideEvent *e)
{
    emit visibleChanged(false);
    QFrame::hideEvent(e);
}

