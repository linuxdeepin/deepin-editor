#ifndef REPLACEALLCOMMOND_H
#define REPLACEALLCOMMOND_H

#include <QUndoCommand>
#include <QTextCursor>
#include <QTextDocument>
#include <QPlainTextEdit>
class TextEdit;
class EditWrapper;

//全部替换撤销-重做
class ReplaceAllCommond:public QUndoCommand
{
public:
    ReplaceAllCommond(QString& oldText,QString& newText,QTextCursor cursor);
    ReplaceAllCommond(TextEdit *edit,EditWrapper* wrapper,QList<int> postions,const QString& oldText,const QString& newText);
    virtual ~ReplaceAllCommond();

    virtual void redo();
    virtual void undo();

public:
    struct Node
    {
        int redoPos;
        int undoPos;
    };

private:
    QString m_oldText;
    QString m_newText;
    QTextCursor m_cursor;

    TextEdit* m_edit;
    EditWrapper* m_wrapper;
    QList<Node> m_nodes;

};

#endif // REPLACEALLCOMMOND_H
