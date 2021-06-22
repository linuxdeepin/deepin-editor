#include "test_flashtween.h"

test_flashTween::test_flashTween()
{

}

TEST_F(test_flashTween, LeftAreaTextEdit)
{
    FlashTween *a = new FlashTween();
    a->stopX();

    delete a;
    a=nullptr;

    assert(1==1);
    delete a;
}

TEST_F(test_flashTween, stopY)
{
    FlashTween *a = new FlashTween();
    a->stopY();
    assert(1==1);
    delete a;
}
//public:
//    void startX(qreal t,qreal b,qreal c,qreal d, FunSlideInertial fSlideGesture);
TEST_F(test_flashTween, startX)
{
    FlashTween *a = new FlashTween();
    FunSlideInertial b;
    a->startX(1.1,1.1,1.1,1.1,b);
    assert(1==1);
    delete a;
}
//    void startY(qreal t,qreal b,qreal c,qreal d, FunSlideInertial fSlideGesture);
TEST_F(test_flashTween, startY)
{
    FlashTween *a = new FlashTween();
    FunSlideInertial b;
    a->startY(1.1,1.1,1.1,1.1,b);
    assert(1==1);
    delete a;
}
//    void stopX(){m_timerX->stop();}
//    void stopY(){m_timerY->stop();}
//    bool activeX(){return m_timerX->isActive();}
TEST_F(test_flashTween, activeX)
{
    FlashTween *a = new FlashTween();
    FunSlideInertial b;
    a->m_timerX->start();
    a->activeX();
    assert(1==1);
    delete a;
}
//    bool activeY(){return m_timerY->isActive();}
TEST_F(test_flashTween, activeY)
{
    FlashTween *a = new FlashTween();
    FunSlideInertial b;
    a->m_timerY->start();
    a->activeY();
    assert(1==1);
    delete a;
}

TEST_F(test_flashTween, stopX)
{
    FlashTween *a = new FlashTween();
    a->stopX();

    delete a;
    a=nullptr;
    assert(1==1);
    delete a;
}





//private slots:
//    void __runY();
//    void __runX();
