#ifndef TEST_WINDOW_H
#define TEST_WINDOW_H
#include "gtest/gtest.h"
#include <QObject>
#include"../../src/editwrapper.h"
#include"../../src/window.h"
#include"../../src/startmanager.h"
#include"../../src/dtextedit.h"
#include<QEvent>


class test_window: public QObject, public::testing::Test
{
public:
    test_window();
    EditWrapper * edit;
    Window * window;
};

#endif // TEST_WINDOW_H
