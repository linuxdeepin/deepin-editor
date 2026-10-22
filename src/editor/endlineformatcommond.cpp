// SPDX-FileCopyrightText: 2019-2026 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include "ddlog.h"
#include "endlineformatcommond.h"
#include <QDebug>
EndlineFormartCommand::EndlineFormartCommand(TextEdit* edit,BottomBar* bar,BottomBar::EndlineFormat from,BottomBar::EndlineFormat to):
    m_edit(edit),
    m_bar(bar),
    m_from(from),
    m_to(to)
{
    qCDebug(appLog) << "EndlineFormartCommand created - edit:" << edit
             << "bar:" << bar << "from:" << from << "to:" << to;
}
EndlineFormartCommand::~EndlineFormartCommand()
{
    qCDebug(appLog) << "EndlineFormartCommand destroyed";
}

void EndlineFormartCommand::undo()
{
   qCInfo(appLog) << "EndlineFormartCommand undo - setting format to:" << m_from;
   m_bar->setEndlineMenuText(m_from);
}
void EndlineFormartCommand::redo()
{
    qCInfo(appLog) << "EndlineFormartCommand redo - setting format to:" << m_to;
    auto cursor = m_edit->textCursor();
    auto pos = cursor.position();
    cursor.insertText(" ");
    cursor.setPosition(pos);
    cursor.deleteChar();
    m_bar->setEndlineMenuText(m_to);
}
