#ifndef TEST_STARTMANAGER_H
#define TEST_STARTMANAGER_H
#include"../../src/startmanager.h"
#include"../../src/dtextedit.h"
#include"../../src/settings.h"
#include"../../src/window.h"
#include"../../src/editwrapper.h"
#include"../../src/tabbar.h"
#include "gtest/gtest.h"
#include <QObject>


class test_startmanager: public QObject, public::testing::Test
{
        Q_OBJECT
public:
    test_startmanager();
};

#endif // TEST_STARTMANAGER_H
