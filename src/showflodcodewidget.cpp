#include "showflodcodewidget.h"

ShowFlodCodeWidget::ShowFlodCodeWidget(DWidget *parent)
    : DWidget(parent)
{
    QVBoxLayout *pMainLayout = new QVBoxLayout();
    m_pDArrowRectangle = new DFrame(this);
    m_pDArrowRectangle->setFrameRounded(true);
    QVBoxLayout *pSubLayout = new QVBoxLayout();
    m_pContentEdit = new DTextEdit(this);
    m_pContentEdit->setWordWrapMode(QTextOption::WordWrap);
    pSubLayout->addWidget(m_pContentEdit);
    m_pDArrowRectangle->setLayout(pSubLayout);
    pMainLayout->addWidget(m_pDArrowRectangle);
    setLayout(pMainLayout);
    connect(m_pContentEdit->document(), SIGNAL(contentsChanged()), this, SLOT(textAreaChanged()));



}

ShowFlodCodeWidget::~ShowFlodCodeWidget()
{

}

void ShowFlodCodeWidget::clear()
{
    m_pContentEdit->document()->clear();
}

void ShowFlodCodeWidget::textAreaChanged()
{
    QTextDocument *document = m_pContentEdit->document();
    document->adjustSize();
    if (document) {
        int newwidth = document->size().width() + 20;//10
        int newheight = document->size().height();
        if (newwidth != m_pContentEdit->width()) {
            m_pContentEdit->setFixedWidth(newwidth);

        }
        if (newheight != m_pContentEdit->height()) {
            m_pContentEdit->setFixedHeight(newheight);

        }
        this->resize(m_pContentEdit->width() + 35, m_pContentEdit->height() + 35);

    }

}

void ShowFlodCodeWidget::appendText(QString strText)
{
    m_pContentEdit->append(strText);
    //m_pContentLabel->textCursor().movePosition(QTextCursor::NoMove);
}

