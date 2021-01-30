/*
* Copyright (C) 2019 ~ 2019 Deepin Technology Co., Ltd.
*
* Author:      Xiao Zhiguo <xiaozhiguo@uniontech.com>
* Maintainer:  Xiao Zhiguo <xiaozhiguo@uniontech.com>
*
* This program is free software: you can redistribute it and/or modify
* it under the terms of the GNU General Public License as published by
* the Free Software Foundation, either version 3 of the License, or
* any later version.
* This program is distributed in the hope that it will be useful,
* but WITHOUT ANY WARRANTY; without even the implied warranty of
* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
* GNU General Public License for more details.
* You should have received a copy of the GNU General Public License
* along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/

#ifndef TEST_COLORSELECTWIDGET_H
#define TEST_COLORSELECTWIDGET_H

#include "../../src/widgets/ColorSelectWdg.h"

#include <gtest/gtest.h>

class test_colorlabel : public testing::Test
{
protected:
    void SetUp()
    {
        colorLabel = new ColorLabel(QColor("#FF0000"));
    }
    void TearDown()
    {
        delete colorLabel;
    }

    ColorLabel *colorLabel = nullptr;
};

class test_colorselectwidget : public testing::Test
{
protected:
    void SetUp()
    {
        colorSelctWidget = new ColorSelectWdg("this is a test");
    }
    void TearDown()
    {
        delete colorSelctWidget;
    }

    ColorSelectWdg *colorSelctWidget = nullptr;
};

#endif // TEST_COLORSELECTWIDGET_H
