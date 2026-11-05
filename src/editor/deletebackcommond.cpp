// SPDX-FileCopyrightText: 2022 - 2026 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include "deletebackcommond.h"
#include "dtextedit.h"
#include <QTextBlock>

DeleteBackCommand::DeleteBackCommand(QTextCursor cursor, QPlainTextEdit *edit):
    m_cursor(cursor),
    m_edit(edit)
{
    m_delText = m_cursor.selectedText();
    m_delPos = std::min(m_cursor.position(), m_cursor.anchor());
    m_insertPos = m_delPos;
}

DeleteBackCommand::~DeleteBackCommand()
{

}

void DeleteBackCommand::undo()
{
    m_cursor.setPosition(m_insertPos);
    m_cursor.insertText(m_delText);

    QTextCursor cursor = m_edit->textCursor();
    cursor.setPosition(m_insertPos);
    cursor.setPosition(m_insertPos + m_delText.size(), QTextCursor::KeepAnchor);
    m_edit->setTextCursor(cursor);
}

void DeleteBackCommand::redo()
{
    m_cursor.setPosition(m_delPos);
    m_cursor.setPosition(m_delPos+m_delText.size(), QTextCursor::KeepAnchor);
    m_cursor.deleteChar();

    // 撤销恢复时光标移回要撤销的位置
    m_edit->setTextCursor(m_cursor);
}

DeleteBackAltCommand::DeleteBackAltCommand(QList<QTextEdit::ExtraSelection> &selections,QPlainTextEdit* edit):
    m_ColumnEditSelections(selections),
    m_edit(edit)
{
    int size = m_ColumnEditSelections.size();
    int sum=0;
    for(int i=0;i<size;i++){
        QString text;
        auto cursor = m_ColumnEditSelections[i].cursor;

        if(!cursor.hasSelection() && !cursor.atBlockEnd()){
            cursor.setPosition(cursor.position() + 1, QTextCursor::KeepAnchor);
        }

        text = cursor.selectedText();
        if(!text.isEmpty()){
            int pos = std::min(cursor.anchor(),cursor.position());
            DelNode node;
            node.m_cursor = cursor;
            node.m_insertPos = pos;
            node.m_delPos = pos - sum;
            node.m_delText = text;
            node.m_id_in_Column = i;
            m_deletions.push_back(node);

            sum += text.size();
        }
    }
}

DeleteBackAltCommand::~DeleteBackAltCommand()
{

}

void DeleteBackAltCommand::undo()
{

    int size = m_deletions.size();
    for(int i=0;i<size;i++){
        auto cursor = m_deletions[i].m_cursor;
        int pos = m_deletions[i].m_insertPos;
        QString text = m_deletions[i].m_delText;
        cursor.setPosition(pos,QTextCursor::MoveAnchor);
        cursor.insertText(text);

        // 使用 KeepAnchor 恢复选区，使撤销后列编辑选中高亮正确显示
        cursor.setPosition(pos,QTextCursor::MoveAnchor);
        cursor.setPosition(pos + text.size(),QTextCursor::KeepAnchor);
        int id = m_deletions[i].m_id_in_Column;
        m_ColumnEditSelections[id].cursor = cursor;
        m_edit->setTextCursor(cursor);
    }

    // 撤销后用命令自身的选区快照恢复列编辑状态并刷新渲染
    TextEdit *textEdit = qobject_cast<TextEdit *>(m_edit);
    if (textEdit) {
        textEdit->setAltModSelections(m_ColumnEditSelections);
        textEdit->renderAllSelections();
    }

}

void DeleteBackAltCommand::redo()
{
    int size = m_deletions.size();

    for(int i=0;i<size;i++){
        auto cursor = m_deletions[i].m_cursor;
        int pos = m_deletions[i].m_delPos;
        QString text = m_deletions[i].m_delText;
        cursor.setPosition(pos,QTextCursor::MoveAnchor);

        for(int j=0;j<text.size();j++){
            cursor.deleteChar();
        }

        int id = m_deletions[i].m_id_in_Column;
        m_ColumnEditSelections[id].cursor = cursor;
    }

    // 重做后同步刷新列编辑选区渲染
    TextEdit *textEdit = qobject_cast<TextEdit *>(m_edit);
    if (textEdit) {
        textEdit->setAltModSelections(m_ColumnEditSelections);
        textEdit->renderAllSelections();
    }

}
