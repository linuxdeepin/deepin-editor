#include "tabbar.h"

Tabbar::Tabbar(QTabBar *parent) : QTabBar(parent)
{
    setMovable(true);
    setExpanding(false);
}

Tabbar::~Tabbar()
{
}
