#ifndef UT_DetectCode_H
#define UT_DetectCode_H
#include "gtest/gtest.h"
#include <QObject>


class UT_DetectCode: public QObject, public::testing::Test
{
     Q_OBJECT
public:
    UT_DetectCode();
};


#endif
