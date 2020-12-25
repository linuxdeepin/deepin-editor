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

#ifndef TEST_BOTTOMBAR_H
#define TEST_BOTTOMBAR_H

#include "../../src/widgets/bottombar.h"

#include <gtest/gtest.h>

class TestBottomBar : public testing::Test
{
protected:
    void SetUp()
    {
        bottomBar = new BottomBar();
    }
    void TearDown()
    {
        delete bottomBar;
    }

    BottomBar *bottomBar = nullptr;
};

#endif // TEST_BOTTOMBAR_H
