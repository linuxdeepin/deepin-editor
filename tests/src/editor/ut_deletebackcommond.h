#ifndef UT_Deletebackcommond_H
#define UT_Deletebackcommond_H


#include "gtest/gtest.h"
#include <QObject>


class UT_Deletebackcommond: public QObject, public::testing::Test
{
        Q_OBJECT
public:
    UT_Deletebackcommond();
};

class UT_Deletebackaltcommond: public QObject, public::testing::Test
{
        Q_OBJECT
public:
    UT_Deletebackaltcommond();
};


#endif // UT_Deletebackcommond_H
