#ifndef TEST_WINDOW_H
#define TEST_WINDOW_H
#include "gtest/gtest.h"
#include"../../src/editor/editwrapper.h"
#include"../../src/widgets/window.h"
#include"../../src/startmanager.h"
#include"../../src/editor/dtextedit.h"
#include <QObject>
#include<QEvent>


class test_window: public QObject, public::testing::Test
{
public:
    test_window();
    EditWrapper * edit;
    Window * window;
};

#endif // TEST_WINDOW_H
