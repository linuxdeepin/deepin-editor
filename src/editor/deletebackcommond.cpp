// SPDX-FileCopyrightText: 2022 - 2023 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include "deletebackcommond.h"

#include <QTextBlock>
#include <QDebug>
#include "dtextedit.h"

DeleteBackCommand::DeleteBackCommand(QTextCursor cursor, QPlainTextEdit *edit)
    : m_cursor(cursor)
    , m_edit(edit)
{
    m_delText = m_cursor.selectedText();
    m_delPos = std::min(m_cursor.position(), m_cursor.anchor());
    m_insertPos = m_delPos;
    
    qDebug() << "DeleteBackCommand created - delete pos:" << m_delPos
                << ", text length:" << m_delText.length();
}

DeleteBackCommand::~DeleteBackCommand()
{
    qDebug() << "DeleteBackCommand destroyed";
}

void DeleteBackCommand::undo()
{
    qDebug() << "DeleteBackCommand undo - inserting text at:" << m_insertPos
                << ", length:" << m_delText.length();
    m_cursor.setPosition(m_insertPos);
    m_cursor.insertText(m_delText);

    QTextCursor cursor = m_edit->textCursor();
    cursor.setPosition(m_insertPos);
    cursor.setPosition(m_insertPos + m_delText.size(), QTextCursor::KeepAnchor);
    m_edit->setTextCursor(cursor);
}

void DeleteBackCommand::redo()
{
    qDebug() << "DeleteBackCommand redo - deleting text at:" << m_delPos
                << ", length:" << m_delText.length();
    m_cursor.setPosition(m_delPos);
    m_cursor.setPosition(m_delPos + m_delText.size(), QTextCursor::KeepAnchor);
    m_cursor.deleteChar();

    // restore cursor position
    m_edit->setTextCursor(m_cursor);
}

/**
   @brief Delete column selection with 'Backsapce' or 'Delete'
        The \a backward is default delete character direction (if not selected).
        The 'Backspace' button is forward, and the 'Delete' button is backward.
 */
DeleteBackAltCommand::DeleteBackAltCommand(const QList<QTextEdit::ExtraSelection> &selections,
                                                 TextEdit *edit,
                                                 bool backward)
    : m_columnEditSelections(selections)
    , m_edit(edit)
{
    qDebug() << "DeleteBackAltCommand created with" << selections.size()
                << "selections, backward:" << backward;
    int sum = 0;
    for (int i = 0; i < m_columnEditSelections.size(); i++) {
        QString text;
        auto cursor = m_columnEditSelections[i].cursor;

        if (!cursor.hasSelection()) {
            qDebug() << "cursor.hasSelection() is false";
            // keyboard 'delete' button
            if (backward) {
                qDebug() << "backward is true";
                if (!cursor.atEnd()) {
                    qDebug() << "cursor.atEnd() is false";
                    cursor.setPosition(cursor.position() + 1, QTextCursor::KeepAnchor);
                }

            } else {
                qDebug() << "backward is false";
                // keyboard 'backspace' button
                if (!cursor.atStart()) {
                    qDebug() << "cursor.atStart() is false";
                    cursor.setPosition(cursor.position() - 1);
                    cursor.setPosition(cursor.position() + 1, QTextCursor::KeepAnchor);
                }
            }
        }

        text = cursor.selectedText();
        if (!text.isEmpty()) {
            qDebug() << "text is not empty";
            int pos = std::min(cursor.anchor(), cursor.position());
            DelNode node;
            node.cursor = cursor;
            node.insertPos = pos;
            node.delPos = pos - sum;
            node.delText = text;
            node.idInColumn = i;
            node.leftToRight = cursor.anchor() < cursor.position();
            m_deletions.push_back(node);

            sum += text.size();
        }
    }
    qDebug() << "DeleteBackAltCommand created end";
}

DeleteBackAltCommand::~DeleteBackAltCommand()
{
    qDebug() << "DeleteBackAltCommand destroyed";
}

void DeleteBackAltCommand::undo()
{
    qDebug() << "DeleteBackAltCommand undo - restoring" << m_deletions.size() << "deletions";
    for (const DelNode &node : m_deletions) {
        auto cursor = node.cursor;
        const int pos = node.insertPos;
        const QString text = node.delText;
        cursor.setPosition(pos, QTextCursor::MoveAnchor);
        cursor.insertText(text);

        if (0 <= node.idInColumn && node.idInColumn < m_columnEditSelections.size()) {
            qDebug() << "node.idInColumn is not empty";
            const int endPos = pos + text.size();
            if (node.leftToRight) {
                qDebug() << "node.leftToRight is true";
                cursor.setPosition(pos);
                cursor.setPosition(endPos, QTextCursor::KeepAnchor);
            } else {
                qDebug() << "node.leftToRight is false";
                cursor.setPosition(endPos);
                cursor.setPosition(pos, QTextCursor::KeepAnchor);
            }
            m_columnEditSelections[node.idInColumn].cursor = cursor;
        }
    }

    if (m_edit && !m_columnEditSelections.isEmpty()) {
        qDebug() << "m_edit is not null";
        m_edit->restoreColumnEditSelection(m_columnEditSelections);
        m_edit->setTextCursor(m_columnEditSelections.last().cursor);
    }
    qDebug() << "DeleteBackAltCommand undo end";
}

void DeleteBackAltCommand::redo()
{
    qDebug() << "DeleteBackAltCommand redo - executing" << m_deletions.size() << "deletions";
    for (int i = 0; i < m_deletions.size(); i++) {
        auto cursor = m_deletions[i].cursor;
        const int pos = m_deletions[i].delPos;
        const QString text = m_deletions[i].delText;
        cursor.setPosition(pos, QTextCursor::MoveAnchor);

        for (int j = 0; j < text.size(); j++) {
            cursor.deleteChar();
        }
    }

    if (m_edit && !m_columnEditSelections.isEmpty()) {
        qDebug() << "m_edit is not null";
        m_edit->restoreColumnEditSelection(m_columnEditSelections);
        m_edit->setTextCursor(m_columnEditSelections.last().cursor);
    }
    qDebug() << "DeleteBackAltCommand redo end";
}

int DeleteBackAltCommand::id() const
{
    qDebug() << "DeleteBackAltCommand id";
    return Utils::IdColumnEditDelete;
}
