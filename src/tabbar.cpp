#include "tabbar.h"
#include <QPainter>
#include <QDebug>

Tabbar::Tabbar(QWidget *parent) : QWidget(parent)
{
    installEventFilter(this);   // add event filter
    
    setFixedHeight(32);
}

Tabbar::~Tabbar()
{
    
}

void Tabbar::paintEvent(QPaintEvent *)
{
    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing, true);

    QPainterPath backgroundPath;
    backgroundPath.addRect(QRect(rect()));
    painter.fillPath(backgroundPath, QColor("#ff0000"));
}
