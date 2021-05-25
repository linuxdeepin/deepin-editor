#ifndef INSERTBLOCKBYTEXTCOMMOND_H
#define INSERTBLOCKBYTEXTCOMMOND_H

#include <QUndoCommand>
#include <QTextCursor>
#include <QTextEdit>
#include <qplaintextedit.h>

//分块插入文本-撤销重做
class InsertBlockByTextCommond:public QUndoCommand
{
public:
    InsertBlockByTextCommond(const QString& text,QPlainTextEdit* edit);
    virtual ~InsertBlockByTextCommond();

    virtual void redo();
    virtual void undo();

private:
    QString m_text;
    QPlainTextEdit* m_edit;
    QList<QUndoCommand* > m_commondList;

};

#endif // INSERTBLOCKBYTEXTCOMMOND_H
