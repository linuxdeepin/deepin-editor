/*
* Copyright (C) 2019 ~ 2021 Uniontech Software Technology Co.,Ltd.
*
* Author:     guoshaoyu <guoshaoyu@uniontech.com>
*
* Maintainer: guoshaoyu <guoshaoyu@uniontech.com>
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
#include "uncommentselection.h"
#include <QTextBlock>

using namespace Comment;

CommentDefinition::CommentDefinition() :
    isAfterWhiteSpaces(false)
{}

void CommentDefinition::setComments(QString singleLineComment, QString multiLineCommentStart, QString multiLineCommentEnd)
{
    singleLine = singleLineComment;
    multiLineStart = multiLineCommentStart;
    multiLineEnd = multiLineCommentEnd;
}

bool CommentDefinition::isValid() const
{
    return hasSingleLineStyle() || hasMultiLineStyle();
}

bool CommentDefinition::hasSingleLineStyle() const
{
    return !singleLine.isEmpty();
}

bool CommentDefinition::hasMultiLineStyle() const
{
    return !multiLineStart.isEmpty() && !multiLineEnd.isEmpty();
}

