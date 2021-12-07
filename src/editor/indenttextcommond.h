#ifndef IndentTextCommond_H
#define IndentTextCommond_H

#include <QUndoCommand>
#include <QTextCursor>
#include <QTextDocument>
#include <QPlainTextEdit>
class TextEdit;
//indent text in front of multiple lines
class IndentTextCommond:public QUndoCommand
{
public:
    IndentTextCommond(TextEdit* edit,int startpos,int endpos,int startline,int endline);
    virtual ~IndentTextCommond();

    virtual void redo();
    virtual void undo();

private:

    TextEdit* m_edit=nullptr;
    //the start postion of selected text.
    int m_startpos=0;
    //the end postion of selected text.
    int m_endpos=0;
    //the start line of selected text.
    int m_startline=0;
    //the end line of selected text.
    int m_endline=0;
    bool m_hasselected=false;

};

#endif // IndentTextCommond_H
