// SPDX-FileCopyrightText: 2019 - 2022 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include "uncommentselection.h"
#include <QTextBlock>
#include <QDebug>

using namespace Comment;
CommentDefinition::CommentDefinition() :
    isAfterWhiteSpaces(false)
{
    qDebug() << "CommentDefinition constructor called";
    qDebug() << "CommentDefinition::CommentDefinition() exit";
}


void CommentDefinition::setComments(QString singleLineComment, QString multiLineCommentStart, QString multiLineCommentEnd)
{
    qInfo() << "Setting comments - singleLine:" << singleLineComment
            << "multiLineStart:" << multiLineCommentStart
            << "multiLineEnd:" << multiLineCommentEnd;
    singleLine = singleLineComment;
    multiLineStart = multiLineCommentStart;
    multiLineEnd = multiLineCommentEnd;
    qDebug() << "CommentDefinition::setComments() exit";
}
bool CommentDefinition::isValid() const
{
    qDebug() << "CommentDefinition::isValid() entry";
    bool result = hasSingleLineStyle() || hasMultiLineStyle();
    qDebug() << "CommentDefinition::isValid() exit, result:" << result;
    return result;
}

bool CommentDefinition::hasSingleLineStyle() const
{
    qDebug() << "CommentDefinition::hasSingleLineStyle() entry";
    bool result = !singleLine.isEmpty();
    qDebug() << "CommentDefinition::hasSingleLineStyle() exit, result:" << result;
    return result;
}

bool CommentDefinition::hasMultiLineStyle() const
{
    qDebug() << "CommentDefinition::hasMultiLineStyle() entry";
    bool result = !multiLineStart.isEmpty() && !multiLineEnd.isEmpty();
    qDebug() << "CommentDefinition::hasMultiLineStyle() exit, result:" << result;
    return result;
}

