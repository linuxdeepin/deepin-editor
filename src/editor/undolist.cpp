// SPDX-FileCopyrightText: 2022-2026 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include "ddlog.h"
#include "undolist.h"

#include <QDebug>

UndoList::UndoList()
{
    qCDebug(appLog) << "UndoList created";

}
UndoList::~UndoList()
{
    qCDebug(appLog) << "UndoList destroyed, cleaning up commands";
    for (auto& it : m_coms) {
        if(nullptr != it){
            delete it;
            it = nullptr;
        }
    }

    m_coms.clear();
}
void UndoList::appendCom(QUndoCommand* com)
{
    if(nullptr != com){
        qCDebug(appLog) << "Appending undo command:" << com->text();
        m_coms.push_back(com);
    }
}
void UndoList::undo()
{
    qCDebug(appLog) << "Executing undo on" << m_coms.size() << "commands";
    //do the undo operation in the reverse order.
    int total = m_coms.size();
    int processed = 0;
    qCDebug(appLog) << "Starting undo of" << total << "commands";
    
    for(auto it = m_coms.rbegin();it!=m_coms.rend();it++){
        qCDebug(appLog) << "Undoing command:" << (*it)->text();
        (*it)->undo();
        processed++;
        
        if (processed % 10 == 0 || processed == total) {
            qCDebug(appLog) << "Undo progress:" << processed << "/" << total;
        }
    }
}
void UndoList::redo()
{
    qCDebug(appLog) << "Executing redo on" << m_coms.size() << "commands";
    int total = m_coms.size();
    int processed = 0;
    qCDebug(appLog) << "Starting redo of" << total << "commands";
    
    foreach (auto it, m_coms) {
        qCDebug(appLog) << "Redoing command:" << it->text();
        it->redo();
        processed++;
        
        if (processed % 10 == 0 || processed == total) {
            qCDebug(appLog) << "Redo progress:" << processed << "/" << total;
        }
    }
}
