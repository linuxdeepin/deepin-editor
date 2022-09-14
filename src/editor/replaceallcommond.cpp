// SPDX-FileCopyrightText: 2022 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include "replaceallcommond.h"



ReplaceAllCommond::ReplaceAllCommond(QString &oldText, QString &newText, QTextCursor cursor, QUndoCommand *parent)
    : QUndoCommand(parent)
    , m_oldText(oldText)
    , m_newText(newText)
    , m_cursor(cursor)
{

}

ReplaceAllCommond::~ReplaceAllCommond()
{

}

void ReplaceAllCommond::redo()
{
    m_cursor.setPosition(0);
    m_cursor.movePosition(QTextCursor::End, QTextCursor::KeepAnchor);
    m_cursor.deleteChar();

    m_cursor.insertText(m_newText);
}

void ReplaceAllCommond::undo()
{
    m_cursor.setPosition(0);
    m_cursor.movePosition(QTextCursor::End, QTextCursor::KeepAnchor);
    m_cursor.deleteChar();

    m_cursor.insertText(m_oldText);
}

