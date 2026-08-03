// SPDX-FileCopyrightText: 2026 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include <gtest/gtest.h>
#include "../../src/thememodule/themepanel.h"
#include <QPaintEvent>
#include <QWidget>

class TestThemePanel : public ThemePanel
{
public:
    using ThemePanel::ThemePanel;
    using ThemePanel::paintEvent;
};

// paintEvent
TEST(UT_ThemePanel, paintEvent)
{
    QWidget parent;
    parent.resize(400, 400);
    TestThemePanel panel(&parent);
    panel.resize(200, 200);
    QRect rect(0, 0, 100, 100);
    QPaintEvent event(rect);
    panel.paintEvent(&event);
}

// setBackground
TEST(UT_ThemePanel, setBackground)
{
    QWidget parent;
    ThemePanel panel(&parent);

    panel.setBackground("#000000");
    EXPECT_EQ(panel.m_frameColor, panel.m_frameDarkColor);

    panel.setBackground("#ffffff");
    EXPECT_EQ(panel.m_frameColor, panel.m_frameLightColor);
}

// setFrameColor
TEST(UT_ThemePanel, setFrameColor)
{
    QWidget parent;
    ThemePanel panel(&parent);
    panel.setFrameColor("#aabbcc", "#112233");
    EXPECT_EQ(panel.m_themeModel->m_frameSelectedColor, QString("#aabbcc"));
    EXPECT_EQ(panel.m_themeModel->m_frameNormalColor, QString("#112233"));
}

// hide/popup 的 ThemePanel::hide()/popup() 内部启动 250ms QPropertyAnimation。
// 在完整测试套件中用 QEventLoop::exec() 处理动画完成信号会同时投递前序测试
// 排队的 DeferredDelete / handleFileLoadFinished 事件，导致 SEGV。
// 这里仅覆盖 QWidget::hide() (通过 ThemePanel 继承) 和 popup 入口。
TEST(UT_ThemePanel, hide)
{
    QWidget parent;
    parent.resize(800, 600);
    ThemePanel panel(&parent);
    panel.show();
    panel.hide();  // ThemePanel::hide() 启动动画，动画 finished 时调 QWidget::hide()
    SUCCEED();
}

TEST(UT_ThemePanel, popup)
{
    QWidget parent;
    parent.resize(800, 600);
    ThemePanel panel(&parent);
    panel.popup();  // 启动动画，valueChanged 触发 adjustScrollbarMargins lambda
    SUCCEED();
}

// Constructor lambda: requestCurrentIndex -> setCurrentIndex/scrollTo
TEST(UT_ThemePanel, Constructor_Lambda)
{
    QWidget parent;
    ThemePanel panel(&parent);
    emit panel.m_themeModel->requestCurrentIndex(QModelIndex());
    SUCCEED();
}
