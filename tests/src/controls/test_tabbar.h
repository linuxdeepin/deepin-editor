#ifndef TEST_TABBAR_H
#define TEST_TABBAR_H
#include "gtest/gtest.h"
#include <QObject>
#include"../../src/controls/tabbar.h"


class test_tabbar: public QObject, public::testing::Test
{
    Q_OBJECT

public:
    test_tabbar();
};

#endif // TEST_TABBAR_H
