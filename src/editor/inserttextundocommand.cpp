// SPDX-FileCopyrightText: 2019 - 2022 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include "inserttextundocommand.h"

InsertTextUndoCommand::InsertTextUndoCommand(QTextCursor textcursor, QString text, QPlainTextEdit *edit, QUndoCommand *parent)
    : QUndoCommand(parent)
    , m_pEdit(edit)
    , m_textCursor(textcursor)
    , m_sInsertText(text)
{

}

InsertTextUndoCommand::InsertTextUndoCommand(QList<QTextEdit::ExtraSelection> &selections, QString text, QPlainTextEdit *edit, QUndoCommand *parent)
    : QUndoCommand(parent)
    , m_pEdit(edit)
    , m_sInsertText(text)
    , m_ColumnEditSelections(selections)
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

        // 进行撤销/恢复时将光标移动到撤销位置
        if (m_pEdit) {
            m_pEdit->setTextCursor(m_textCursor);
        }
    } else {
        int cnt = m_ColumnEditSelections.size();
        for (int i = 0; i < cnt; i++) {
            m_ColumnEditSelections[i].cursor.deleteChar();
        }

        if (m_pEdit && !m_ColumnEditSelections.isEmpty()) {
            m_pEdit->setTextCursor(m_ColumnEditSelections.last().cursor);
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
        #if 0
        else if(!m_selectText.isEmpty())
        {
           m_textCursor.movePosition(QTextCursor::PreviousCharacter, QTextCursor::KeepAnchor, m_selectText.size());
           m_textCursor.removeSelectedText();
        }
        #endif
        m_textCursor.insertText(m_sInsertText);
        m_textCursor.movePosition(QTextCursor::PreviousCharacter, QTextCursor::KeepAnchor, m_sInsertText.length());
        m_beginPostion = m_textCursor.selectionStart();
        m_endPostion = m_textCursor.selectionEnd();

        // 进行撤销/恢复时将光标移动到撤销位置
        if (m_pEdit) {
            QTextCursor curCursor = m_pEdit->textCursor();
            curCursor.setPosition(m_endPostion);
            m_pEdit->setTextCursor(curCursor);
        }
    } else {
        int cnt = m_ColumnEditSelections.size();
        for (int i = 0; i < cnt; i++) {
            m_ColumnEditSelections[i].cursor.insertText(m_sInsertText);
            m_ColumnEditSelections[i].cursor.movePosition(QTextCursor::Left, QTextCursor::KeepAnchor, m_sInsertText.length());
        }

        if (m_pEdit && !m_ColumnEditSelections.isEmpty()) {
            QTextCursor curCursor = m_pEdit->textCursor();
            curCursor.setPosition(m_ColumnEditSelections.last().cursor.selectionEnd());
            m_pEdit->setTextCursor(curCursor);
        }
    }
}
