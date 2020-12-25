#ifndef TEST_THEMEITEMDELEGATE_H
#define TEST_THEMEITEMDELEGATE_H

#include "gtest/gtest.h"
#include <QTest>
#include <DStyledItemDelegate>

class ThemeItemDelegate;
class test_ThemeItemDelegate :public testing::Test
{
public:
    test_ThemeItemDelegate();
    virtual void SetUp() override;
    virtual void TearDown() override;
    ThemeItemDelegate *tid;
};

#endif // TEST_THEMEITEMDELEGATE_H
