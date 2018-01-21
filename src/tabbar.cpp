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
    connect(tabbar, SIGNAL(tabMoved(int, int)), this, SLOT(handleTabMoved(int , int)), Qt::QueuedConnection);
    connect(tabbar, SIGNAL(tabCloseRequested(int)), this, SLOT(handleTabClosed(int)), Qt::QueuedConnection);
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

void Tabbar::handleTabMoved(int fromIndex, int toIndex)
{
    tabFiles.swap(fromIndex, toIndex);
}

void Tabbar::handleTabClosed(int closeIndex)
{
    QString filepath = tabFiles[closeIndex];
    
    tabFiles.takeAt(closeIndex);
    tabbar->removeTab(closeIndex);
    
    closeFile(filepath);
}

void Tabbar::activeTab(int index)
{
    tabbar->setCurrentIndex(index);
}

void Tabbar::selectNextTab()
{
    int currentIndex = tabbar->currentIndex();
    if (currentIndex >= tabbar->count() - 1) {
        tabbar->setCurrentIndex(0);
    } else {
        tabbar->setCurrentIndex(currentIndex + 1);
    }
}

void Tabbar::selectPrevTab()
{
    int currentIndex = tabbar->currentIndex();
    if (currentIndex <= 0) {
        tabbar->setCurrentIndex(tabbar->count() - 1);
    } else {
        tabbar->setCurrentIndex(currentIndex - 1);
    }
}
                      

void Tabbar::closeTab()
{
    handleTabClosed(tabbar->currentIndex());
}
