#ifndef TEST_TOAST_H
#define TEST_TOAST_H

#include"../../src/common/settings.h"
#include"../../src/controls/tabbar.h"
#include"../../src/editor/editwrapper.h"
#include"../../src/widgets/window.h"
#include"../../src/startmanager.h"
#include"../../src/editor/dtextedit.h"
#include"../../src/controls/toast.h"

#include "gtest/gtest.h"
#include <QObject>

class test_toast: public QObject, public::testing::Test
{
public:
    test_toast();
};

#endif // TEST_TOAST_H
