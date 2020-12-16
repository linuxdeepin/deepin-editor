/*
 * Copyright (C) 2017 ~ 2018 Deepin Technology Co., Ltd.
 *
 * Author:     rekols <rekols@foxmail.com>
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

#include "CSyntaxHighlighter.h"
#include "utils.h"
#include "../encodes/detectcode.h"
#include "../editor/uncommentselection.h"
#include <QFile>
#include <QFileInfo>
#include <QDebug>
#include <QXmlStreamReader>


CSyntaxHighlighter::CSyntaxHighlighter(const QString &filepath, QObject *parent)
    : QThread(parent),
      m_sFilePath(filepath)
{

}

CSyntaxHighlighter::~CSyntaxHighlighter()
{
}

void CSyntaxHighlighter::run()
{
    Comment::CommentDefinition commentDefinition;
    KSyntaxHighlighting::Repository repository;
    QVector<KSyntaxHighlighting::Definition> defs =repository.definitions();
    KSyntaxHighlighting::Definition def = repository.definitionForFileName(m_sFilePath);
    if (def.isValid()) {
        qDebug()<<"KSyntaxHighlighting::Definition Path "<<def.filePath();
        qDebug()<<"def.singleLineCommentMarker():"<< def.singleLineCommentMarker();
        qDebug()<<"def.multiLineCommentMarker():"<< def.multiLineCommentMarker();
        qDebug()<<def.section()<<def.name()<<def.translatedSection()<<def.translatedName();
    }
   emit sigDefinition(def);
}
