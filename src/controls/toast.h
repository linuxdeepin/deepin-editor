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

#ifndef TOAST_H
#define TOAST_H

#include <QFrame>
#include <QLabel>
#include <QIcon>
#include <QGraphicsOpacityEffect>
#include <QPropertyAnimation>
#include <QPushButton>

#include "dhidpihelper.h"
#include "dthememanager.h"
#include "dimagebutton.h"
#include "dgraphicsgloweffect.h"
#include <QHBoxLayout>

DWIDGET_USE_NAMESPACE

class Toast : public QFrame
{
    Q_OBJECT
    Q_PROPERTY(qreal opacity READ opacity WRITE setOpacity)

public:
    Toast(QWidget *parent = nullptr);
    ~Toast();

    void pop();
    void pack();

    void setOnlyShow(bool onlyshow);
    void setText(QString text);
    void setIcon(QString icon);

    void showAnimation();
    void hideAnimation();

    void setReloadState(bool enable);

signals:
    void visibleChanged(bool visible);
    void reloadBtnClicked();
    void closeBtnClicked();
    void saveAsBtnClicked();

private:
    void setTheme(QString theme);

    qreal opacity() const;
    void setOpacity(qreal opacity);

    void initCloseBtn(QString theme);

    void showEvent(QShowEvent *);
    void hideEvent(QHideEvent *);

private:
    QHBoxLayout *m_layout;
    QLabel *m_iconLabel;
    QLabel *m_textLabel;
    int m_duration;
    bool m_onlyshow;

    QPropertyAnimation *m_animation = nullptr;
    DGraphicsGlowEffect *m_effect = nullptr;
    DImageButton *m_closeBtn;
    QPushButton *m_reloadBtn;
    QPushButton *m_saveAsBtn;
};

#endif
