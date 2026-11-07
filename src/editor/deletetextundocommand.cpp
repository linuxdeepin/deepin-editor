// SPDX-FileCopyrightText: 2019 - 2026 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include "ddlog.h"
#include "deletetextundocommand.h"
#include "editwrapper.h"
#include <QDebug>
#include <QTextBlock>

#include "dtextedit.h"

DeleteTextUndoCommand::DeleteTextUndoCommand(QTextCursor textcursor, TextEdit *edit, QUndoCommand *parent)
    : QUndoCommand(parent)
    , m_edit(edit)
    , m_textCursor(textcursor)
    , m_beginPos(m_textCursor.selectionStart())
{
    qCDebug(appLog) << "DeleteTextUndoCommand created - beginPos:" << m_beginPos;

    if (m_textCursor.hasSelection()) {
        qCDebug(appLog) << "m_textCursor.hasSelection()";
        m_sInsertText = m_textCursor.selectedText();
    } else {
        qCDebug(appLog) << "m_textCursor.hasSelection() is false";
        // 无选区删除的是光标前一个字符，m_beginPos 记录被删字符位置（光标-1），
        // 否则 undo 会把字符重插到原位置之后一位
        m_beginPos = m_textCursor.position() - 1;
        int pos = m_textCursor.positionInBlock() - 1;
        if (pos >= 0) {
            qCDebug(appLog) << "pos >= 0";
            m_sInsertText = m_textCursor.block().text().at(pos);
        } else {
            //上一行lastQChar
            qCDebug(appLog) << "pos < 0";
            m_sInsertText = "\n";
        }
    }
    qCDebug(appLog) << "DeleteTextUndoCommand created end";
}

DeleteTextUndoCommand::DeleteTextUndoCommand(QList<QTextEdit::ExtraSelection> &selections,
                                             TextEdit *edit,
                                             QUndoCommand *parent)
    : QUndoCommand(parent)
    , m_edit(edit)
    , m_ColumnEditSelections(selections)
    , m_beginPos(m_textCursor.selectionStart())
{
    qCDebug(appLog) << "DeleteTextUndoCommand created - beginPos:" << m_beginPos;
    int cnt = m_ColumnEditSelections.size();
    for (int i = 0; i < cnt; i++) {
        QTextCursor textCursor = m_ColumnEditSelections[i].cursor;
        if (textCursor.hasSelection()) {
            qCDebug(appLog) << "textCursor.hasSelection()";
            m_selectTextList.append(textCursor.selectedText());
        } else {
            int pos = textCursor.positionInBlock() - 1;
            if (pos >= 0) {
                qCDebug(appLog) << "pos >= 0";
                m_selectTextList.append(textCursor.block().text().at(pos));
            } else {
                //上一行lastQChar
                qCDebug(appLog) << "pos < 0";
                m_selectTextList.append("\n");
            }
        }
    }
    qCDebug(appLog) << "DeleteTextUndoCommand created end";
}

void DeleteTextUndoCommand::undo()
{
    qCDebug(appLog) << "DeleteTextUndoCommand undo - beginPos:" << m_beginPos
                << ", text length:" << m_sInsertText.length();

    if (m_ColumnEditSelections.isEmpty()) {
        qCDebug(appLog) << "m_ColumnEditSelections.isEmpty()";
        // 插入前将光标恢复到删除前位置
        m_textCursor.setPosition(m_beginPos);
        m_textCursor.insertText(m_sInsertText);
        m_textCursor.setPosition(m_textCursor.position() - m_sInsertText.length(), QTextCursor::KeepAnchor);

        // 进行撤销/恢复时将光标移动到撤销位置
        if (m_edit) {
            m_edit->setTextCursor(m_textCursor);
        }
    } else {
        qCDebug(appLog) << "m_ColumnEditSelections.isEmpty() is false";
        int cnt = m_ColumnEditSelections.size();
        for (int i = 0; i < cnt; i++) {
            m_ColumnEditSelections[i].cursor.insertText(m_selectTextList[i]);
            m_ColumnEditSelections[i].cursor.setPosition(
                m_ColumnEditSelections[i].cursor.position() - m_selectTextList[i].length(), QTextCursor::KeepAnchor);
        }

        if (m_edit && !m_ColumnEditSelections.isEmpty()) {
            qCDebug(appLog) << "m_edit is not null";
            m_edit->restoreColumnEditSelection(m_ColumnEditSelections);
            m_edit->setTextCursor(m_ColumnEditSelections.last().cursor);
            m_edit->renderAllSelections();
        }
    }
    qCDebug(appLog) << "DeleteTextUndoCommand undo end";
}

