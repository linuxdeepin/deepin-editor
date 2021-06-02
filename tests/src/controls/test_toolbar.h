#ifndef TEST_TOOLBAR_H
#define TEST_TOOLBAR_H


#include "gtest/gtest.h"
#include <QObject>

class test_toolbar : public QObject, public::testing::Test
{
public:
    test_toolbar();
};
#endif // TEST_TOOLBAR_H
