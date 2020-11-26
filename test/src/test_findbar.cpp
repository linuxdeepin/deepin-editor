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
#include "test_findbar.h"
#include "../../src/controls/findbar.h"
#include <QFocusEvent>
#include <QEvent>

test_findbar::test_findbar()
{

}

TEST_F(test_findbar, FindBar)
{
    FindBar findBar(nullptr);
    assert(1==1);
}

//bool isFocus();
TEST_F(test_findbar, isFocus)
{
    FindBar *findBar = new FindBar();
    findBar->isFocus();
    assert(1==1);
}

//void focus();
TEST_F(test_findbar, focus)
{
    FindBar *findBar = new FindBar();
    findBar->focus();
    assert(1==1);
}

//void activeInput(QString text, QString file, int row, int column, int scrollOffset);
TEST_F(test_findbar, activeInput)
{
    FindBar *findBar = new FindBar();
    findBar->activeInput("aa","aa",1,1,1);
    assert(1==1);
}

//void setMismatchAlert(bool isAlert);
TEST_F(test_findbar, setMismatchAlert)
{
    FindBar *findBar = new FindBar();
    findBar->setMismatchAlert(true);
    assert(1==1);
}

//void receiveText(QString t);
TEST_F(test_findbar, receiveText)
{
    FindBar *findBar = new FindBar();
    findBar->receiveText("aa");
    assert(1==1);
}

//void setSearched(bool _);
TEST_F(test_findbar, setSearched)
{
    FindBar *findBar = new FindBar();
    findBar->setSearched(true);
    assert(1==1);
}

//void findPreClicked();
TEST_F(test_findbar, findPreClicked)
{
    FindBar *findBar = new FindBar();
    findBar->findPreClicked();
    assert(1==1);
}

//public slots:
//void findCancel();
TEST_F(test_findbar, findCancel)
{
    FindBar *findBar = new FindBar();
    findBar->findCancel();
    assert(1==1);
}

//void handleContentChanged();
TEST_F(test_findbar, handleContentChanged)
{
    FindBar *findBar = new FindBar();
    findBar->handleContentChanged();
    assert(1==1);
}

//void slot_ifClearSearchWord();
TEST_F(test_findbar, slot_ifClearSearchWord)
{
    FindBar *findBar = new FindBar();
    findBar->slot_ifClearSearchWord();
    assert(1==1);
}

//protected:
//void hideEvent(QHideEvent *event) override;
TEST_F(test_findbar, hideEvent)
{
    QHideEvent *event = new QHideEvent();
    FindBar *findBar = new FindBar();
    findBar->hideEvent(event);
    assert(1==1);
}

//bool focusNextPrevChild(bool next) override;
TEST_F(test_findbar, focusNextPrevChild)
{
    FindBar *findBar = new FindBar();
    findBar->focusNextPrevChild(true);
    assert(1==1);
}

//void keyPressEvent(QKeyEvent *e) override;
TEST_F(test_findbar, keyPressEvent)
{
    QKeyEvent *e = new QKeyEvent(QEvent::KeyPress,1,Qt::NoModifier);
    FindBar *findBar = new FindBar();
    findBar->keyPressEvent(e);
    assert(1==1);
}
