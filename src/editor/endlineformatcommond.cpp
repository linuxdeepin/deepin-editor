// SPDX-FileCopyrightText: 2019 - 2023 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include "endlineformatcommond.h"
#include <QDebug>
EndlineFormartCommand::EndlineFormartCommand(TextEdit* edit,BottomBar* bar,BottomBar::EndlineFormat from,BottomBar::EndlineFormat to):
    m_edit(edit),
    m_bar(bar),
    m_from(from),
    m_to(to)
{
    qDebug() << "EndlineFormartCommand created - edit:" << edit
             << "bar:" << bar << "from:" << from << "to:" << to;
}
EndlineFormartCommand::~EndlineFormartCommand()
{
    qDebug() << "EndlineFormartCommand destroyed";
}

void EndlineFormartCommand::undo()
{
   qInfo() << "EndlineFormartCommand undo - setting format to:" << m_from;
   m_bar->setEndlineMenuText(m_from);
}
void EndlineFormartCommand::redo()
{
    qInfo() << "EndlineFormartCommand redo - setting format to:" << m_to;
    auto cursor = m_edit->textCursor();
    auto pos = cursor.position();
    cursor.insertText(" ");
    cursor.setPosition(pos);
    cursor.deleteChar();
    m_bar->setEndlineMenuText(m_to);
}
