// SPDX-FileCopyrightText: 2019 - 2026 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include "ddlog.h"
#include "warningnotices.h"

#include <DGuiApplicationHelper>
#include <DDialogCloseButton>
#include <QDebug>

#ifdef DTKWIDGET_CLASS_DSizeMode
#include <DSizeMode>
#endif

const int s_WNCloseBtnSize = 32;
const int s_WNCloseBtnSizeCompact = 26;

WarningNotices::WarningNotices(MessageType notifyType, QWidget *parent)
    : DFloatingMessage(notifyType, parent)
{
    qCDebug(appLog) << "WarningNotices constructor start, type:" << notifyType;
    this->setFont(qApp->font());

    setIcon(QIcon(":/images/warning.svg"));
    m_reloadBtn = new QPushButton(tr("Reload"), this);
    m_reloadBtn->setObjectName("ReloadBtn");
    m_reloadBtn->setAccessibleName("ReloadFile");
    m_saveAsBtn = new QPushButton(qApp->translate("Window", "Save as"), this);
    m_saveAsBtn->setObjectName("SaveAsBtn");
    m_saveAsBtn->setAccessibleName("WarningSaveAs");
    m_editAnywayBtn = new QPushButton(tr("Edit Anyway"), this);
    m_editAnywayBtn->setObjectName("EditAnywayBtn");
    m_editAnywayBtn->setAccessibleName("EditAnyway");
    m_reloadBtn->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);
    m_saveAsBtn->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);
    m_editAnywayBtn->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);
    m_editAnywayBtn->setVisible(false);
    qCDebug(appLog) << "Warning buttons initialized";

    connect(m_reloadBtn, &QPushButton::clicked, this, &WarningNotices::slotreloadBtnClicked);
    connect(m_saveAsBtn, &QPushButton::clicked, this, &WarningNotices::slotsaveAsBtnClicked);
    connect(m_editAnywayBtn, &QPushButton::clicked, this, &WarningNotices::slotEditAnywayBtnClicked);
    qCDebug(appLog) << "Warning button signals connected";

#ifdef DTKWIDGET_CLASS_DSizeMode
    DDialogCloseButton *closeBtn = findChild<DDialogCloseButton *>();
    if (closeBtn) {
        qCDebug(appLog) << "closeBtn is not null";
        closeBtn->setIconSize(DSizeModeHelper::element(QSize(26, 26), QSize(32, 32)));
        connect(DGuiApplicationHelper::instance(), &DGuiApplicationHelper::sizeModeChanged, this, [=]() {
            closeBtn->setIconSize(DSizeModeHelper::element(QSize(26, 26), QSize(32, 32)));
        });
    }

    // 紧凑模式下字体切换同样处理
    connect(DGuiApplicationHelper::instance(), &DGuiApplicationHelper::fontChanged, this, [this](const QFont &font) {
        this->setFont(font);
    });
#endif

    // TODO: wait dtkwidget fixed, see dtkwidget PR-628
    connect(this, &DFloatingMessage::closeButtonClicked, this, [this]() {
        if (this->isVisible()) {
            qCDebug(appLog) << "closeButtonClicked";
            this->close();
        }
    });
    qCDebug(appLog) << "WarningNotices constructor end";
}

WarningNotices::~WarningNotices()
{
    qCDebug(appLog) << "WarningNotices destructor";
}

void WarningNotices::setReloadBtn()
{
    qCDebug(appLog) << "setReloadBtn";
    if (!m_reloadBtn->isVisible()) {
        qCDebug(appLog) << "m_reloadBtn->setVisible(true)";
        m_reloadBtn->setVisible(true);
    }
    qCDebug(appLog) << "m_reloadBtn->setVisible(true)";
    m_reloadBtn->setVisible(true);
    m_saveAsBtn->setVisible(false);
    setWidget(m_reloadBtn);
    qCDebug(appLog) << "setReloadBtn end";
}

void WarningNotices::setSaveAsBtn()
{
    qCDebug(appLog) << "setSaveAsBtn";
    if (!m_saveAsBtn->isVisible()) {
        m_saveAsBtn->setVisible(true);
    }
    m_saveAsBtn->setVisible(true);
    m_reloadBtn->setVisible(false);
    setWidget(m_saveAsBtn);
    qCDebug(appLog) << "setSaveAsBtn end";
}

void WarningNotices::clearBtn()
{
    qCDebug(appLog) << "clearBtn";
    m_saveAsBtn->setVisible(false);
    m_reloadBtn->setVisible(false);
    qCDebug(appLog) << "clearBtn end";
}

void WarningNotices::slotreloadBtnClicked()
{
    qCDebug(appLog) << "slotreloadBtnClicked";
    this->hide();
    emit reloadBtnClicked();
    qCDebug(appLog) << "slotreloadBtnClicked end";
}

void WarningNotices::slotsaveAsBtnClicked()
{
    qCDebug(appLog) << "slotsaveAsBtnClicked";
    this->hide();
    emit saveAsBtnClicked();
    qCDebug(appLog) << "slotsaveAsBtnClicked end";
}

void WarningNotices::setEditAnywayBtn()
{
    qCDebug(appLog) << "setEditAnywayBtn";
    m_reloadBtn->setVisible(false);
    m_saveAsBtn->setVisible(false);
    m_editAnywayBtn->setVisible(true);
    setWidget(m_editAnywayBtn);
    qCDebug(appLog) << "setEditAnywayBtn end";
}

void WarningNotices::slotEditAnywayBtnClicked()
{
    qCDebug(appLog) << "slotEditAnywayBtnClicked";
    this->hide();
    emit editAnywayBtnClicked();
    qCDebug(appLog) << "slotEditAnywayBtnClicked end";
}
