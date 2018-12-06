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

#include "ddropdownmenu.h"
#include "../utils.h"
#include <QHBoxLayout>
#include <QMouseEvent>

DDropdownMenu::DDropdownMenu(QWidget *parent)
    : QFrame(parent),
      m_menu(new QMenu),
      m_text(new QLabel("undefined")),
      m_arrowLabel(new QLabel)
{
    QHBoxLayout *layout = new QHBoxLayout(this);
    layout->setContentsMargins(15, 0, 15, 0);
    layout->setSpacing(0);

    m_arrowLabel->setFixedSize(9, 5);
    QPixmap arrowPixmap = Utils::renderSVG(":/images/dropdown_arrow_light.svg", QSize(9, 5));
    m_arrowLabel->setPixmap(arrowPixmap);

    layout->addWidget(m_text, 0, Qt::AlignHCenter);
    layout->addSpacing(5);
    layout->addWidget(m_arrowLabel);

    connect(m_menu, &QMenu::triggered, this, [=] (QAction *action) {
        setText(action->text());
        setCurrentAction(action);
        Q_EMIT this->triggered(action);
        Q_EMIT this->currentTextChanged(action->text());
    });

    connect(this, &DDropdownMenu::requestContextMenu, this, [=] {
        auto center = this->mapToGlobal(this->rect().topLeft());
        center.setY(center.y() + this->height() + 5);
        m_menu->move(center);
        m_menu->exec();
    });
}

DDropdownMenu::~DDropdownMenu()
{
}

QList<QAction *> DDropdownMenu::actions() const
{
    return m_menu->actions();
}

QAction *DDropdownMenu::addAction(const QString &text)
{
    QAction *action = m_menu->addAction(text);
    action->setCheckable(true);
    setText(action->text());
    return action;
}

void DDropdownMenu::addActions(QStringList list)
{
    for (QString text : list) {
        QAction *action = m_menu->addAction(text);
        action->setCheckable(true);
        setText(action->text());
    }
}

void DDropdownMenu::setCurrentAction(QAction *action)
{
    if (action) {
        for (QAction *action : m_menu->actions()) {
            action->setChecked(false);
        }

        m_text->setText(action->text());
        action->setChecked(true);
    } else {
        for (QAction *action : m_menu->actions()) {
            action->setChecked(false);
        }
    }
}

void DDropdownMenu::setCurrentText(const QString &text)
{
    for (QAction *action : m_menu->actions()) {
        if (action->text() == text) {
            setCurrentAction(action);
            setText(text);
        }
    }
}

void DDropdownMenu::setText(const QString &text)
{
    QFontMetrics fm(font());
    m_text->setText(text);
    setFixedWidth(fm.width(text) + 50);
}

void DDropdownMenu::setTheme(const QString &theme)
{
    QString arrowSvgPath = QString(":/images/dropdown_arrow_%1.svg").arg(theme);
    QPixmap arrowPixmap = Utils::renderSVG(arrowSvgPath, QSize(9, 5));
    m_arrowLabel->setPixmap(arrowPixmap);
}

void DDropdownMenu::mouseReleaseEvent(QMouseEvent *e)
{
    if (e->button() == Qt::LeftButton) {
        Q_EMIT requestContextMenu();
    }

    QFrame::mouseReleaseEvent(e);
}
