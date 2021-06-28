/*
* Copyright (C) 2019 ~ 2021 Uniontech Software Technology Co.,Ltd.
*
* Author:     guoshaoyu <guoshaoyu@uniontech.com>
*
* Maintainer: guoshaoyu <guoshaoyu@uniontech.com>
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

#include "warningnotices.h"
#include <QDebug>

WarningNotices::WarningNotices(MessageType notifyType, QWidget *parent)
    : DFloatingMessage(notifyType,parent)
{
    QFont font;
    font.setPixelSize(14);
    font.setFamily("SourceHanSansSC-Medium");
    this->setFont(font);

    this->setContentsMargins(7, 7, 7, 7);

    setIcon(QIcon(":/images/warning.svg"));
    m_reloadBtn = new QPushButton(tr("Reload"),this);
    //m_reloadBtn->setFixedSize(80, 36);
    m_saveAsBtn = new QPushButton(qApp->translate("Window", "Save as"),this);

    //m_saveAsBtn->setFixedSize(80, 36);
    m_reloadBtn->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);
    m_saveAsBtn->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);

    connect(m_reloadBtn, &QPushButton::clicked, this, &WarningNotices::slotreloadBtnClicked);
    connect(m_saveAsBtn, &QPushButton::clicked, this, &WarningNotices::slotsaveAsBtnClicked);
}

WarningNotices::~WarningNotices()
{

}

void WarningNotices::setReloadBtn()
{
    if (!m_reloadBtn->isVisible()) {
        m_reloadBtn->setVisible(true);
    }
    m_reloadBtn->setVisible(true);
    m_saveAsBtn->setVisible(false);
    setWidget(m_reloadBtn);
}

void WarningNotices::setSaveAsBtn()
{
    if (!m_saveAsBtn->isVisible()) {
        m_saveAsBtn->setVisible(true);
    }
    m_saveAsBtn->setVisible(true);
    m_reloadBtn->setVisible(false);
    setWidget(m_saveAsBtn);
}

void WarningNotices::slotreloadBtnClicked()
{
    this->hide();
    emit reloadBtnClicked();
}

void WarningNotices::slotsaveAsBtnClicked()
{
    this->hide();
    emit saveAsBtnClicked();
}
