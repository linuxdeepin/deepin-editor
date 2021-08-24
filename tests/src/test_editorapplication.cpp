#include "test_editorapplication.h"
#include "../../src/editorapplication.h"
#include <QPushButton>
#include <QKeyEvent>
test_editorapplication::test_editorapplication()
{

}


TEST_F(test_editorapplication, EditorApplication)
{
    int argc = 1;
    char* argv[] = {"test"};
    //no deleted...
    EditorApplication *app = new EditorApplication(argc,argv);

}

TEST_F(test_editorapplication, notify)
{
    int argc = 1;
    char* argv[] = {"test"};
    //no deleted...
    EditorApplication *app = new EditorApplication(argc,argv);
    Qt::KeyboardModifier modefiers[4] = {Qt::ControlModifier,Qt::AltModifier,Qt::MetaModifier,Qt::NoModifier};
    QKeyEvent *e = new QKeyEvent(QEvent::KeyPress,Qt::Key_Return ,modefiers[0],"\r");

    QPushButton* btn = new QPushButton;
    btn->setObjectName("CustomRebackButton");
    app->notify(btn,e);

}

TEST_F(test_editorapplication, pressSpace)
{
    int argc = 1;
    char* argv[] = {"test"};
    //no deleted...
    EditorApplication *app = new EditorApplication(argc,argv);

    QPushButton* btn = new QPushButton;
    btn->setObjectName("CustomRebackButton");
    app->pressSpace(btn);

}