void DeleteTextUndoCommand::redo()
{
    qCDebug(appLog) << "DeleteTextUndoCommand redo - beginPos:" << m_beginPos;

    if (m_ColumnEditSelections.isEmpty()) {
        qCDebug(appLog) << "m_ColumnEditSelections.isEmpty()";
        m_textCursor.deletePreviousChar();

        // 进行撤销/恢复时将光标移动到撤销位置
        if (m_edit) {
            m_edit->setTextCursor(m_textCursor);
        }
    } else {
        qCDebug(appLog) << "m_ColumnEditSelections.isEmpty() is false";
        int cnt = m_ColumnEditSelections.size();
        for (int i = 0; i < cnt; i++) {
            m_ColumnEditSelections[i].cursor.deletePreviousChar();
        }

        if (m_edit && !m_ColumnEditSelections.isEmpty()) {
            qCDebug(appLog) << "m_edit is not null";
            m_edit->restoreColumnEditSelection(m_ColumnEditSelections);
            m_edit->setTextCursor(m_ColumnEditSelections.last().cursor);
            m_edit->renderAllSelections();
        }
    }
    qCDebug(appLog) << "DeleteTextUndoCommand redo end";
}

int DeleteTextUndoCommand::id() const
{
    qCDebug(appLog) << "DeleteTextUndoCommand id";
    if (m_ColumnEditSelections.isEmpty()) {
        qCDebug(appLog) << "m_ColumnEditSelections.isEmpty()";
        return Utils::IdDelete;
    } else {
        qCDebug(appLog) << "m_ColumnEditSelections.isEmpty() is false";
        return Utils::IdColumnEditDelete;
    }
}

DeleteTextUndoCommand2::DeleteTextUndoCommand2(QTextCursor textcursor, QString text, QPlainTextEdit *edit, bool currLine)
    : m_textCursor(textcursor)
    , m_sInsertText(text)
    , m_edit(edit)
    , m_iscurrLine(currLine)
{
    qCDebug(appLog) << "DeleteTextUndoCommand2 created";
    m_sInsertText.replace("\r\n", "\n");
}

DeleteTextUndoCommand2::DeleteTextUndoCommand2(QList<QTextEdit::ExtraSelection> &selections,
                                               QString text,
                                               QPlainTextEdit *edit,
                                               bool currLine)
    : m_sInsertText(text)
    , m_ColumnEditSelections(selections)
    , m_edit(edit)
    , m_iscurrLine(currLine)
{
    qCDebug(appLog) << "DeleteTextUndoCommand2 created";
    m_sInsertText.replace("\r\n", "\n");
    int cnt = m_ColumnEditSelections.size();
    for (int i = 0; i < cnt; i++) {
        QTextCursor textCursor = m_ColumnEditSelections[i].cursor;
        if (textCursor.hasSelection()) {
            qCDebug(appLog) << "textCursor.hasSelection()";
            m_selectTextList.append(textCursor.selectedText());
        } else {
            int pos = textCursor.positionInBlock() - 1;
            if (pos >= 0) {
                qCDebug(appLog) << "pos >= 0";
                m_selectTextList.append(textCursor.block().text().at(pos));
            } else {
                //上一行lastQChar
                qCDebug(appLog) << "pos < 0";
                m_selectTextList.append("\n");
            }
        }
    }
    qCDebug(appLog) << "DeleteTextUndoCommand2 created end";
}

