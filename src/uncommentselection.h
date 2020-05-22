#pragma once

#include "comment_global.h"

#include <QString>
#include<QtDebug>

QT_BEGIN_NAMESPACE
class QTextEdit;
QT_END_NAMESPACE

namespace Comment {
    class COMMENT_EXPORT CommentDefinition
    {
    public:
    CommentDefinition();

    enum Style { NoStyle, CppStyle, HashStyle };
    void setComments(QString singleLineComment, QString multiLineCommentStart, QString multiLineCommentEnd);

    bool isValid() const;
    bool hasSingleLineStyle() const;
    bool hasMultiLineStyle() const;

    public:
    bool isAfterWhiteSpaces;
    QString singleLine;
    QString multiLineStart;
    QString multiLineEnd;
    };

    COMMENT_EXPORT
    void unCommentSelection(QTextEdit *edit,
                        const CommentDefinition &definiton = CommentDefinition());
    void setComment(QTextEdit *edit,
                    const CommentDefinition &definiton = CommentDefinition(),QString name="");

    void removeComment(QTextEdit *edit,
                       const CommentDefinition &definiton = CommentDefinition(),QString name="");
}
