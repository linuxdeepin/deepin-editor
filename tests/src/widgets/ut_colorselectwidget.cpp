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

#include "ut_colorselectwidget.h"

#include <QSignalSpy>

// 测试函数ColorLabel::setColorSelected
TEST_F(test_colorlabel, checkSetColorSelected)
{
    do {
        colorLabel->setColorSelected(true);
    } while (false);

    do {
        colorLabel->setColorSelected(false);
    } while (false);
}

// 测试函数 ColorLabel::isSelected
TEST_F(test_colorlabel, checkIsSelected)
{
    do {
        colorLabel->setColorSelected(true);
        bool result = colorLabel->isSelected();
        EXPECT_TRUE(result);
    } while (false);

    do {
        colorLabel->setColorSelected(false);
        bool result = colorLabel->isSelected();
        EXPECT_FALSE(result);
    } while (false);
}

// 测试函数 ColorLabel::getColor
TEST_F(test_colorlabel, checkGetColor)
{
    do {
        QColor colorSet("red");
        ColorLabel *colorLabel = new ColorLabel(colorSet);
        QColor colorGet = colorLabel->getColor();
        EXPECT_EQ(colorSet, colorGet);
    } while (false);
}

// 测试函数 ColorLabel::paintEvent
TEST_F(test_colorlabel, checkPaintEvent)
{
    do {
        colorLabel->setColorSelected(true);
        QPaintEvent event(colorLabel->rect());
        colorLabel->paintEvent(&event);
    } while (false);

    do {
        colorLabel->setColorSelected(false);
        QPaintEvent event(colorLabel->rect());
        colorLabel->paintEvent(&event);
    } while (false);
}

// 测试函数 ColorLabel::mousePressEvent
TEST_F(test_colorlabel, checkMousePressEvent)
{
    do {
        // 场景1: LeftButtonPress
        QSignalSpy spy(colorLabel, &ColorLabel::sigColorClicked);
        QMouseEvent *event = new QMouseEvent(QEvent::MouseButtonPress, QPointF(),
                                             Qt::LeftButton, Qt::LeftButton, Qt::NoModifier);
        colorLabel->mousePressEvent(event);
        delete event;
        EXPECT_EQ(spy.count(), 1);
    } while (false);

    do {
        // 场景2: 非LeftButtonPress
        QSignalSpy spy(colorLabel, &ColorLabel::sigColorClicked);
        QMouseEvent *event = new QMouseEvent(QEvent::MouseButtonPress, QPointF(),
                                             Qt::RightButton, Qt::RightButton, Qt::NoModifier);
        colorLabel->mousePressEvent(event);
        delete event;
        EXPECT_EQ(spy.count(), 0);
    } while (false);
}

// 测试函数 ColorSelectWdg::initWidget
TEST_F(test_colorselectwidget, checkInitWidget)
{
    do {
        // 场景1: 字符串非空
        ColorSelectWdg *colorSelctWidget = new ColorSelectWdg("this is a test");
        colorSelctWidget->initWidget();
        delete colorSelctWidget;
    } while (false);

    do {
        // 场景2: 字符串为空
        ColorSelectWdg *colorSelctWidget = new ColorSelectWdg(QString());
        colorSelctWidget->initWidget();
        delete colorSelctWidget;
    } while (false);
}

// 测试函数  ColorSelectWdg::setTheme
TEST_F(test_colorselectwidget, checkSetTheme)
{
    do {
        colorSelctWidget->setTheme("light");
    } while (false);

    do {
        colorSelctWidget->setTheme("dark");
    } while (false);
}

// 测试函数  ColorSelectWdg::getDefaultColor
TEST_F(test_colorselectwidget, checkGetDefaultColor)
{
    do {
        QColor defaultColor = colorSelctWidget->getDefaultColor();
        EXPECT_TRUE(defaultColor.isValid());
    } while (false);
}

// 测试函数  ColorSelectWdg::eventFilter
TEST_F(test_colorselectwidget, checkEventFilter)
{
    do {
        // 场景2: 过滤m_pLabel的LeftButton事件
        QMouseEvent *event = new QMouseEvent(QEvent::MouseButtonPress, QPointF(),
                                             Qt::LeftButton, Qt::LeftButton, Qt::NoModifier);
        bool result = colorSelctWidget->eventFilter(colorSelctWidget->m_pLabel, event);
        EXPECT_TRUE(result);
        delete event;
    } while (false);

    do {
        // 场景2: 过滤非m_pLabel的LeftButton事件
        QMouseEvent *event = new QMouseEvent(QEvent::MouseButtonPress, QPointF(),
                                             Qt::LeftButton, Qt::LeftButton, Qt::NoModifier);
        bool result = colorSelctWidget->eventFilter(colorSelctWidget->m_pButton, event);
        EXPECT_FALSE(result);
        delete event;
    } while (false);

    do {
        // 场景3: 过滤m_pLabel的非LeftButton事件
        QMouseEvent *event = new QMouseEvent(QEvent::MouseButtonPress, QPointF(),
                                             Qt::RightButton, Qt::RightButton, Qt::NoModifier);
        bool result = colorSelctWidget->eventFilter(colorSelctWidget->m_pLabel, event);
        EXPECT_FALSE(result);
        delete event;
    } while (false);
}
