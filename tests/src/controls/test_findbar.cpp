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
    
}

//bool isFocus();
TEST_F(test_findbar, isFocus)
{
    FindBar *findBar = new FindBar();
    findBar->isFocus();
    
}

//void focus();
TEST_F(test_findbar, focus)
{
    FindBar *findBar = new FindBar();
    findBar->focus();
    
}

//void activeInput(QString text, QString file, int row, int column, int scrollOffset);
TEST_F(test_findbar, activeInput)
{
    FindBar *findBar = new FindBar();
    findBar->activeInput("aa","aa",1,1,1);
    
}

//void setMismatchAlert(bool isAlert);
TEST_F(test_findbar, setMismatchAlert)
{
    FindBar *findBar = new FindBar();
    findBar->setMismatchAlert(true);
    
}

//void receiveText(QString t);
TEST_F(test_findbar, receiveText)
{
    FindBar *findBar = new FindBar();
    findBar->receiveText("aa");
    
}

//void setSearched(bool _);
TEST_F(test_findbar, setSearched)
{
    FindBar *findBar = new FindBar();
    findBar->setSearched(true);
    
}

//void findPreClicked();
TEST_F(test_findbar, findPreClicked)
{
    FindBar *findBar = new FindBar();
    findBar->findPreClicked();

    findBar->findPreClicked();
    
}

//public slots:
//void findCancel();
TEST_F(test_findbar, findCancel)
{
    FindBar *findBar = new FindBar();
    findBar->findCancel();
    
}

//void handleContentChanged();
TEST_F(test_findbar, handleContentChanged)
{
    FindBar *findBar = new FindBar();
    findBar->handleContentChanged();
    findBar->handleFindPrev();
    findBar->handleFindNext();
    
}

//void slot_ifClearSearchWord();
//TEST_F(test_findbar, slot_ifClearSearchWord)
//{
//    FindBar *findBar = new FindBar();
//    findBar->slot_ifClearSearchWord();
//    
//}

//protected:
//void hideEvent(QHideEvent *event) override;
TEST_F(test_findbar, hideEvent)
{
    QHideEvent *event = new QHideEvent();
    FindBar *findBar = new FindBar();
    findBar->hideEvent(event);

    findBar->deleteLater();
    delete event;
    
}

//bool focusNextPrevChild(bool next) override;
TEST_F(test_findbar, focusNextPrevChild)
{
    FindBar *findBar = new FindBar();
    findBar->focusNextPrevChild(true);
    
}

//void keyPressEvent(QKeyEvent *e) override;
TEST_F(test_findbar, keyPressEvent)
{
    FindBar *findBar = new FindBar();

    QKeyEvent *e = new QKeyEvent(QEvent::KeyPress,Qt::Key_Tab,Qt::NoModifier);
    findBar->keyPressEvent(e);

    QKeyEvent *e1 = new QKeyEvent(QEvent::KeyPress,Qt::Key_Escape,Qt::NoModifier);
    findBar->keyPressEvent(e1);

    QKeyEvent *e2 = new QKeyEvent(QEvent::KeyPress,Qt::Key_Enter,Qt::NoModifier);
    findBar->keyPressEvent(e2);
    
}
