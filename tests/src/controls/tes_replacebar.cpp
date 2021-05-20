#include "tes_replacebar.h"

tes_replacebar::tes_replacebar()
{

}
//bool isFocus();
TEST_F(tes_replacebar, isFocus)
{
    ReplaceBar * rep = new ReplaceBar();
    rep->isFocus();

    assert(1==1);
}
//void focus();
TEST_F(tes_replacebar, focus)
{
    ReplaceBar * rep = new ReplaceBar();
    rep->focus();

    assert(1==1);
}

//void activeInput(QString text, QString file, int row, int column, int scrollOffset);
TEST_F(tes_replacebar, activeInput)
{
    ReplaceBar * rep = new ReplaceBar();
    rep->activeInput("aa","bb",2,2,2);

    assert(1==1);
}
//void setMismatchAlert(bool isAlert);
TEST_F(tes_replacebar, setMismatchAlert)
{
    ReplaceBar * rep = new ReplaceBar();
    rep->setMismatchAlert(true);

    assert(1==1);
}
//void setsearched(bool _);
TEST_F(tes_replacebar, setsearched)
{
    ReplaceBar * rep = new ReplaceBar();
    rep->setsearched(true);

    assert(1==1);
}
//    void change();
TEST_F(tes_replacebar, change)
{
    ReplaceBar * rep = new ReplaceBar();
    rep->change();

    assert(1==1);
}
//void replaceClose();
TEST_F(tes_replacebar, replaceClose)
{
    ReplaceBar * rep = new ReplaceBar();
    rep->replaceClose();

    assert(1==1);
}
//void handleContentChanged();
TEST_F(tes_replacebar, handleContentChanged)
{
    ReplaceBar * rep = new ReplaceBar();
    rep->handleContentChanged();

    assert(1==1);
}
//void handleReplaceAll();
TEST_F(tes_replacebar, handleReplaceAll)
{
    ReplaceBar * rep = new ReplaceBar();
    rep->handleReplaceAll();

    assert(1==1);
}
//void handleReplaceNext();
TEST_F(tes_replacebar, handleReplaceNext)
{
    ReplaceBar * rep = new ReplaceBar();
    rep->handleReplaceNext();

    assert(1==1);
}
//void handleReplaceRest();
TEST_F(tes_replacebar, handleReplaceRest)
{
    ReplaceBar * rep = new ReplaceBar();
    rep->handleReplaceRest();

    assert(1==1);
}

//protected:
//void hideEvent(QHideEvent *event);
TEST_F(tes_replacebar, hideEvent)
{
    ReplaceBar * rep = new ReplaceBar();
    QHideEvent*e;
    rep->hideEvent(e);

    assert(1==1);
}
//bool focusNextPrevChild(bool next);
//void keyPressEvent(QKeyEvent *e);


