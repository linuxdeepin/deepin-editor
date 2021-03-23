#include "test_codeflodarea.h"

test_codeflodarea::test_codeflodarea()
{

}

TEST_F(test_codeflodarea, BookMarkWidget)
{
    TextEdit * c = new TextEdit();
    LeftAreaTextEdit*b = new LeftAreaTextEdit(c);
    CodeFlodArea * a = new CodeFlodArea(b);
    QPaintEvent *e;
    a->paintEvent(e);
    a->deleteLater();
}
