/*
* Copyright (C) 2019 ~ 2021 Uniontech Software Technology Co.,Ltd.
*
* Author:     liangweidong <liangweidong@uniontech.com>
*
* Maintainer: liangweidong <liangweidong@uniontech.com>
*
* This program is free software: you can redistribute it and/or modify
* it under the terms of the GNU General Public License as published by
* the Free Software Foundation, either version 3 of the License, or
* any later version.
*
* This program is distributed in the hope that it will be useful,
* but WITHOUT ANY WARRANTY; without even the implied warranty of
* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
* GNU General Public License for more details.
*
* You should have received a copy of the GNU General Public License
* along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/

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
    if(m_ColumnEditSelections.isEmpty()){
        m_textCursor.setPosition(m_endPostion);
        m_textCursor.movePosition(QTextCursor::PreviousCharacter, QTextCursor::KeepAnchor, m_endPostion - m_beginPostion);
        m_textCursor.deleteChar();
        if (m_selectText != QString()) {
            m_textCursor.insertText(m_selectText);
            m_textCursor.movePosition(QTextCursor::Left, QTextCursor::KeepAnchor, m_selectText.length());
        }
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
        if (m_textCursor.hasSelection()) {
            m_selectText = m_textCursor.selectedText();
            m_textCursor.removeSelectedText();
        }
        m_textCursor.insertText(m_sInsertText);
        m_textCursor.movePosition(QTextCursor::PreviousCharacter, QTextCursor::KeepAnchor, m_sInsertText.length());
        m_beginPostion = m_textCursor.selectionStart();
        m_endPostion = m_textCursor.selectionEnd();
    }else {
        int cnt = m_ColumnEditSelections.size();
        for (int i = 0; i < cnt; i++) {
            //m_ColumnEditSelections[i].cursor.deleteChar();
            m_ColumnEditSelections[i].cursor.insertText(m_sInsertText);
            m_ColumnEditSelections[i].cursor.movePosition(QTextCursor::Left,QTextCursor::KeepAnchor,m_sInsertText.length());
        }
    }
}
