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

#ifndef CHANGEMARKCOMMAND_H
#define CHANGEMARKCOMMAND_H

#include <QUndoCommand>
#include "dtextedit.h"

// 用于进行颜色标记前后变更撤销恢复
class ChangeMarkCommand : public QUndoCommand
{
public:
    ChangeMarkCommand(QPointer<TextEdit> editPtr,
                      const QList<TextEdit::MarkReplaceInfo> &oldMark,
                      const QList<TextEdit::MarkReplaceInfo> &newMark,
                      QUndoCommand *parent = nullptr);
    virtual ~ChangeMarkCommand();

    virtual void undo();
    virtual void redo();

private:
    QPointer<TextEdit> m_EditPtr;     // 操作的文本编辑器对象指针
    QList<TextEdit::MarkReplaceInfo> m_oldMarkReplace;  // 缓存的标识操作记录
    QList<TextEdit::MarkReplaceInfo> m_newMarkReplace;  // 新的标识操作记录
};

#endif // CHANGEMARKCOMMAND_H
