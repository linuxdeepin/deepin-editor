// SPDX-FileCopyrightText: 2019-2026 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include "ut_colorselectwidget.h"

#include <QSignalSpy>

// 测试函数ColorLabel::setColorSelected
TEST_F(test_colorlabel, checkSetColorSelected)
{


    auto colorLabel = new ColorLabel(QColor("#FF0000"));
    colorLabel->setColorSelected(true);
    colorLabel->setColorSelected(false);

    EXPECT_NE(colorLabel->m_bSelected,true);
    EXPECT_NE(colorLabel,nullptr);
    colorLabel->deleteLater();

}

// 测试函数 ColorLabel::isSelected
TEST_F(test_colorlabel, checkIsSelected)
{
    auto colorLabel = new ColorLabel(QColor("#FF0000"));
    colorLabel->setColorSelected(true);
    bool result = colorLabel->isSelected();
    EXPECT_TRUE(result);



    colorLabel->setColorSelected(false);
    result = colorLabel->isSelected();
    EXPECT_FALSE(result);
    EXPECT_NE(colorLabel,nullptr);
    colorLabel->deleteLater();

}

// 测试函数 ColorLabel::getColor
TEST_F(test_colorlabel, checkGetColor)
{

    auto colorLabel = new ColorLabel(QColor("#FF0000"));
    QColor colorSet("red");
    QColor colorGet = colorLabel->getColor();
    EXPECT_EQ(colorSet, colorGet);

    EXPECT_NE(colorLabel,nullptr);
    colorLabel->deleteLater();

}

// 测试函数 ColorLabel::paintEvent
TEST_F(test_colorlabel, checkPaintEvent)
{
    auto colorLabel = new ColorLabel(QColor("#FF0000"));
    colorLabel->setColorSelected(true);
    QPaintEvent event(colorLabel->rect());
    colorLabel->paintEvent(&event);
    EXPECT_NE(colorLabel,nullptr);
    colorLabel->deleteLater();

    colorLabel = new ColorLabel(QColor("#FF0000"));
    colorLabel->setColorSelected(false);
    QPaintEvent event2(colorLabel->rect());
    colorLabel->paintEvent(&event2);
    EXPECT_NE(colorLabel,nullptr);

    colorLabel->m_bHover = true;
    colorLabel->paintEvent(&event2);
    colorLabel->m_bHover = false;
    colorLabel->m_bPressed = true;
    colorLabel->paintEvent(&event2);
    colorLabel->deleteLater();
}

// 测试函数 ColorLabel::mousePressEvent
TEST_F(test_colorlabel, checkMousePressEvent)
{

    // 场景1: LeftButtonPress
    auto colorLabel = new ColorLabel(QColor("#FF0000"));
    QSignalSpy spy(colorLabel, &ColorLabel::sigColorClicked);
    QMouseEvent pressEvent(QEvent::MouseButtonPress, QPointF(), QPointF(),
                           Qt::LeftButton, Qt::LeftButton, Qt::NoModifier);
    colorLabel->mousePressEvent(&pressEvent);
    EXPECT_TRUE(colorLabel->m_bPressed);
    EXPECT_EQ(spy.count(), 0);

    QMouseEvent releaseEvent(QEvent::MouseButtonRelease, QPointF(), QPointF(),
                             Qt::LeftButton, Qt::NoButton, Qt::NoModifier);
    colorLabel->mouseReleaseEvent(&releaseEvent);
    EXPECT_FALSE(colorLabel->m_bPressed);
    EXPECT_TRUE(colorLabel->isSelected());
    EXPECT_EQ(spy.count(), 1);

    EXPECT_NE(colorLabel,nullptr);
    colorLabel->deleteLater();



    // 场景2: 非LeftButtonPress
    colorLabel = new ColorLabel(QColor("#FF0000"));
    QSignalSpy spy2(colorLabel, &ColorLabel::sigColorClicked);
    QMouseEvent rightPressEvent(QEvent::MouseButtonPress, QPointF(), QPointF(),
                                Qt::RightButton, Qt::RightButton, Qt::NoModifier);
    colorLabel->mousePressEvent(&rightPressEvent);
    EXPECT_FALSE(colorLabel->m_bPressed);
    EXPECT_EQ(spy2.count(), 0);

    EXPECT_NE(colorLabel,nullptr);
    colorLabel->deleteLater();

}

// 测试函数 ColorLabel::enterEvent/leaveEvent
TEST_F(test_colorlabel, checkHoverState)
{
    auto colorLabel = new ColorLabel(QColor("#FF0000"));

#if QT_VERSION < QT_VERSION_CHECK(6, 0, 0)
    QEvent enterEvent(QEvent::Enter);
#else
    QEnterEvent enterEvent(QPointF{}, QPointF{}, QPointF{});
#endif
    colorLabel->enterEvent(&enterEvent);
    EXPECT_TRUE(colorLabel->m_bHover);

    colorLabel->m_bPressed = true;
    QEvent leaveEvent(QEvent::Leave);
    colorLabel->leaveEvent(&leaveEvent);
    EXPECT_FALSE(colorLabel->m_bHover);
    EXPECT_FALSE(colorLabel->m_bPressed);

    colorLabel->deleteLater();
}

