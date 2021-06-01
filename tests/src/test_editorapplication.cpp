#include "test_editorapplication.h"
#include "../../src/editorapplication.h"
test_editorapplication::test_editorapplication()
{

}


TEST_F(test_editorapplication, EditorApplication)
{
    int argc = 1;
    char* argv[] = {"test"};
    //no deleted...
    EditorApplication *app = new EditorApplication(argc,argv);
    assert(1==1);
}

TEST_F(test_editorapplication, notify)
{
    int argc = 1;
    char* argv[] = {"test"};
    //no deleted...
    EditorApplication *app = new EditorApplication(argc,argv);
    QEvent e(QEvent::KeyPress);
    QObject o;
    app->notify(&o,&e);
    assert(1==1);
}
