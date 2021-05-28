#ifndef INSERTBLOCKBYTEXTCOMMOND_H
#define INSERTBLOCKBYTEXTCOMMOND_H

#include <QUndoCommand>
#include <QTextCursor>
#include <QTextEdit>
#include <qplaintextedit.h>
class TextEdit;
class EditWrapper;

//分块插入文本-撤销重做
class InsertBlockByTextCommond:public QUndoCommand
{
public:
    InsertBlockByTextCommond(const QString& text,TextEdit* edit,EditWrapper* wrapper);
    virtual ~InsertBlockByTextCommond();

    virtual void redo();
    virtual void undo();

private:
    void treat(bool isStart = true);
    void insertByBlock();

private:
    QString m_text;
    TextEdit* m_edit;
    EditWrapper* m_wrapper;
    int m_insertPos;
    int m_delPos;
    QString m_selected;
};

#endif // INSERTBLOCKBYTEXTCOMMOND_H
