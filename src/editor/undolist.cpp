// SPDX-FileCopyrightText: 2022 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include "undolist.h"

#include <QDebug>

UndoList::UndoList()
{
    qDebug() << "UndoList created";

}
UndoList::~UndoList()
{
    qDebug() << "UndoList destroyed, cleaning up commands";
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
        qDebug() << "Appending undo command:" << com->text();
        m_coms.push_back(com);
    }
}
void UndoList::undo()
{
    qDebug() << "Executing undo on" << m_coms.size() << "commands";
    //do the undo operation in the reverse order.
    int total = m_coms.size();
    int processed = 0;
    qDebug() << "Starting undo of" << total << "commands";
    
    for(auto it = m_coms.rbegin();it!=m_coms.rend();it++){
        qDebug() << "Undoing command:" << (*it)->text();
        (*it)->undo();
        processed++;
        
        if (processed % 10 == 0 || processed == total) {
            qDebug() << "Undo progress:" << processed << "/" << total;
        }
    }
}
void UndoList::redo()
{
    qDebug() << "Executing redo on" << m_coms.size() << "commands";
    int total = m_coms.size();
    int processed = 0;
    qDebug() << "Starting redo of" << total << "commands";
    
    foreach (auto it, m_coms) {
        qDebug() << "Redoing command:" << it->text();
        it->redo();
        processed++;
        
        if (processed % 10 == 0 || processed == total) {
            qDebug() << "Redo progress:" << processed << "/" << total;
        }
    }
}
