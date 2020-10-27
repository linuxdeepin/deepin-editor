#include "warningnotices.h"


WarningNotices::WarningNotices(MessageType notifyType)
    : DFloatingMessage(notifyType)
{
    QFont font;
    font.setPixelSize(14);
    font.setFamily("SourceHanSansSC-Medium");
    this->setFont(font);

    this->setContentsMargins(7, 7, 7, 7);

    setIcon(QIcon(":/images/warning.svg"));
    m_reloadBtn = new QPushButton(tr("Reload"));
    //m_reloadBtn->setFixedSize(80, 36);
    m_saveAsBtn = new QPushButton(qApp->translate("Window", "Save as"));
    //m_saveAsBtn->setFixedSize(80, 36);
    m_reloadBtn->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);
    m_saveAsBtn->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);

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
    if (!m_reloadBtn->isVisible()) {
        m_reloadBtn->setVisible(true);
    }

    m_saveAsBtn->setVisible(false);
    setWidget(m_reloadBtn);
}

void WarningNotices::setSaveAsBtn()
{
    if (!m_saveAsBtn->isVisible()) {
        m_saveAsBtn->setVisible(true);
    }

    m_reloadBtn->setVisible(false);
    setWidget(m_saveAsBtn);
}
