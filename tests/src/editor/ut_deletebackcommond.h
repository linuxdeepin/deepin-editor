#ifndef TEST_DELETEBACKCOMMOND_H
#define TEST_DELETEBACKCOMMOND_H


#include "gtest/gtest.h"
#include <QObject>


class test_deletebackcommond: public QObject, public::testing::Test
{
        Q_OBJECT
public:
    test_deletebackcommond();
};

class test_deletebackaltcommond: public QObject, public::testing::Test
{
        Q_OBJECT
public:
    test_deletebackaltcommond();
};


#endif // TEST_DELETEBACKCOMMOND_H
