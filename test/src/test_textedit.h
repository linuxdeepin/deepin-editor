#ifndef TEST_TEXTEDIT_H
#define TEST_TEXTEDIT_H
#include"../../src/startmanager.h"
#include"../../src/dtextedit.h"
#include"../../src/settings.h"
#include"../../src/window.h"
#include"../../src/editwrapper.h"
#include"../../src/tabbar.h"
#include "gtest/gtest.h"
#include <QObject>
#define private public


class test_textedit: public QObject, public::testing::Test
{
public:
    test_textedit();
};

#endif // TEST_TEXTEDIT_H
