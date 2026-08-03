// SPDX-FileCopyrightText: 2022-2026 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include "ut_flashtween.h"

UT_FlashTween::UT_FlashTween()
{

}

//void startX(qreal t,qreal b,qreal c,qreal d, FunSlideInertial fSlideGesture);
TEST(UT_FlashTween_startX, UT_FlashTween_startX)
{
    FlashTween *a = new FlashTween();
    FunSlideInertial b;
    a->startX(1.1,1.1,1.1,1.1,b);
    int iRet = a->m_timerX->interval();
    ASSERT_TRUE(a->m_timerX->interval() == 15);

    a->deleteLater();
}

//void startY(qreal t,qreal b,qreal c,qreal d, FunSlideInertial fSlideGesture);
TEST(UT_FlashTween_startY, UT_FlashTween_startY)
{
    FlashTween *a = new FlashTween();
    FunSlideInertial b;
    a->startY(1.1,1.1,1.1,1.1,b);
    int iRet = a->m_timerY->interval();
    ASSERT_TRUE(a->m_timerY->interval() == 15);

    a->deleteLater();
}

//bool activeX(){return m_timerX->isActive();}
TEST(UT_FlashTween_activeX, UT_FlashTween_activeX)
{
    FlashTween *a = new FlashTween();
    FunSlideInertial b;
    a->m_timerX->start();
    bool bRet = a->activeX();
    ASSERT_TRUE(bRet == true);

    a->deleteLater();
}

//bool activeY(){return m_timerY->isActive();}
TEST(UT_FlashTween_activeY, UT_FlashTween_activeY)
{
    FlashTween *a = new FlashTween();
    FunSlideInertial b;
    a->m_timerY->start();
    bool bRet = a->activeY();
    ASSERT_TRUE(bRet == true);

    a->deleteLater();
}

TEST(UT_FlashTween_stopX, UT_FlashTween_stopX)
{
    FlashTween *a = new FlashTween();
    a->stopX();
    bool bRet = a->activeX();
    ASSERT_TRUE(bRet == false);

    a->deleteLater();
}

TEST(UT_FlashTween_stopY, UT_FlashTween_stopY)
{
    FlashTween *a = new FlashTween();
    a->stopY();
    bool bRet = a->activeY();
    ASSERT_TRUE(bRet == false);

    a->deleteLater();
}

TEST(UT_FlashTween___runY, UT_FlashTween___runY)
{
    FlashTween *a = new FlashTween();
    a->m_timerX = new QTimer;
    a->m_timerY = new QTimer;
    //a->__runY();

    EXPECT_NE(a->m_lastValueX,2.2);
    a->deleteLater();
    a->m_timerX->deleteLater();
    a->m_timerY->deleteLater();
}

TEST(UT_FlashTween___runX, UT_FlashTween___runX)
{
    FlashTween *a = new FlashTween();
    a->m_timerX = new QTimer;
    a->m_timerY = new QTimer;
    //a->__runX();

    EXPECT_NE(a->m_lastValueX,2.2);
    a->deleteLater();
    a->m_timerX->deleteLater();
    a->m_timerY->deleteLater();
}

// ---------------------------------------------------------------------------
// Appended tests that actually exercise the previously uncovered functions
// ---------------------------------------------------------------------------

// FlashTween::sinusoidalEaseOut(double,double,double,double) -- static private
TEST(UT_FlashTween_sinusoidalEaseOut, sinusoidalEaseOut)
{
    qreal result = FlashTween::sinusoidalEaseOut(0.0, 0.0, 100.0, 100.0);
    EXPECT_NEAR(result, 0.0, 0.001);

    qreal mid = FlashTween::sinusoidalEaseOut(50.0, 0.0, 100.0, 100.0);
    EXPECT_GT(mid, 0.0);

    qreal end = FlashTween::sinusoidalEaseOut(100.0, 0.0, 100.0, 100.0);
    EXPECT_NEAR(end, 100.0, 0.001);
}

// FlashTween::__runX() -- private slot, invoked directly via -fno-access-control
TEST(UT_FlashTween___runX_Invoke, __runX)
{
    FlashTween *a = new FlashTween();
    FunSlideInertial cb = [](qreal) {};
    // startX initializes m_fSlideGestureX / m_timerX / tween parameters
    a->startX(0.0, 0.0, 100.0, 100.0, cb);
    ASSERT_TRUE(a->activeX());

    a->__runX();   // process one frame
    // after one frame the current time advances and last value is updated
    EXPECT_GE(a->m_lastValueX, 0.0);

    delete a;
}

// FlashTween::__runY() -- private slot
TEST(UT_FlashTween___runY_Invoke, __runY)
{
    FlashTween *a = new FlashTween();
    FunSlideInertial cb = [](qreal) {};
    a->startY(0.0, 0.0, 100.0, 100.0, cb);
    ASSERT_TRUE(a->activeY());

    a->__runY();   // process one frame
    EXPECT_GE(a->m_lastValueY, 0.0);

    delete a;
}

// FlashTween::~FlashTween()
TEST(UT_FlashTween_Destructor, destructor)
{
    FlashTween *a = new FlashTween();
    ASSERT_TRUE(a->m_timerX != nullptr);
    ASSERT_TRUE(a->m_timerY != nullptr);

    delete a;   // destructor deletes both timers
    a = nullptr;
}
