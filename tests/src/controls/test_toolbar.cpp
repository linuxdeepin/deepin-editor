#include "test_toolbar.h"
#include "../src/controls/toolbar.h"
test_toolbar::test_toolbar()
{

}

TEST_F(test_toolbar, ToolBar)
{
    ToolBar* tool = new ToolBar();

    delete tool;
    tool = nullptr;

}

TEST_F(test_toolbar, setTabbar)
{
    ToolBar* tool = new ToolBar();
    tool->setTabbar(nullptr);

    tool->deleteLater();

}
