// SPDX-FileCopyrightText: 2019 - 2023 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include "inserttextundocommand.h"

InsertTextUndoCommand::InsertTextUndoCommand(QTextCursor textcursor, QString text, QPlainTextEdit *edit, QUndoCommand *parent)
    : QUndoCommand(parent)
    , m_pEdit(edit)
    , m_textCursor(textcursor)
    , m_sInsertText(text)
{
    m_sInsertText.replace("\r\n", "\n");
}

InsertTextUndoCommand::InsertTextUndoCommand(QList<QTextEdit::ExtraSelection> &selections, QString text, QPlainTextEdit *edit, QUndoCommand *parent)
    : QUndoCommand(parent)
    , m_pEdit(edit)
    , m_sInsertText(text)
    , m_ColumnEditSelections(selections)
{
    m_sInsertText.replace("\r\n", "\n");
}

void InsertTextUndoCommand::undo()
{
    if (m_ColumnEditSelections.isEmpty()) {
        // 注意部分字符显示占位超过1
        m_textCursor.setPosition(m_endPostion);
        m_textCursor.setPosition(m_beginPostion, QTextCursor::KeepAnchor);
        m_textCursor.deleteChar();
        if (m_selectText != QString()) {
            m_textCursor.insertText(m_selectText);
            m_textCursor.setPosition(m_textCursor.position() - m_selectText.length(), QTextCursor::KeepAnchor);
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

        m_textCursor.insertText(m_sInsertText);
        m_textCursor.setPosition(m_textCursor.position() - m_sInsertText.length(), QTextCursor::KeepAnchor);
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
            m_ColumnEditSelections[i].cursor.setPosition(m_ColumnEditSelections[i].cursor.position() - m_sInsertText.length() + 1,
                                                         QTextCursor::KeepAnchor);
        }

        if (m_pEdit && !m_ColumnEditSelections.isEmpty()) {
            QTextCursor curCursor = m_pEdit->textCursor();
            curCursor.setPosition(m_ColumnEditSelections.last().cursor.selectionEnd());
            m_pEdit->setTextCursor(curCursor);
        }
    }
}


/**
 * @class MidButtonInsertTextUndoCommand
 * @brief 用于鼠标中键黏贴的插入撤销项，使用 QClipboard::Selection 类型插入选中的数据，
 *      需要注意中键插入不会覆盖被选中的文本。
 */
MidButtonInsertTextUndoCommand::MidButtonInsertTextUndoCommand(QTextCursor textcursor, QString text, QPlainTextEdit *edit, QUndoCommand *parent)
    : QUndoCommand(parent)
    , m_pEdit(edit)
    , m_textCursor(textcursor)
    , m_sInsertText(text)
{
    m_sInsertText.replace("\r\n", "\n");
    // 中键黏贴需要构造时计算一次光标位置
    m_beginPostion = m_textCursor.position();
    m_endPostion = m_textCursor.position() + m_sInsertText.length();
}

/**
 * @brief 撤销插入动作，移除已被插入的文本。
 */
void MidButtonInsertTextUndoCommand::undo()
{
    m_textCursor.setPosition(m_endPostion);
    m_textCursor.setPosition(m_beginPostion, QTextCursor::KeepAnchor);
    m_textCursor.deleteChar();

    // 进行撤销/恢复时将光标移动到撤销位置
    if (m_pEdit) {
        m_pEdit->setTextCursor(m_textCursor);
    }
}

/**
 * @brief 重新执行插入文本，用于恢复动作。
 *      当前中键插入使用默认流程，数据已插入至文本后再添加撤销项，
 *      首次 redo() 不执行，通过插入撤销栈后再添加子撤销项规避。
 */
void MidButtonInsertTextUndoCommand::redo()
{
    m_textCursor.insertText(m_sInsertText);
    m_textCursor.setPosition(m_textCursor.position() - m_sInsertText.length(), QTextCursor::KeepAnchor);
    m_beginPostion = m_textCursor.selectionStart();
    m_endPostion = m_textCursor.selectionEnd();

    // 进行撤销/恢复时将光标移动到撤销位置
    if (m_pEdit) {
        QTextCursor curCursor = m_pEdit->textCursor();
        curCursor.setPosition(m_endPostion);
        m_pEdit->setTextCursor(curCursor);
    }
}
