// SPDX-FileCopyrightText: 2022 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include "replaceallcommond.h"
#include <QDebug>


ReplaceAllCommand::ReplaceAllCommand(QString &oldText, QString &newText, QTextCursor cursor, QUndoCommand *parent)
    : QUndoCommand(parent)
    , m_oldText(oldText)
    , m_newText(newText)
    , m_cursor(cursor)
{
    qDebug() << "ReplaceAllCommand created - oldText length:" << oldText.length()
             << "newText length:" << newText.length() << "cursor pos:" << cursor.position();
}

ReplaceAllCommand::~ReplaceAllCommand()
{
    qDebug() << "ReplaceAllCommand destroyed";
}

void ReplaceAllCommand::redo()
{
    qInfo() << "ReplaceAllCommand redo - replacing all text with new text (length:" << m_newText.length() << ")";
    m_cursor.setPosition(0);
    m_cursor.movePosition(QTextCursor::End, QTextCursor::KeepAnchor);
    m_cursor.deleteChar();

    m_cursor.insertText(m_newText);
}

void ReplaceAllCommand::undo()
{
    qInfo() << "ReplaceAllCommand undo - restoring original text (length:" << m_oldText.length() << ")";
    m_cursor.setPosition(0);
    m_cursor.movePosition(QTextCursor::End, QTextCursor::KeepAnchor);
    m_cursor.deleteChar();

    m_cursor.insertText(m_oldText);
}