// 测试函数 ColorSelectWdg::initWidget
TEST_F(test_colorselectwidget, checkInitWidget)
{

    // 场景1: 字符串非空
    ColorSelectWdg *colorSelctWidget = new ColorSelectWdg("this is a test");
    colorSelctWidget->initWidget();
    EXPECT_NE(colorSelctWidget,nullptr);
    EXPECT_NE(colorSelctWidget->m_pMainLayout,nullptr);
    colorSelctWidget->deleteLater();



    // 场景2: 字符串为空
    colorSelctWidget = new ColorSelectWdg(QString());
    colorSelctWidget->initWidget();
    EXPECT_NE(colorSelctWidget->m_pHLayout2,nullptr);
    EXPECT_NE(colorSelctWidget,nullptr);
    colorSelctWidget->deleteLater();

}

// 测试函数  ColorSelectWdg::setTheme
TEST_F(test_colorselectwidget, checkSetTheme)
{

    ColorSelectWdg *colorSelctWidget = new ColorSelectWdg("this is a test");
    colorSelctWidget->setTheme("light");
    EXPECT_NE(colorSelctWidget,nullptr);
    colorSelctWidget->deleteLater();



    colorSelctWidget = new ColorSelectWdg("this is a test");
    colorSelctWidget->setTheme("dark");
    EXPECT_NE(colorSelctWidget,nullptr);
    colorSelctWidget->deleteLater();

}

// 测试函数  ColorSelectWdg::getDefaultColor
TEST_F(test_colorselectwidget, checkGetDefaultColor)
{

    ColorSelectWdg *colorSelctWidget = new ColorSelectWdg("this is a test");
    QColor defaultColor = colorSelctWidget->getDefaultColor();
    EXPECT_TRUE(defaultColor.isValid());
    EXPECT_NE(colorSelctWidget,nullptr);
    colorSelctWidget->deleteLater();

}

// 测试函数  ColorSelectWdg::eventFilter
TEST_F(test_colorselectwidget, checkEventFilter)
{

    // 场景2: 过滤m_pLabel的LeftButton事件
    ColorSelectWdg *colorSelctWidget = new ColorSelectWdg("this is a test");
    QMouseEvent *event = new QMouseEvent(QEvent::MouseButtonPress, QPointF(), QPointF(),
                                         Qt::LeftButton, Qt::LeftButton, Qt::NoModifier);
    bool result = colorSelctWidget->eventFilter(colorSelctWidget->m_pLabel, event);
    EXPECT_TRUE(result);
    EXPECT_NE(colorSelctWidget,nullptr);
    colorSelctWidget->deleteLater();
    delete event;event=nullptr;



    // 场景2: 过滤非m_pLabel的LeftButton事件
    colorSelctWidget = new ColorSelectWdg("this is a test");
    event = new QMouseEvent(QEvent::MouseButtonPress, QPointF(), QPointF(),
                            Qt::LeftButton, Qt::LeftButton, Qt::NoModifier);
    result = colorSelctWidget->eventFilter(colorSelctWidget->m_pButton, event);
    EXPECT_FALSE(result);
    EXPECT_NE(colorSelctWidget,nullptr);
    colorSelctWidget->deleteLater();
    delete event;event=nullptr;



    // 场景3: 过滤m_pLabel的非LeftButton事件
    colorSelctWidget = new ColorSelectWdg("this is a test");
    event = new QMouseEvent(QEvent::MouseButtonPress, QPointF(), QPointF(),
                            Qt::RightButton, Qt::RightButton, Qt::NoModifier);
    result = colorSelctWidget->eventFilter(colorSelctWidget->m_pLabel, event);
    EXPECT_FALSE(result);
    EXPECT_NE(colorSelctWidget,nullptr);
    colorSelctWidget->deleteLater();
    delete event;event=nullptr;

}

// 测试 initWidget 内 lambda #1: 点击默认颜色按钮 (DPushButton::clicked)
TEST_F(test_colorselectwidget, checkButtonClickedLambda)
{
    ColorSelectWdg *colorSelctWidget = new ColorSelectWdg("this is a test");
    ASSERT_NE(colorSelctWidget->m_pButton, nullptr);

    QSignalSpy spy(colorSelctWidget, &ColorSelectWdg::sigColorSelected);
    // 触发按钮 clicked，调用 connect 注册的 lambda
    colorSelctWidget->m_pButton->click();
    EXPECT_EQ(spy.count(), 1);

    colorSelctWidget->deleteLater();
}

// 测试 initWidget 内 lambda #2: 点击单个颜色标签 (ColorLabel::sigColorClicked)
TEST_F(test_colorselectwidget, checkColorLabelClickedLambda)
{
    ColorSelectWdg *colorSelctWidget = new ColorSelectWdg("this is a test");
    ASSERT_GE(colorSelctWidget->m_colorLabels.size(), 2);

    QSignalSpy spy(colorSelctWidget, &ColorSelectWdg::sigColorSelected);

    // 点击第二个颜色标签(非默认选中)，按下时显示反馈，释放时发出 sigColorClicked，
    // 进而调用 ColorSelectWdg 中 connect 注册的 lambda
    ColorLabel *label = colorSelctWidget->m_colorLabels.at(1);
    QMouseEvent pressEvent(QEvent::MouseButtonPress, QPointF(), QPointF(),
                           Qt::LeftButton, Qt::LeftButton, Qt::NoModifier);
    label->mousePressEvent(&pressEvent);
    EXPECT_EQ(spy.count(), 0);

    QMouseEvent releaseEvent(QEvent::MouseButtonRelease, QPointF(), QPointF(),
                             Qt::LeftButton, Qt::NoButton, Qt::NoModifier);
    label->mouseReleaseEvent(&releaseEvent);
    // 默认选中的第一个标签应被取消选中
    EXPECT_FALSE(colorSelctWidget->m_colorLabels.at(0)->isSelected());
    EXPECT_EQ(spy.count(), 1);

    colorSelctWidget->deleteLater();
}
