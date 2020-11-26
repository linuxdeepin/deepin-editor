#ifndef TEST_UNCOMMENTSELECTION_H
#define TEST_UNCOMMENTSELECTION_H

#include"../../src/common/settings.h"
#include"../../src/controls/tabbar.h"
#include"../../src/editor/editwrapper.h"
#include"../../src/widgets/window.h"
#include"../../src/startmanager.h"
#include"../../src/editor/dtextedit.h"
#include "gtest/gtest.h"
#include"../../src/editor/uncommentselection.h"
#include <QObject>

using namespace Comment;
class test_uncommentselection: public QObject, public::testing::Test
{
public:
    test_uncommentselection();
};

#endif // TEST_UNCOMMENTSELECTION_H
