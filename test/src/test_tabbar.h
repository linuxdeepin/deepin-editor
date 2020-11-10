#ifndef TEST_TABBAR_H
#define TEST_TABBAR_H
#include "gtest/gtest.h"
#include <QObject>
#include"../../src/tabbar.h"


class test_tabbar: public QObject, public::testing::Test
{
public:
    test_tabbar();
};

#endif // TEST_TABBAR_H
