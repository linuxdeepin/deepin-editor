#ifndef TES_REPLACEBAR_H
#define TES_REPLACEBAR_H
#include "gtest/gtest.h"
#include <QObject>
#include "../../src/editor/editwrapper.h"
#include "../../src/widgets/window.h"
#include "../../src/startmanager.h"
#include "../../src/editor/dtextedit.h"
#include "../../src/controls/replacebar.h"
#include<QEvent>

class tes_replacebar: public QObject, public::testing::Test
{
public:
    tes_replacebar();
};

#endif // TES_REPLACEBAR_H
