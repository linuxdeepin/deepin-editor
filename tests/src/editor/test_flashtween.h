#ifndef TEST_FLASHTWEEN_H
#define TEST_FLASHTWEEN_H
#include "gtest/gtest.h"
#include"../../src/editor/editwrapper.h"
#include"../../src/widgets/window.h"
#include"../../src/startmanager.h"
#include"../../src/editor/dtextedit.h"
#include"../../src/editor/FlashTween.h"
#include"../stub.h"
#include <QObject>
#include<QWindow>
#include<QEvent>

class test_flashTween : public QObject, public::testing::Test
{
public:
    test_flashTween();
};

#endif // TEST_FLASHTWEEN_H
