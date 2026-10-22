// SPDX-FileCopyrightText: 2019-2026 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include "ddlog.h"
#include "uncommentselection.h"
#include <QTextBlock>
#include <QDebug>

using namespace Comment;
CommentDefinition::CommentDefinition() :
    isAfterWhiteSpaces(false)
{
    qCDebug(appLog) << "CommentDefinition constructor called";
    qCDebug(appLog) << "CommentDefinition::CommentDefinition() exit";
}


void CommentDefinition::setComments(QString singleLineComment, QString multiLineCommentStart, QString multiLineCommentEnd)
{
    qCInfo(appLog) << "Setting comments - singleLine:" << singleLineComment
            << "multiLineStart:" << multiLineCommentStart
            << "multiLineEnd:" << multiLineCommentEnd;
    singleLine = singleLineComment;
    multiLineStart = multiLineCommentStart;
    multiLineEnd = multiLineCommentEnd;
    qCDebug(appLog) << "CommentDefinition::setComments() exit";
}
bool CommentDefinition::isValid() const
{
    qCDebug(appLog) << "CommentDefinition::isValid() entry";
    bool result = hasSingleLineStyle() || hasMultiLineStyle();
    qCDebug(appLog) << "CommentDefinition::isValid() exit, result:" << result;
    return result;
}

bool CommentDefinition::hasSingleLineStyle() const
{
    qCDebug(appLog) << "CommentDefinition::hasSingleLineStyle() entry";
    bool result = !singleLine.isEmpty();
    qCDebug(appLog) << "CommentDefinition::hasSingleLineStyle() exit, result:" << result;
    return result;
}

bool CommentDefinition::hasMultiLineStyle() const
{
    qCDebug(appLog) << "CommentDefinition::hasMultiLineStyle() entry";
    bool result = !multiLineStart.isEmpty() && !multiLineEnd.isEmpty();
    qCDebug(appLog) << "CommentDefinition::hasMultiLineStyle() exit, result:" << result;
    return result;
}