void DeleteTextUndoCommand2::undo()
{
    qCDebug(appLog) << "DeleteTextUndoCommand2 undo";
    if (m_ColumnEditSelections.isEmpty()) {
        qCDebug(appLog) << "m_ColumnEditSelections.isEmpty()";
        m_textCursor.setPosition(m_beginPostion);
        m_textCursor.insertText(m_sInsertText);
        m_textCursor.setPosition(m_beginPostion);
        m_edit->setTextCursor(m_textCursor);
    } else {
        qCDebug(appLog) << "m_ColumnEditSelections.isEmpty() is false";
        int cnt = m_ColumnEditSelections.size();
        // 逆序恢复：redo 按序删除时记录的各选区位置在"全部删除后"的坐标系中有效，
        // 逆序插入（先恢复靠后的选区）不会使靠前选区的记录位置发生偏移
        for (int i = cnt - 1; i >= 0; i--) {
            m_ColumnEditSelections[i].cursor.setPosition(m_selectBeginPosList.value(i));
            m_ColumnEditSelections[i].cursor.insertText(m_selectTextList[i]);
            m_ColumnEditSelections[i].cursor.setPosition(m_selectBeginPosList.value(i));
            m_edit->setTextCursor(m_ColumnEditSelections[i].cursor);
        }
    }
    qCDebug(appLog) << "DeleteTextUndoCommand2 undo end";
}

void DeleteTextUndoCommand2::redo()
{
    qCDebug(appLog) << "DeleteTextUndoCommand2 redo";
    if (m_ColumnEditSelections.isEmpty()) {
        qCDebug(appLog) << "m_ColumnEditSelections.isEmpty()";
        bool isEmptyLine = (m_sInsertText.size() == 0);
        bool isBlankLine = (m_sInsertText.trimmed().size() == 0);

        if (!m_iscurrLine) {
            qCDebug(appLog) << "m_iscurrLine is false";
            //删除到行尾
            if (isEmptyLine || m_textCursor.atBlockEnd()) {
                qCDebug(appLog) << "isEmptyLine || m_textCursor.atBlockEnd()";
                m_textCursor.movePosition(QTextCursor::NextCharacter, QTextCursor::KeepAnchor);
            } else if (isBlankLine && m_textCursor.atBlockStart()) {
                qCDebug(appLog) << "isBlankLine && m_textCursor.atBlockStart()";
                m_textCursor.movePosition(QTextCursor::StartOfBlock);
                m_textCursor.movePosition(QTextCursor::EndOfBlock, QTextCursor::KeepAnchor);
            } else {
                qCDebug(appLog) << "else";
                m_textCursor.movePosition(QTextCursor::NoMove, QTextCursor::MoveAnchor);
                m_textCursor.movePosition(QTextCursor::EndOfBlock, QTextCursor::KeepAnchor);
            }
        } else {
            qCDebug(appLog) << "m_iscurrLine is true";
            //删除整行
            m_textCursor.movePosition(QTextCursor::StartOfBlock, QTextCursor::MoveAnchor);
            m_textCursor.movePosition(QTextCursor::EndOfBlock, QTextCursor::KeepAnchor);
            m_textCursor.movePosition(QTextCursor::NextCharacter, QTextCursor::KeepAnchor);
        }

        m_sInsertText = m_textCursor.selectedText();
        m_beginPostion = m_textCursor.selectionStart();
        m_textCursor.deletePreviousChar();

        // 进行撤销/恢复时将光标移动到撤销位置
        m_edit->setTextCursor(m_textCursor);
    } else {
        qCDebug(appLog) << "m_ColumnEditSelections.isEmpty() is false";
        int cnt = m_ColumnEditSelections.size();
        // 每个选区记录各自的起始位置，供 undo 恢复使用（ redo/undo 严格交替，先清空防御重复累积）
        m_selectBeginPosList.clear();
        for (int i = 0; i < cnt; i++) {
            m_selectBeginPosList.append(m_ColumnEditSelections[i].cursor.selectionStart());
            m_ColumnEditSelections[i].cursor.deletePreviousChar();
            m_edit->setTextCursor(m_ColumnEditSelections[i].cursor);
        }
    }
    qCDebug(appLog) << "DeleteTextUndoCommand2 redo end";
}
