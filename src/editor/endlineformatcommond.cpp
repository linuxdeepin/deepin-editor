#include "endlineformatcommond.h"
EndlineFormartCommand::EndlineFormartCommand(BottomBar* bar,BottomBar::EndlineFormat from,BottomBar::EndlineFormat to):
    m_bar(bar),
    m_from(from),
    m_to(to)
{

}
EndlineFormartCommand::~EndlineFormartCommand()
{

}

void EndlineFormartCommand::undo()
{
   m_bar->setEndlineMenuText(m_from);
}
void EndlineFormartCommand::redo()
{
    m_bar->setEndlineMenuText(m_to);
}
