#include "test_codeflodarea.h"
#include "QPaintEvent"
test_codeflodarea::test_codeflodarea()
{

}

TEST_F(test_codeflodarea, BookMarkWidget)
{
    TextEdit * c = new TextEdit();
    LeftAreaTextEdit*b = new LeftAreaTextEdit(c);
    CodeFlodArea * a = new CodeFlodArea(b);

    delete a;a=nullptr;
}

TEST_F(test_codeflodarea, paintEvent)
{
#if 0
    Window* window = new Window;
    EditWrapper *wrapper = window->createEditor();
    TextEdit * edit = wrapper->textEditor();
    edit->insertTextEx(edit->textCursor(),"123456");
    auto l = edit->getLeftAreaWidget();
    CodeFlodArea area(l);
    QPaintEvent* e = new QPaintEvent(area.rect());
    area.paintEvent(e);
    area.m_pLeftAreaWidget->codeFlodAreaPaintEvent(e);
#endif
}
