#include "warningnotices.h"


WarningNotices::WarningNotices(MessageType notifyType)
    : DFloatingMessage(notifyType)
{

    setIcon(QIcon(":/images/warning.svg"));
    m_reloadBtn = new QPushButton(tr("Reload"));
    m_saveAsBtn = new QPushButton(qApp->translate("Window", "Save as"));

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
