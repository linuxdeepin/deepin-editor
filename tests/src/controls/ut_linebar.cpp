// SPDX-FileCopyrightText: 2019-2026 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include "ut_linebar.h"
#include "../../src/controls/linebar.h"
#include <QApplication>
#include <QFocusEvent>
#include <QEvent>
#include <DGuiApplicationHelper>
DGUI_USE_NAMESPACE
DGUI_USE_NAMESPACE
DGUI_USE_NAMESPACE

test_linebar::test_linebar()
{

}

TEST_F(test_linebar, LineBar)
{
    LineBar lineBar(nullptr);
    
}

//public slots:
//    void handleTextChangeTimer();
TEST_F(test_linebar, handleTextChangeTimer)
{
    LineBar *lineBar = new LineBar();
    lineBar->handleTextChangeTimer();


     EXPECT_NE(lineBar,nullptr);

    lineBar->deleteLater();
    
}

//    void handleTextChanged();
TEST_F(test_linebar, handleTextChanged)
{
    LineBar *lineBar = new LineBar();
    lineBar->m_autoSaveTimer->start();

    lineBar->handleTextChanged();

    lineBar->m_autoSaveTimer->stop();

    EXPECT_EQ(lineBar->m_autoSaveTimer->isActive(),false);
    EXPECT_NE(lineBar,nullptr);

   lineBar->deleteLater();
}

//    void sendText(QString t);
TEST_F(test_linebar, sendText)
{
    LineBar *lineBar = new LineBar();
    lineBar->sendText("aa");
    
    EXPECT_NE(lineBar,nullptr);

    lineBar->deleteLater();
}

//protected:
//    virtual void focusOutEvent(QFocusEvent *e);
TEST_F(test_linebar, focusOutEvent)
{
    LineBar *lineBar = new LineBar();
    QFocusEvent *e = new QFocusEvent(QEvent::FocusIn);
    lineBar->focusOutEvent(e);


    EXPECT_NE(lineBar,nullptr);

    lineBar->deleteLater();
    delete e;e=nullptr;
    
}

//    virtual void keyPressEvent(QKeyEvent *e);
TEST_F(test_linebar, keyPressEvent)
{
    LineBar *lineBar = new LineBar();
    Qt::KeyboardModifier modefiers[4] = {Qt::ControlModifier,Qt::AltModifier,Qt::MetaModifier,Qt::NoModifier};

    EXPECT_NE(lineBar,nullptr);

    QKeyEvent *e = new QKeyEvent(QEvent::KeyPress,1,modefiers[0],"\r");
    lineBar->keyPressEvent(e);
    delete e;e=nullptr;


    e = new QKeyEvent(QEvent::KeyPress,1,modefiers[1],"\r");
    lineBar->keyPressEvent(e);
    delete e;e=nullptr;

    e = new QKeyEvent(QEvent::KeyPress,1,modefiers[2],"\r");
    lineBar->keyPressEvent(e);
    delete e;e=nullptr;

    e = new QKeyEvent(QEvent::KeyPress,1,modefiers[3],"\r");
    lineBar->keyPressEvent(e);
    delete e;e=nullptr;


    lineBar->deleteLater();
}

// Constructor lambda connected to DGuiApplicationHelper::sizeModeChanged
TEST_F(test_linebar, ConstructorSizeModeLambda)
{
    LineBar *lineBar = new LineBar();
    auto helper = DGuiApplicationHelper::instance();
    auto origMode = helper->sizeMode();

    EXPECT_NO_FATAL_FAILURE(helper->setSizeMode(origMode == DGuiApplicationHelper::NormalMode
                                                    ? DGuiApplicationHelper::CompactMode
                                                    : DGuiApplicationHelper::NormalMode));
    helper->setSizeMode(origMode); // restore

    EXPECT_NE(lineBar, nullptr);
    lineBar->deleteLater();
}

//setMatchCount 显示文本和可见性
TEST_F(test_linebar, setMatchCount_Display)
{
    LineBar *lineBar = new LineBar();
    lineBar->show();
    qApp->processEvents();

    lineBar->setMatchCount(5, 10);

    EXPECT_EQ(lineBar->m_matchCountLabel->text().toStdString(), "第5/10项");
    EXPECT_TRUE(lineBar->m_matchCountLabel->isVisible());

    lineBar->deleteLater();
}

//setMatchCount total==0 隐藏
TEST_F(test_linebar, setMatchCount_HideOnZero)
{
    LineBar *lineBar = new LineBar();
    lineBar->show();
    qApp->processEvents();
    lineBar->setMatchCount(5, 10);

    lineBar->setMatchCount(0, 0);

    EXPECT_FALSE(lineBar->m_matchCountLabel->isVisible());

    lineBar->deleteLater();
}

//setMatchCount 0/N 场景
TEST_F(test_linebar, setMatchCount_ZeroCurrent)
{
    LineBar *lineBar = new LineBar();
    lineBar->show();
    qApp->processEvents();

    lineBar->setMatchCount(0, 5);

    EXPECT_EQ(lineBar->m_matchCountLabel->text().toStdString(), "第0/5项");
    EXPECT_TRUE(lineBar->m_matchCountLabel->isVisible());

    lineBar->deleteLater();
}

//m_matchCountLabel 已无 stylesheet hack，验证不再包含 padding-left
TEST_F(test_linebar, m_matchCountLabel_NoStyleSheetHack)
{
    LineBar *lineBar = new LineBar();

    EXPECT_FALSE(lineBar->m_matchCountLabel->styleSheet().contains("padding-left"));

    lineBar->deleteLater();
}

//自绘清除按钮：有文本时显示，无文本时隐藏
TEST_F(test_linebar, clearButton_VisibilityWithText)
{
    LineBar *lineBar = new LineBar();
    lineBar->show();
    qApp->processEvents();

    // 初始无文本，清除按钮应隐藏
    lineBar->handleTextChanged("");
    EXPECT_FALSE(lineBar->m_clearButton->isVisible());

    // 有文本时清除按钮应显示
    lineBar->handleTextChanged("hello");
    EXPECT_TRUE(lineBar->m_clearButton->isVisible());

    // 清空文本后清除按钮应再次隐藏
    lineBar->handleTextChanged("");
    EXPECT_FALSE(lineBar->m_clearButton->isVisible());

    lineBar->deleteLater();
}

//setMatchCount label 文本与可见性（含 0/N 场景）
TEST_F(test_linebar, setMatchCount_LabelTextAndVisibility)
{
    LineBar *lineBar = new LineBar();
    lineBar->show();
    qApp->processEvents();

    // 正常计数
    lineBar->setMatchCount(3, 10);
    EXPECT_EQ(lineBar->m_matchCountLabel->text().toStdString(), "第3/10项");
    EXPECT_TRUE(lineBar->m_matchCountLabel->isVisible());

    // 0/N 场景
    lineBar->setMatchCount(0, 5);
    EXPECT_EQ(lineBar->m_matchCountLabel->text().toStdString(), "第0/5项");
    EXPECT_TRUE(lineBar->m_matchCountLabel->isVisible());

    // total==0 隐藏
    lineBar->setMatchCount(0, 0);
    EXPECT_FALSE(lineBar->m_matchCountLabel->isVisible());

    lineBar->deleteLater();
}
