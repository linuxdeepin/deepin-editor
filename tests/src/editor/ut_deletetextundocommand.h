#ifndef TEST_TEXTUNDOCOMMAND_H
#define TEST_TEXTUNDOCOMMAND_H
#include"../../src/editor/bookmarkwidget.h"
#include "gtest/gtest.h"
#include"../../src/editor/editwrapper.h"
#include"../../src/widgets/window.h"
#include"../../src/startmanager.h"
#include"../../src/editor/dtextedit.h"
#include"../../src/editor/FlashTween.h"
#include"../../src/editor/deletetextundocommand.h"
#include"../stub.h"
#include <QObject>
#include <QWindow>
#include <QEvent>


class test_deletetextundocommond: public QObject, public::testing::Test
{
public:
    test_deletetextundocommond();
};

#endif // TEST_TEXTUNDOCOMMAND_H
