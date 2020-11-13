#ifndef TES_REPLACEBAR_H
#define TES_REPLACEBAR_H
#include "gtest/gtest.h"
#include <QObject>
#include"../../src/editwrapper.h"
#include"../../src/window.h"
#include"../../src/startmanager.h"
#include"../../src/dtextedit.h"
#include"../../src/replacebar.h"
#include<QEvent>

class tes_replacebar: public QObject, public::testing::Test
{
public:
    tes_replacebar();
};

#endif // TES_REPLACEBAR_H
