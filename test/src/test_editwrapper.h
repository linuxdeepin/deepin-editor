/*
* Copyright (C) 2019 ~ 2020 Deepin Technology Co., Ltd.
*
* Author:     liumaochuan <liumaochuan@uniontech.com>
* Maintainer: liumaochuan <liumaochuan@uniontech.com>
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
#ifndef TEST_EDITWRAPPER_H
#define TEST_EDITWRAPPER_H

#include"../../src/startmanager.h"
#include"../../src/dtextedit.h"
#include"../../src/settings.h"
#include"../../src/window.h"
#include"../../src/editwrapper.h"
#include"../../src/tabbar.h"
#include "gtest/gtest.h"
#include <QObject>
#define private public
#define protected public

class test_editwrapper: public QObject, public::testing::Test
{
public:
    test_editwrapper();
};

#endif // TEST_EDITWRAPPER_H
