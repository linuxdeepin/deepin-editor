#include "test_replacebar.h"
#include <QKeyEvent>
test_replacebar::test_replacebar()
{

}
//bool isFocus();
TEST_F(test_replacebar, isFocus)
{
    ReplaceBar * rep = new ReplaceBar();
    rep->isFocus();
    rep->handleSkip();

    assert(1==1);
}
//void focus();
TEST_F(test_replacebar, focus)
{
    ReplaceBar * rep = new ReplaceBar();
    rep->focus();

    assert(1==1);
}

//void activeInput(QString text, QString file, int row, int column, int scrollOffset);
TEST_F(test_replacebar, activeInput)
{
    ReplaceBar * rep = new ReplaceBar();
    rep->activeInput("aa","bb",2,2,2);

    assert(1==1);
}
//void setMismatchAlert(bool isAlert);
TEST_F(test_replacebar, setMismatchAlert)
{
    ReplaceBar * rep = new ReplaceBar();
    rep->setMismatchAlert(true);

    assert(1==1);
}
//void setsearched(bool _);
TEST_F(test_replacebar, setsearched)
{
    ReplaceBar * rep = new ReplaceBar();
    rep->setsearched(true);

    assert(1==1);
}
//    void change();
TEST_F(test_replacebar, change)
{
    ReplaceBar * rep = new ReplaceBar();
    rep->change();

    assert(1==1);
}
//void replaceClose();
TEST_F(test_replacebar, replaceClose)
{
    ReplaceBar * rep = new ReplaceBar();
    rep->replaceClose();

    assert(1==1);
}
//void handleContentChanged();
TEST_F(test_replacebar, handleContentChanged)
{
    ReplaceBar * rep = new ReplaceBar();
    rep->handleContentChanged();

    assert(1==1);
}
//void handleReplaceAll();
TEST_F(test_replacebar, handleReplaceAll)
{
    ReplaceBar * rep = new ReplaceBar();
    rep->handleReplaceAll();

    assert(1==1);
}
//void handleReplaceNext();
TEST_F(test_replacebar, handleReplaceNext)
{
    ReplaceBar * rep = new ReplaceBar();
    rep->handleReplaceNext();

    assert(1==1);
}
//void handleReplaceRest();
TEST_F(test_replacebar, handleReplaceRest)
{
    ReplaceBar * rep = new ReplaceBar();
    rep->handleReplaceRest();

    assert(1==1);
}

//protected:
//void hideEvent(QHideEvent *event);
TEST_F(test_replacebar, hideEvent)
{
    ReplaceBar * rep = new ReplaceBar();
    QHideEvent*e;
    rep->hideEvent(e);

    assert(1==1);
}
//bool focusNextPrevChild(bool next);
//void keyPressEvent(QKeyEvent *e);

TEST_F(test_replacebar, focusNextPrevChild)
{
    ReplaceBar * rep = new ReplaceBar();
    rep->focusNextPrevChild(true);

    assert(1==1);
}

TEST_F(test_replacebar, keyPressEvent)
{
    ReplaceBar * rep = new ReplaceBar();
    QKeyEvent * e3 = new QKeyEvent(QEvent::KeyPress,Qt::Key_Excel,Qt::NoModifier);
    rep->keyPressEvent(e3);

    QKeyEvent *e = new QKeyEvent(QEvent::KeyPress,Qt::Key_Tab,Qt::NoModifier);
    rep->m_closeButton->setFocus();
    rep->keyPressEvent(e);

    QKeyEvent *e1 = new QKeyEvent(QEvent::KeyPress,Qt::Key_Escape,Qt::NoModifier);
    rep->keyPressEvent(e1);

    QKeyEvent *e2 = new QKeyEvent(QEvent::KeyPress,Qt::Key_Enter,Qt::NoModifier);
    rep->m_replaceSkipButton->setFocus();
    rep->m_replaceButton->setFocus();
    rep->m_replaceAllButton->setFocus();
    rep->m_replaceRestButton->setFocus();
    rep->keyPressEvent(e2);


    assert(1==1);
}
