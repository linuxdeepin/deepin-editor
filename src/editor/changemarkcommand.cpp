/*
 * Copyright (C) 2020 ~ 2022 Uniontech Software Technology Co.,Ltd.
 *
 * Author:     RenBin <renbin@uniontech.com>
 *
 * Maintainer: TanLang <tanlang@uniontech.com>
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

#include "changemarkcommand.h"

ChangeMarkCommand::ChangeMarkCommand(QPointer<TextEdit> editPtr, const QList<TextEdit::MarkReplaceInfo> &oldMark, const QList<TextEdit::MarkReplaceInfo> &newMark, QUndoCommand *parent)
    : QUndoCommand(parent)
    , m_EditPtr(editPtr)
    , m_oldMarkReplace(oldMark)
    , m_newMarkReplace(newMark)
{

}

ChangeMarkCommand::~ChangeMarkCommand()
{

}

void ChangeMarkCommand::undo()
{
    // 优先执行子撤销项，保证在文本操作后执行
    QUndoCommand::undo();

    // 插入文本后，恢复旧的颜色标记状态
    if (m_EditPtr && !m_oldMarkReplace.isEmpty()) {
        auto oldMark = TextEdit::convertReplaceToMark(m_oldMarkReplace);
        m_EditPtr->manualUpdateAllMark(oldMark);
    }
}

void ChangeMarkCommand::redo()
{
    // 优先执行子撤销项，保证在文本操作后执行
    QUndoCommand::redo();

    // 插入文本后，更新颜色标记状态
    if (m_EditPtr && !m_newMarkReplace.isEmpty()) {
        auto newMark = TextEdit::convertReplaceToMark(m_newMarkReplace);
        m_EditPtr->manualUpdateAllMark(newMark);
    }
}
