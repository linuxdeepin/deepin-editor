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

    connect(tabbar, &DTabBar::tabBarDoubleClicked, this, &Tabbar::handleTabbarDoubleClick, Qt::QueuedConnection);
    connect(tabbar, &DTabBar::currentChanged, this, &Tabbar::handleCurrentIndexChanged, Qt::QueuedConnection);
    connect(tabbar, &DTabBar::tabMoved, this, &Tabbar::handleTabMoved, Qt::QueuedConnection);
    connect(tabbar, &DTabBar::tabCloseRequested, this, &Tabbar::handleTabClosed, Qt::QueuedConnection);
    connect(tabbar, &DTabBar::tabAddRequested, this, &Tabbar::tabAddRequested, Qt::QueuedConnection);
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

void Tabbar::closeOtherTabs()
{
    QString currentFilepath = tabFiles[tabbar->currentIndex()];
    
    while (tabFiles.size() > 1) {
        QString firstPath = tabFiles[0];
        if (firstPath != currentFilepath) {
            handleTabClosed(0);
        } else {
            handleTabClosed(1);
        }
    }
}


QString Tabbar::getActiveTabName()
{
    return tabbar->tabText(currentIndex());
}

QString Tabbar::getActiveTabPath()
{
    return tabFiles.value(currentIndex());
}

void Tabbar::updateTab(int index, QString filepath, QString tabName)
{
    tabbar->setTabText(index, tabName);
    tabFiles[index] = filepath;
}
