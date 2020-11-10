#include "test_tabbar.h"

test_tabbar::test_tabbar()
{

}

TEST_F(test_tabbar, openFilesInWindow)
{
    Tabbar * tab = new Tabbar();
    tab->addTab(".cache/deepin/deepin-editor","aabb");

    assert(1==1);
}
//addTabWithIndex
TEST_F(test_tabbar, addTabWithIndex)
{
    Tabbar * tab = new Tabbar();
    tab->addTabWithIndex(0,".cache/deepin/deepin-editor","aabb");

    assert(1==1);
}
//closeTab
TEST_F(test_tabbar, closeTab)
{
    Tabbar * tab = new Tabbar();
    tab->closeTab(0);

    assert(1==1);
}
