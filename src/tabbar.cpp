#include "tabbar.h"
#include <QLabel>
#include <QDebug>
#include "utils.h"

Tabbar::Tabbar(QWidget *parent) : QWidget(parent)
{
    layout = new QHBoxLayout(this);
    layout->setContentsMargins(0, 0, 0, 0);

    QPixmap iconPixmap = QPixmap(Utils::getQrcPath("logo_24.svg"));
    QLabel *iconLabel = new QLabel();
    iconLabel->setPixmap(iconPixmap);
    iconLabel->setFixedSize(24, 40);

    tabbar = new DTabBar();
    tabbar->setMovable(true);
    tabbar->setTabsClosable(true);
    tabbar->setVisibleAddButton(true);

    layout->addSpacing(10);
    layout->addWidget(iconLabel, 0, Qt::AlignTop);
    layout->addSpacing(10);
    layout->addWidget(tabbar, 0, Qt::AlignTop);
    layout->addSpacing(40);
    
    connect(tabbar, SIGNAL(tabBarDoubleClicked(int)), this, SLOT(handleTabbarDoubleClick()), Qt::QueuedConnection);
}

void Tabbar::addTab(QString tabName)
{
    tabbar->addTab(tabName);
}

int Tabbar::currentIndex()
{
    return tabbar->currentIndex();
}

void Tabbar::handleTabbarDoubleClick()
{
    this->doubleClicked();
}
