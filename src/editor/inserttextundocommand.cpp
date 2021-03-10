#include "inserttextundocommand.h"
#include <QDebug>

InsertTextUndoCommand::InsertTextUndoCommand(QTextCursor textcursor, QString text):
    m_textCursor(textcursor),
    m_sInsertText(text)
{

}

InsertTextUndoCommand::InsertTextUndoCommand(QList<QTextEdit::ExtraSelection> &selections, QString text):
    m_sInsertText(text),
    m_ColumnEditSelections(selections)
{

}


void InsertTextUndoCommand::undo()
{
    if (m_ColumnEditSelections.isEmpty()) {
        m_textCursor.setPosition(m_endPostion);
        m_textCursor.movePosition(QTextCursor::PreviousCharacter, QTextCursor::KeepAnchor, m_endPostion - m_beginPostion);
        m_textCursor.deleteChar();
        if (m_selectText != QString()) {
            m_textCursor.insertText(m_selectText);
            m_textCursor.movePosition(QTextCursor::Left, QTextCursor::KeepAnchor, m_selectText.length());
        }
    } else {
        int cnt = m_ColumnEditSelections.size();
        for (int i = 0; i < cnt; i++) {
            m_ColumnEditSelections[i].cursor.deleteChar();
        }
    }

}


void InsertTextUndoCommand::redo()
{
    if (m_ColumnEditSelections.isEmpty()) {
        if (m_textCursor.hasSelection()) {
            m_selectText = m_textCursor.selectedText();
            m_textCursor.removeSelectedText();
        }
        m_textCursor.insertText(m_sInsertText);
        m_textCursor.movePosition(QTextCursor::PreviousCharacter, QTextCursor::KeepAnchor, m_sInsertText.length());
        m_beginPostion = m_textCursor.selectionStart();
        m_endPostion = m_textCursor.selectionEnd();
    } else {
        int cnt = m_ColumnEditSelections.size();
        for (int i = 0; i < cnt; i++) {
            m_ColumnEditSelections[i].cursor.insertText(m_sInsertText);
            m_ColumnEditSelections[i].cursor.movePosition(QTextCursor::Left, QTextCursor::KeepAnchor, m_sInsertText.length());
        }
    }
}
