#include "codeflodarea.h"
#include "leftareaoftextedit.h"


CodeFlodArea::CodeFlodArea(LeftAreaTextEdit *leftAreaWidget)
{
    m_pLeftAreaWidget = leftAreaWidget;
}

CodeFlodArea::~CodeFlodArea()
{

}

void CodeFlodArea::paintEvent(QPaintEvent *e)
{
    m_pLeftAreaWidget->codeFlodAreaPaintEvent(e);
}

