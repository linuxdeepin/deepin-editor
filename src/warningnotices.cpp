#include "warningnotices.h"


WarningNotices::WarningNotices(MessageType notifyType)
    : DFloatingMessage(notifyType)
{

    setIcon(QIcon(":/images/warning.svg"));
    m_pLayout = new QHBoxLayout();
    m_pLayout->setContentsMargins(10, 0, 10, 0);
    m_reloadBtn = new QPushButton(tr("Reload"));
    m_saveAsBtn = new QPushButton(qApp->translate("Window", "Save as"));
    m_reloadBtn->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);
    m_saveAsBtn->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);
    this->setFixedSize(606, 60);

    connect(m_reloadBtn, &QPushButton::clicked, this, [=] {
        this->hide();
        emit reloadBtnClicked();
    });

    connect(m_saveAsBtn, &QPushButton::clicked, this, [=] {
        this->hide();
        emit saveAsBtnClicked();
    });

}

WarningNotices::~WarningNotices()
{

}

void WarningNotices::setReloadBtn()
{
    setWidget(m_reloadBtn);
}

void WarningNotices::setSaveAsBtn()
{
    setWidget(m_saveAsBtn);
}
