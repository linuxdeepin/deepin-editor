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

#include "test_bottombar.h"

#include <QPushButton>
#include <QPaintEvent>

// 测试函数 BottomBar::updatePosition
TEST_F(TestBottomBar, checkUpdatePosition)
{
    do {
        bottomBar->updatePosition(1, 1);
    } while (false);
}

// 测试函数 BottomBar::updateWordCount
TEST_F(TestBottomBar, checkUpdateWordCount)
{
    do {
        bottomBar->updateWordCount(1);
    } while (false);
}

// 测试函数 BottomBar::setEncodeName
TEST_F(TestBottomBar, checkSetEncodeName)
{
    do {
        bottomBar->setEncodeName("UTF-8");
    } while (false);
}

// 测试函数 BottomBar::setCursorStatus
TEST_F(TestBottomBar, checkSetCursorStatus)
{
    do {
        bottomBar->setCursorStatus("INSERT");
    } while (false);
}

// 测试函数 BottomBar::setPalette
TEST_F(TestBottomBar, checkSetPalette)
{
    do {
        QString backgroundColor = "#f8f8f8";
        QString textColor = "#1f1c1b";
        QPalette palette = bottomBar->palette();
        palette.setColor(QPalette::Background, backgroundColor);
        palette.setColor(QPalette::Text, textColor);
        bottomBar->setPalette(palette);
    } while (false);
}

// 测试函数 BottomBar::updateSize
TEST_F(TestBottomBar, checkUpdateSize)
{
    do {
        bottomBar->updateSize(32, false);
    } while (false);
}

// 测试函数 BottomBar::setChildrenFocus
TEST_F(TestBottomBar, checkSetChildrenFocus)
{
    do {
        // 场景1: ok = false, preOrderWidget = nullptr
        bottomBar->setChildrenFocus(false, nullptr);
    } while (false);

    do {
        // 场景2: ok = true, preOrderWidget = nullptr
        bottomBar->setChildrenFocus(true, nullptr);
    } while (false);

    do {
        // 场景3: ok = true, preOrderWidget = new QWidget()
        QPushButton *button = new QPushButton();
        bottomBar->setChildrenFocus(true, button);
        delete button;
    } while (false);

    do {
        // 场景4: ok = false, preOrderWidget = new QWidget()
        QPushButton *button = new QPushButton();
        bottomBar->setChildrenFocus(false, button);
        delete button;
    } while (false);
}

// 测试函数 BottomBar::getEncodeMenu
TEST_F(TestBottomBar, checkGetEncodeMenu)
{
    do {
        DDropdownMenu *menu = bottomBar->getEncodeMenu();
        EXPECT_NE(menu, nullptr);
    } while (false);
}

// 测试函数 BottomBar::getHighlightMenu
TEST_F(TestBottomBar, checkGetHighlightMenu)
{
    do {
        DDropdownMenu *menu = bottomBar->getHighlightMenu();
        EXPECT_NE(menu, nullptr);
    } while (false);
}

// 测试函数 BottomBar::paintEvent
TEST_F(TestBottomBar, checkPaintEvent)
{
    do {
        QPaintEvent event(bottomBar->rect());
        bottomBar->paintEvent(&event);
    } while (false);
}
