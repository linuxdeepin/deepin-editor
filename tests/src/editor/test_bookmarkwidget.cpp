#include "test_bookmarkwidget.h"

test_bookmarkwidget::test_bookmarkwidget()
{

}

TEST_F(test_bookmarkwidget, paintEvent)
{
    TextEdit * c = new TextEdit();
    LeftAreaTextEdit*b = new LeftAreaTextEdit(c);
    BookMarkWidget * a = new BookMarkWidget(b);
    QPaintEvent *e;
    a->paintEvent(e);
    a->deleteLater();

    delete a;
    a=nullptr;
}
