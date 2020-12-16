#ifndef DELETETEXTUNDOCOMMAND_H
#define DELETETEXTUNDOCOMMAND_H

#include <QObject>
#include <QUndoCommand>
#include <QTextCursor>
#include <QTextEdit>

class DeleteTextUndoCommand : public QUndoCommand
{
public:
    explicit DeleteTextUndoCommand(QTextCursor textcursor);
    explicit DeleteTextUndoCommand(QList<QTextEdit::ExtraSelection> &selections);
    virtual void undo();
    virtual void redo();


private:
    QTextCursor m_textCursor;
    QString m_sInsertText;
    QList<QString> m_selectTextList;
    QList<QTextEdit::ExtraSelection> m_ColumnEditSelections;
};

#endif // INSERTTEXTUNDOCOMMAND_H
