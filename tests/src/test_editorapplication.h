#ifndef TEST_EDITORAPPLICATION_H
#define TEST_EDITORAPPLICATION_H

#include "gtest/gtest.h"
#include <QObject>


class test_editorapplication: public QObject, public::testing::Test
{
        Q_OBJECT
public:
    test_editorapplication();
};



#endif // TEST_EDITORAPPLICATION_H
