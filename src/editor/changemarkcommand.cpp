// SPDX-FileCopyrightText: 2020-2026 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include "ddlog.h"
#include "changemarkcommand.h"
#include <QDebug>

ChangeMarkCommand::ChangeMarkCommand(QPointer<TextEdit> editPtr, const QList<TextEdit::MarkReplaceInfo> &oldMark, const QList<TextEdit::MarkReplaceInfo> &newMark, QUndoCommand *parent)
    : QUndoCommand(parent)
    , m_EditPtr(editPtr)
    , m_oldMarkReplace(oldMark)
    , m_newMarkReplace(newMark)
{
    qCDebug(appLog) << "ChangeMarkCommand created with"
                << oldMark.size() << "old marks and"
                << newMark.size() << "new marks";

}

ChangeMarkCommand::~ChangeMarkCommand()
{
    qCDebug(appLog) << "ChangeMarkCommand destroyed";

}

void ChangeMarkCommand::undo()
{
    qCDebug(appLog) << "Executing ChangeMarkCommand undo";
    // 优先执行子撤销项，保证在文本操作后执行
    QUndoCommand::undo();

    // 插入文本后，恢复旧的颜色标记状态
    if (m_EditPtr && !m_oldMarkReplace.isEmpty()) {
        qCDebug(appLog) << "m_EditPtr is not null";
        auto oldMark = TextEdit::convertReplaceToMark(m_oldMarkReplace);
        m_EditPtr->manualUpdateAllMark(oldMark);
    }
    qCDebug(appLog) << "ChangeMarkCommand undo end";
}

void ChangeMarkCommand::redo()
{
    qCDebug(appLog) << "Executing ChangeMarkCommand redo";
    // 优先执行子撤销项，保证在文本操作后执行
    QUndoCommand::redo();

    // 插入文本后，更新颜色标记状态
    if (m_EditPtr && !m_newMarkReplace.isEmpty()) {
        qCDebug(appLog) << "m_EditPtr is not null";
        auto newMark = TextEdit::convertReplaceToMark(m_newMarkReplace);
        m_EditPtr->manualUpdateAllMark(newMark);
    }
    qCDebug(appLog) << "ChangeMarkCommand redo end";
}
