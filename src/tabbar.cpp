#include "tabbar.h"
#include <QPainter>
#include <QDebug>

Tabbar::Tabbar(QWidget *parent) : QWidget(parent)
{
    installEventFilter(this);   // add event filter
    currentTabIndex = 0;

    setFixedHeight(39);

    tabNameSize = 11;
    tabNameLeftPadding = 10;
    tabNameRightPadding = 10;
    tabCloseButtonWidth = 12;
    tabCloseButtonRightPadding = 10;
    tabUnderlineHeight = 2;
    tabNameTopPadding = 6;

    QFont font;
    font.setPointSize(tabNameSize);
    fontMetrics = new QFontMetrics(font);
}

Tabbar::~Tabbar()
{

}

void Tabbar::addTab(QString tabName)
{
    tabNames.insert(currentTabIndex, tabName);

    tabNameWidths.insert(currentTabIndex, fontMetrics->width(tabName));
    
    repaint();
}

void Tabbar::paintEvent(QPaintEvent *)
{
    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing, true);

    QFont font = painter.font() ;
    font.setPointSize(tabNameSize);
    painter.setFont(font);
    int tabX = 0;
    for (int i = 0; i < tabNames.length(); i++) {
        QString tabName = tabNames[i];
        int tabNameWidth = tabNameWidths[i];
        int width = tabNameLeftPadding + tabNameWidth + tabNameRightPadding + tabCloseButtonWidth + tabCloseButtonRightPadding;

        painter.setOpacity(0.05);
        QPainterPath separatorPath;
        separatorPath.addRect(QRectF(tabX, 0, 1, rect().height()));
        painter.fillPath(separatorPath, QColor("#ffffff"));

        if (i == tabNames.length() - 1) {
            QPainterPath lastSeparatorPath;
            lastSeparatorPath.addRect(QRectF(tabX + width - 1, 0, 1, rect().height()));
            painter.fillPath(lastSeparatorPath, QColor("#ffffff"));
        }

        painter.setOpacity(1);
        if (i == currentTabIndex) {
            painter.setPen(QPen(QColor("#2CA7F8")));
        } else {
            painter.setPen(QPen(QColor("#ffffff")));
        }
        painter.drawText(QRect(tabX + tabNameLeftPadding, tabNameTopPadding, width - tabNameLeftPadding, rect().height() - tabNameTopPadding), Qt::AlignLeft | Qt::AlignTop, tabName);
        
        if (i == currentTabIndex) {
            QPainterPath tabUnderlinePath;
            tabUnderlinePath.addRect(QRectF(tabX, rect().height() - tabUnderlineHeight, width, tabUnderlineHeight));
            painter.fillPath(tabUnderlinePath, QColor("#2CA7F8"));
        }

        tabX += width;
    }
}
