#ifndef TEST_STARTMANAGER_H
#define TEST_STARTMANAGER_H
#include"../../src/startmanager.h"
#include"../../src/editor/dtextedit.h"
#include"../../src/common/settings.h"
#include"../../src/widgets/window.h"
#include"../../src/editor/editwrapper.h"
#include"../../src/controls/tabbar.h"
#include "gtest/gtest.h"
#include <QObject>


class test_startmanager: public QObject, public::testing::Test
{
        Q_OBJECT
public:
    test_startmanager();
};

#endif // TEST_STARTMANAGER_H
