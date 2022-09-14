// SPDX-FileCopyrightText: 2019 - 2022 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

#ifndef INSERTTEXTUNDOCOMMAND_H
#define INSERTTEXTUNDOCOMMAND_H

#include <QObject>
#include <QUndoCommand>
#include <QTextCursor>
#include <QTextEdit>
#include <QPlainTextEdit>

class InsertTextUndoCommand : public QUndoCommand
{
public:
    explicit InsertTextUndoCommand(QTextCursor textcursor, QString text, QPlainTextEdit *edit, QUndoCommand *parent = nullptr);
    explicit InsertTextUndoCommand(QList<QTextEdit::ExtraSelection> &selections, QString text, QPlainTextEdit *edit, QUndoCommand *parent = nullptr);
    virtual void undo();
    virtual void redo();

private:
    QPlainTextEdit *m_pEdit = nullptr;
    QTextCursor m_textCursor;
    int m_beginPostion {0};
    int m_endPostion   {0};
    QString m_sInsertText;
    QList<QTextEdit::ExtraSelection> m_ColumnEditSelections;
    QString m_selectText = QString();
};

#endif // INSERTTEXTUNDOCOMMAND_H
