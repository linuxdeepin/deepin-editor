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
    connect(tabbar, SIGNAL(currentChanged(int)), this, SLOT(handleCurrentIndexChanged(int)), Qt::QueuedConnection);
}

void Tabbar::addTab(QString filepath, QString tabName)
{
    int index = currentIndex();

    tabFiles.insert(index + 1, filepath);
    tabbar->insertTab(index + 1, tabName);

    tabbar->setCurrentIndex(index + 1);
}

int Tabbar::currentIndex()
{
    return tabbar->currentIndex();
}

void Tabbar::handleTabbarDoubleClick()
{
    this->doubleClicked();
}

void Tabbar::handleCurrentIndexChanged(int index)
{
    switchToFile(tabFiles.value(index));
}

int Tabbar::isTabExist(QString filepath)
{
    for (int i = 0; i < tabFiles.size(); i++) {
        if (tabFiles[i] == filepath) {
            return i;
        }
    }

    return -1;
}
