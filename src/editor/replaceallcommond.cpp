// SPDX-FileCopyrightText: 2022-2026 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include "ddlog.h"
#include "replaceallcommond.h"
#include <QDebug>


ReplaceAllCommand::ReplaceAllCommand(QString &oldText, QString &newText, QTextCursor cursor, QUndoCommand *parent)
    : QUndoCommand(parent)
    , m_oldText(oldText)
    , m_newText(newText)
    , m_cursor(cursor)
{
    qCDebug(appLog) << "ReplaceAllCommand created - oldText length:" << oldText.length()
             << "newText length:" << newText.length() << "cursor pos:" << cursor.position();
}

ReplaceAllCommand::~ReplaceAllCommand()
{
    qCDebug(appLog) << "ReplaceAllCommand destroyed";
}

void ReplaceAllCommand::redo()
{
    qCInfo(appLog) << "ReplaceAllCommand redo - replacing all text with new text (length:" << m_newText.length() << ")";
    m_cursor.setPosition(0);
    m_cursor.movePosition(QTextCursor::End, QTextCursor::KeepAnchor);
    m_cursor.deleteChar();

    m_cursor.insertText(m_newText);
}

void ReplaceAllCommand::undo()
{
    qCInfo(appLog) << "ReplaceAllCommand undo - restoring original text (length:" << m_oldText.length() << ")";
    m_cursor.setPosition(0);
    m_cursor.movePosition(QTextCursor::End, QTextCursor::KeepAnchor);
    m_cursor.deleteChar();

    m_cursor.insertText(m_oldText);
}
