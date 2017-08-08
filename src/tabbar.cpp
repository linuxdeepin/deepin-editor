#include "tabbar.h"
#include "utils.h"
#include "dimagebutton.h"
#include "tab_close_button.h"
#include <QDebug>
#include <QMouseEvent>

DWIDGET_USE_NAMESPACE

Tabbar::Tabbar(QTabBar *parent) : QTabBar(parent)
{
    installEventFilter(this);  // add event filter
    setMouseTracking(true);   // make MouseMove can response

    setMovable(true);
    setExpanding(false);

    hoverTabIndex = -1;
    selectTabIndex = 0;
}

Tabbar::~Tabbar()
{
}

bool Tabbar::eventFilter(QObject *object, QEvent *event)
{
    if (event->type() == QEvent::MouseMove) {
        QMouseEvent *mouseEvent = static_cast<QMouseEvent*>(event);
        int currentTabIndex = tabAt(mouseEvent->pos());

        if (currentTabIndex != hoverTabIndex) {
            QWidget *prevWidget = tabButton(hoverTabIndex, QTabBar::RightSide);
            if (prevWidget != 0) {
                TabCloseButton *prevCloseButton = static_cast<TabCloseButton*>(prevWidget);
                prevCloseButton->setButtonVisible(false);
            }

            QWidget *currentWidget = tabButton(currentTabIndex, QTabBar::RightSide);
            if (currentWidget != 0) {
                TabCloseButton *currentCloseButton = static_cast<TabCloseButton*>(currentWidget);
                currentCloseButton->setButtonVisible(true);
            }
            
            hoverTabIndex = currentTabIndex;
        }
    }
    
    return false;
}

void Tabbar::newTab(QString tabName)
{
    addTab(tabName);

    TabCloseButton *closeButton = new TabCloseButton();
    
    setTabButton(selectTabIndex, QTabBar::RightSide, ((QWidget *) closeButton));
    
    selectTabIndex += 1;
}
