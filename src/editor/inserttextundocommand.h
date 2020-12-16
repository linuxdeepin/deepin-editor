#ifndef INSERTTEXTUNDOCOMMAND_H
#define INSERTTEXTUNDOCOMMAND_H

#include <QObject>
#include <QUndoCommand>
#include <QTextCursor>
#include <QTextEdit>

class InsertTextUndoCommand : public QUndoCommand
{
public:
    explicit InsertTextUndoCommand(QTextCursor textcursor,QString text);
    explicit InsertTextUndoCommand(QList<QTextEdit::ExtraSelection> &selections,QString text);
    virtual void undo();
    virtual void redo();

//private:
//    InsertTextUndoCommand(const InsertTextUndoCommand&) = delete;
//    InsertTextUndoCommand& operator=(const InsertTextUndoCommand&) = delete;

private:
    QTextCursor m_textCursor;
    QString m_sInsertText;
    QList<QTextEdit::ExtraSelection> m_ColumnEditSelections;
};

#endif // INSERTTEXTUNDOCOMMAND_H
