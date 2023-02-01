// SPDX-FileCopyrightText: 2019 - 2022 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

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

void WarningNotices::clearBtn()
{
    m_saveAsBtn->setVisible(false);
    m_reloadBtn->setVisible(false);
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
