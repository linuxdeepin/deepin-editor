#include "tabbar.h"
#include <QLabel>
#include <QDebug>
#include "utils.h"

Tabbar::Tabbar()
{
    layout = new QHBoxLayout(this);
    layout->setContentsMargins(0, 0, 0, 0);

    QPixmap iconPixmap = QPixmap(Utils::getQrcPath("logo_24.svg"));
    QLabel *iconLabel = new QLabel();
    iconLabel->setPixmap(iconPixmap);
    iconLabel->setFixedSize(24, 40);

    tabbar = new TabWidget();

    layout->addSpacing(10);
    layout->addWidget(iconLabel, 0, Qt::AlignTop);
    layout->addSpacing(10);
    layout->addWidget(tabbar, 0, Qt::AlignTop);
    layout->addSpacing(40);

    connect(tabbar, &TabWidget::tabBarDoubleClicked, this, &Tabbar::handleTabbarDoubleClick, Qt::QueuedConnection);
    connect(tabbar, &TabWidget::currentChanged, this, &Tabbar::handleCurrentIndexChanged, Qt::QueuedConnection);
    connect(tabbar, &TabWidget::tabMoved, this, &Tabbar::handleTabMoved, Qt::QueuedConnection);
    connect(tabbar, &TabWidget::tabCloseRequested, this, &Tabbar::handleTabClosed, Qt::QueuedConnection);
    connect(tabbar, &TabWidget::tabAddRequested, this, &Tabbar::tabAddRequested, Qt::QueuedConnection);
    connect(tabbar, &TabWidget::tabReleaseRequested, this, &Tabbar::handleTabReleaseRequested, Qt::QueuedConnection);
    connect(tabbar, &TabWidget::tabDroped, this, &Tabbar::handleTabDroped, Qt::QueuedConnection);
    connect(tabbar, &TabWidget::closeTab, this, &Tabbar::handleCloseTab, Qt::QueuedConnection);
    connect(tabbar, &TabWidget::closeOtherTabs, this, &Tabbar::handleCloseOtherTabs, Qt::QueuedConnection);
}

void Tabbar::addTab(QString filepath, QString tabName)
{
    int index = currentIndex();

    tabbar->tabFiles.insert(index + 1, filepath);
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
    switchToFile(tabbar->tabFiles.value(index));
}

int Tabbar::isTabExist(QString filepath)
{
    for (int i = 0; i < tabbar->tabFiles.size(); i++) {
        if (tabbar->tabFiles[i] == filepath) {
            return i;
        }
    }

    return -1;
}

void Tabbar::handleTabMoved(int fromIndex, int toIndex)
{
    tabbar->tabFiles.swap(fromIndex, toIndex);
}

void Tabbar::handleCloseTab(int index)
{
    handleTabClosed(index);
}

void Tabbar::handleCloseOtherTabs(int index)
{
    closeOtherTabsExceptFile(tabbar->tabFiles[index]);
}

void Tabbar::handleTabClosed(int closeIndex)
{
    QString filepath = tabbar->tabFiles[closeIndex];

    tabbar->tabFiles.takeAt(closeIndex);
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

void Tabbar::closeTabWithIndex(int index)
{
    handleTabClosed(index);
}

void Tabbar::closeOtherTabs()
{
    closeOtherTabsExceptFile(tabbar->tabFiles[tabbar->currentIndex()]);
}

void Tabbar::closeOtherTabsExceptFile(QString filepath)
{
    while (tabbar->tabFiles.size() > 1) {
        QString firstPath = tabbar->tabFiles[0];
        if (firstPath != filepath) {
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
    return tabbar->tabFiles.value(currentIndex());
}

QString Tabbar::getTabName(int index)
{
    return tabbar->tabText(index);
}

QString Tabbar::getTabPath(int index)
{
    return tabbar->tabFiles.value(index);
}

void Tabbar::updateTab(int index, QString filepath, QString tabName)
{
    tabbar->setTabText(index, tabName);
    tabbar->tabFiles[index] = filepath;
}

void Tabbar::handleTabReleaseRequested(int index)
{
    if (tabbar->count() > 1) {
        tabReleaseRequested(getTabName(index), getTabPath(index), index);
    } else {
        qDebug() << "Just one tab in current window, don't need create another new window.";
    }
}

void Tabbar::handleTabDroped(int index, Qt::DropAction, QObject *target)
{
    // Remove match tab if tab drop to tabbar of deepin-editor.
    auto *tabWidget = qobject_cast<TabWidget*>(target);
    if (tabWidget != nullptr) {
        handleTabClosed(index);
    }
}
