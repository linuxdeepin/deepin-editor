#ifndef DELETEBACKCOMMOND_H
#define DELETEBACKCOMMOND_H
#include <QUndoCommand>
#include <QTextCursor>
#include <QTextEdit>
#include <qplaintextedit.h>
//向后删除单一文字或选中文字的撤销重做
class DeleteBackCommond:public QUndoCommand
{
public:
    DeleteBackCommond(QTextCursor cursor,QPlainTextEdit* edit);
    virtual ~DeleteBackCommond();
    virtual void undo();
    virtual void redo();

private:
    QTextCursor m_cursor;
    QString m_delText;
    int m_delPos;
    int m_insertPos;

    QPlainTextEdit* m_edit;

};

//列模式下向后删除的撤销重做
class DeleteBackAltCommond:public QUndoCommand
{
public:
    DeleteBackAltCommond(QList<QTextEdit::ExtraSelection> &selections,QPlainTextEdit* edit);
    virtual ~DeleteBackAltCommond();
    virtual void undo();
    virtual void redo();

public:
    struct DelNode
    {
        QString m_delText;
        int m_delPos;
        int m_insertPos;
        int m_id_in_Column;
        QTextCursor m_cursor;
    };

private:
    QList<QTextEdit::ExtraSelection>& m_ColumnEditSelections;
    QList<DelNode> m_deletions;
    QPlainTextEdit* m_edit;
};

#endif // DELETEBACKCOMMOND_H
