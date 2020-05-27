#include "showflodcodewidget.h"

ShowFlodCodeWidget::ShowFlodCodeWidget(DWidget *parent)
    : DWidget(parent)
{
    QVBoxLayout *pMainLayout = new QVBoxLayout(this);
    m_pDArrowRectangle = new DFrame(this);
    QVBoxLayout *pSubLayout = new QVBoxLayout(this);
    m_pContentEdit = new DTextEdit(this);
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
        int newwidth = document->size().width() + 10;//10
        int newheight = document->size().height() + 20;//20
        if (newwidth != m_pContentEdit->width()) {
            m_pContentEdit->setFixedWidth(newwidth);

        }
        if (newheight != m_pContentEdit->height()) {
            m_pContentEdit->setFixedHeight(newheight);

        }

    }

}

void ShowFlodCodeWidget::appendText(QString strText)
{
    m_pContentEdit->append(strText);
    //m_pContentLabel->textCursor().movePosition(QTextCursor::NoMove);
}

