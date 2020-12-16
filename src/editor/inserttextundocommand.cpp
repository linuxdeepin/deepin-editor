#include "inserttextundocommand.h"
#include <QDebug>

InsertTextUndoCommand::InsertTextUndoCommand(QTextCursor textcursor, QString text):
    m_textCursor(textcursor),
    m_sInsertText(text)
{
    m_textCursor.clearSelection();
}

InsertTextUndoCommand::InsertTextUndoCommand(QList<QTextEdit::ExtraSelection> &selections, QString text):
    m_sInsertText(text),
    m_ColumnEditSelections(selections)
{

}


void InsertTextUndoCommand::undo()
{
    if(m_ColumnEditSelections.isEmpty()){
        m_textCursor.deleteChar();
        qDebug()<<"InsertTextUndoCommand[undo]:"<<m_sInsertText<<m_sInsertText.length();
    }else {
        int cnt = m_ColumnEditSelections.size();
        for (int i = 0; i < cnt; i++) {
            m_ColumnEditSelections[i].cursor.deleteChar();
        }
    }

}


void InsertTextUndoCommand::redo()
{
    if(m_ColumnEditSelections.isEmpty()){
        m_textCursor.insertText(m_sInsertText);
        m_textCursor.movePosition(QTextCursor::Left,QTextCursor::KeepAnchor,m_sInsertText.length());
        qDebug()<<"InsertTextUndoCommand[redo]:"<<m_sInsertText<<m_sInsertText.length();
    }else {
        int cnt = m_ColumnEditSelections.size();
        for (int i = 0; i < cnt; i++) {
            //m_ColumnEditSelections[i].cursor.deleteChar();
            m_ColumnEditSelections[i].cursor.insertText(m_sInsertText);
            m_ColumnEditSelections[i].cursor.movePosition(QTextCursor::Left,QTextCursor::KeepAnchor,m_sInsertText.length());
        }
    }
}
