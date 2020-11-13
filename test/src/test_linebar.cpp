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
#include "test_linebar.h"
#include "../../src/linebar.h"
#include <QFocusEvent>
#include <QEvent>

test_linebar::test_linebar()
{

}

TEST_F(test_linebar, LineBar)
{
    LineBar lineBar(nullptr);
    assert(1==1);
}

//public slots:
//    void handleTextChangeTimer();
TEST_F(test_linebar, handleTextChangeTimer)
{
    LineBar *lineBar = new LineBar();
    lineBar->handleTextChangeTimer();
    assert(1==1);
}

//    void handleTextChanged();
TEST_F(test_linebar, handleTextChanged)
{
    LineBar *lineBar = new LineBar();
    lineBar->handleTextChanged();
    assert(1==1);
}

//    void sendText(QString t);
TEST_F(test_linebar, sendText)
{
    LineBar *lineBar = new LineBar();
    lineBar->sendText("aa");
    assert(1==1);
}

//protected:
//    virtual void focusOutEvent(QFocusEvent *e);
TEST_F(test_linebar, focusOutEvent)
{
    LineBar *lineBar = new LineBar();
    QFocusEvent *e = new QFocusEvent(QEvent::FocusIn);
    lineBar->focusOutEvent(e);
    assert(1==1);
}

//    virtual void keyPressEvent(QKeyEvent *e);
TEST_F(test_linebar, keyPressEvent)
{
    LineBar *lineBar = new LineBar();
    QKeyEvent *e = new QKeyEvent(QEvent::KeyPress,1,Qt::NoModifier);
    lineBar->keyPressEvent(e);
    assert(1==1);
}
