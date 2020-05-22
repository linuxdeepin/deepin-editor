#include "codeflodarea.h"
#include "dtextedit.h"
#include "leftareaoftextedit.h"
#include <QDebug>

CodeFlodArea::CodeFlodArea(leftareaoftextedit *leftAreaWidget)
{
    m_leftAreaWidget = leftAreaWidget;
}

CodeFlodArea::~CodeFlodArea()
{

}

void CodeFlodArea::paintEvent(QPaintEvent *e)
{
    qDebug() << "codeflodarea::paintEvent ";
    m_leftAreaWidget->codeFlodAreaPaintEvent(e);
}

