#include "test_themeitemdelegate.h"
#include "../src/thememodule/themeitemdelegate.h"
#include <QPainter>

test_ThemeItemDelegate::test_ThemeItemDelegate()
{
}

void test_ThemeItemDelegate::SetUp()
{
    tid = new ThemeItemDelegate();
}

void test_ThemeItemDelegate::TearDown()
{
    delete tid;
}

TEST_F(test_ThemeItemDelegate, paint)
{
    QPainter *painter = new QPainter();
    const QModelIndex index;
    const QStyleOptionViewItem option;
    //    ThemeItemDelegate tid;
    tid->paint(painter, option, index);
}

TEST_F(test_ThemeItemDelegate, sizeHint)
{
    const QModelIndex index;
    const QStyleOptionViewItem option;
    tid->sizeHint(option, index);
}
