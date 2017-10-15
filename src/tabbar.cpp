#include "tabbar.h"
#include <QPaintEvent>
#include <QPainter>
#include <QColor>
#include <QDebug>
#include "utils.h"

Tabbar::Tabbar(QWidget *parent) : QWidget(parent)
{
    tabList = new QList<TabNode>();
    fontSize = 11;
    tabUnderlineHeight = 2;

    currentIndex = 0;
}

Tabbar::~Tabbar()
{
}

void Tabbar::paintEvent(QPaintEvent *)
{
    QPainter painter(this);

    int drawX = 0;
    for (int i = 0; i < tabList->size(); i++) {
        TabNode node = tabList->at(i);

        if (i == 0) {
            painter.setOpacity(0.05);
            QPainterPath separatorPath;
            separatorPath.addRect(QRectF(drawX, 0, 1, rect().height()));
            painter.fillPath(separatorPath, QColor("#ffffff"));
        }

        painter.setOpacity(0.05);
        QPainterPath separatorPath;
        separatorPath.addRect(QRectF(drawX + node.tabWidth, 0, 1, rect().height()));
        painter.fillPath(separatorPath, QColor("#ffffff"));

        Utils::setFontSize(painter, fontSize);
        painter.setOpacity(1);
        if (i == currentIndex) {
            painter.setPen(QColor("#2CA7F8"));
        } else {
            painter.setPen(QColor("#ffffff"));
        }
        painter.drawText(QRect(drawX + node.offsetLeft, rect().y(), node.nameWidth, rect().height()), Qt::AlignLeft | Qt::AlignVCenter, node.tabName);

        if (i == currentIndex) {
            QPainterPath tabUnderlinePath;
            tabUnderlinePath.addRect(QRectF(drawX, rect().height() - tabUnderlineHeight, node.tabWidth, tabUnderlineHeight));
            painter.fillPath(tabUnderlinePath, QColor("#2CA7F8"));
        }

        drawX += node.tabWidth;
    }
}

void Tabbar::addTab(QString tabId, QString tabName, int pos)
{
    TabNode node;
    node.tabId = tabId;
    node.tabName = tabName;
    node.offsetLeft = 10;
    node.offsetRight = 10;
    QFont font;
    font.setPointSize(fontSize);
    QFontMetrics fm(font);
    node.nameWidth = fm.width(tabName);
    node.tabWidth = node.nameWidth + node.offsetLeft + node.offsetRight;

    if (pos == -1) {
        if (tabList->size() > 0) {
            currentIndex = currentIndex + 1;
        }
        
        tabList->insert(currentIndex, node);
    } else {
        currentIndex = pos;
        
        tabList->insert(pos, node);
    }
    
    repaint();
}

void Tabbar::selectNextTab()
{
    if (currentIndex < tabList->size() - 1) {
        currentIndex += 1;
    } else {
        currentIndex = 0;
    }
    
    repaint();
}

void Tabbar::selectPrevTab()
{
    if (currentIndex > 0) {
        currentIndex -= 1;
    } else {
        currentIndex = tabList->size() - 1;
    }
    
    repaint();
}
