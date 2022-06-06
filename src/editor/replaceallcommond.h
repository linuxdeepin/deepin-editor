#ifndef REPLACEALLCOMMOND_H
#define REPLACEALLCOMMOND_H

#include <QUndoCommand>
#include <QTextCursor>
#include <QTextDocument>
#include <QPlainTextEdit>
#include "dtextedit.h"

//全部替换撤销-重做
class ReplaceAllCommond:public QUndoCommand
{
public:
    ReplaceAllCommond(QString& oldText,QString& newText,QTextCursor cursor);
    virtual ~ReplaceAllCommond();

    virtual void redo();
    virtual void undo();

private:
    QString m_oldText;
    QString m_newText;
    QTextCursor m_cursor;

};

#endif // REPLACEALLCOMMOND_H
