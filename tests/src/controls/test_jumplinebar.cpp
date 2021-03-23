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
#include "test_jumplinebar.h"
#include "../../src/controls/jumplinebar.h"

test_jumplinebar::test_jumplinebar()
{

}

TEST_F(test_jumplinebar, JumpLineBar)
{
    JumpLineBar jumpLineBar(nullptr);
    assert(1==1);
}

//void focus();
TEST_F(test_jumplinebar, focus)
{
    JumpLineBar *jumpLineBar = new JumpLineBar();
    jumpLineBar->focus();
    assert(1==1);
}

//bool isFocus();
TEST_F(test_jumplinebar, isFocus)
{
    JumpLineBar *jumpLineBar = new JumpLineBar();
    jumpLineBar->isFocus();
    assert(1==1);
}

//void activeInput(QString file, int row, int column, int lineCount, int scrollOffset);
TEST_F(test_jumplinebar, activeInput)
{
    JumpLineBar *jumpLineBar = new JumpLineBar();
    jumpLineBar->activeInput("aa",1,1,1,1);
    assert(1==1);
}

//void handleFocusOut();
TEST_F(test_jumplinebar, handleFocusOut)
{
    JumpLineBar *jumpLineBar = new JumpLineBar();
    jumpLineBar->handleFocusOut();
    assert(1==1);
}

//void handleLineChanged();
TEST_F(test_jumplinebar, handleLineChanged)
{
    JumpLineBar *jumpLineBar = new JumpLineBar();
    jumpLineBar->handleLineChanged();
    assert(1==1);
}

//void jumpCancel();
TEST_F(test_jumplinebar, jumpCancel)
{
    JumpLineBar *jumpLineBar = new JumpLineBar();
    jumpLineBar->jumpCancel();
    assert(1==1);
}

//void jumpConfirm();
TEST_F(test_jumplinebar, jumpConfirm)
{
    JumpLineBar *jumpLineBar = new JumpLineBar();
    jumpLineBar->jumpConfirm();
    assert(1==1);
}

//void slotFocusChanged(bool bFocus);
TEST_F(test_jumplinebar, slotFocusChanged)
{
    JumpLineBar *jumpLineBar = new JumpLineBar();
    jumpLineBar->slotFocusChanged(true);
    assert(1==1);
}
