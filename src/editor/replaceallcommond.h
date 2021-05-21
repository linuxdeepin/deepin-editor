#ifndef REPLACEALLCOMMOND_H
#define REPLACEALLCOMMOND_H

#include <QUndoCommand>
#include <QTextCursor>
#include <QTextDocument>
#include <QPlainTextEdit>

//全部替换撤销-重做
class ReplaceAllCommond:public QUndoCommand
{
public:
    ReplaceAllCommond(QString replace,QString replaceWith,QList<QTextCursor>& cursors);
    virtual ~ReplaceAllCommond();
    virtual void redo();
    virtual void undo();

private:
    QList<QTextCursor> m_cursors;
    QString m_replace;
    QString m_replaceWith;
    QList<int> m_insertPos;
    QList<int> m_delPos;
};

#endif // REPLACEALLCOMMOND_H
