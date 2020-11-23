#ifndef TEST_UNCOMMENTSELECTION_H
#define TEST_UNCOMMENTSELECTION_H

#include"../../src/startmanager.h"
#include"../../src/dtextedit.h"
#include"../../src/settings.h"
#include"../../src/window.h"
#include"../../src/editwrapper.h"
#include"../../src/tabbar.h"
#include "gtest/gtest.h"
#include"../../src/uncommentselection.h"
#include <QObject>

using namespace Comment;
class test_uncommentselection: public QObject, public::testing::Test
{
public:
    test_uncommentselection();
};

#endif // TEST_UNCOMMENTSELECTION_H
