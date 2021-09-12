#include "ut_flashtween.h"

test_flashTween::test_flashTween()
{

}

//void startX(qreal t,qreal b,qreal c,qreal d, FunSlideInertial fSlideGesture);
TEST_F(test_flashTween, startX)
{
    FlashTween *a = new FlashTween();
    FunSlideInertial b;
    a->startX(1.1,1.1,1.1,1.1,b);
    int iRet = a->m_timerX->interval();
    ASSERT_TRUE(a->m_timerX->interval() == 15);
    
    a->deleteLater();
}

//void startY(qreal t,qreal b,qreal c,qreal d, FunSlideInertial fSlideGesture);
TEST_F(test_flashTween, startY)
{
    FlashTween *a = new FlashTween();
    FunSlideInertial b;
    a->startY(1.1,1.1,1.1,1.1,b);
    int iRet = a->m_timerY->interval();
    ASSERT_TRUE(a->m_timerY->interval() == 15);

    a->deleteLater();
}

//bool activeX(){return m_timerX->isActive();}
TEST_F(test_flashTween, activeX)
{
    FlashTween *a = new FlashTween();
    FunSlideInertial b;
    a->m_timerX->start();
    bool bRet = a->activeX();
    ASSERT_TRUE(bRet == true);

    a->deleteLater();
}

//bool activeY(){return m_timerY->isActive();}
TEST_F(test_flashTween, activeY)
{
    FlashTween *a = new FlashTween();
    FunSlideInertial b;
    a->m_timerY->start();
    bool bRet = a->activeY();
    ASSERT_TRUE(bRet == true);

    a->deleteLater();
}

TEST_F(test_flashTween, stopX)
{
    FlashTween *a = new FlashTween();
    a->stopX();
    bool bRet = a->activeX();
    ASSERT_TRUE(bRet == false);

    a->deleteLater();
}

TEST_F(test_flashTween, stopY)
{
    FlashTween *a = new FlashTween();
    a->stopY();
    bool bRet = a->activeY();
    ASSERT_TRUE(bRet == false);

    a->deleteLater();
}
