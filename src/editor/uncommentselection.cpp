#include "uncommentselection.h"
#include <QPlainTextEdit>
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

static bool isComment(const QString &text, int index, const QString &commentType)
{
    const int length = commentType.length();

    Q_ASSERT(text.length() - index >= length);

    int i = 0;
    while (i < length) {
        if (text.at(index + i) != commentType.at(i))
            return false;
        ++i;
    }
    return true;
}

void Comment::unCommentSelection(QPlainTextEdit *edit, const CommentDefinition &definition)
{
    if (!definition.isValid())
        return;

    QTextCursor cursor = edit->textCursor();
    QTextDocument *doc = cursor.document();
    cursor.beginEditBlock();

    int pos = cursor.position();
    int anchor = cursor.anchor();
    int start = qMin(anchor, pos);
    int end = qMax(anchor, pos);
    bool anchorIsStart = (anchor == start);

    QTextBlock startBlock = doc->findBlock(start);
    QTextBlock endBlock = doc->findBlock(end);

    if (end > start && endBlock.position() == end) {
        --end;
        endBlock = endBlock.previous();
    }

    bool doMultiLineStyleUncomment = false;
    bool doMultiLineStyleComment = false;
    bool doSingleLineStyleUncomment = false;

    bool hasSelection = cursor.hasSelection();

    if (hasSelection && definition.hasMultiLineStyle()) {

        QString startText = startBlock.text();
        int startPos = start - startBlock.position();
        const int multiLineStartLength = definition.multiLineStart.length();
        bool hasLeadingCharacters = !startText.left(startPos).trimmed().isEmpty();

        if (startPos >= multiLineStartLength
            && isComment(startText,
                         startPos - multiLineStartLength,
                         definition.multiLineStart)) {
            startPos -= multiLineStartLength;
            start -= multiLineStartLength;
        }

        bool hasSelStart = startPos <= startText.length() - multiLineStartLength
            && isComment(startText, startPos, definition.multiLineStart);

        QString endText = endBlock.text();
        int endPos = end - endBlock.position();
        const int multiLineEndLength = definition.multiLineEnd.length();
        bool hasTrailingCharacters =
            !endText.left(endPos).remove(definition.singleLine).trimmed().isEmpty()
            && !endText.mid(endPos).trimmed().isEmpty();

        if (endPos <= endText.length() - multiLineEndLength
            && isComment(endText, endPos, definition.multiLineEnd)) {
            endPos += multiLineEndLength;
            end += multiLineEndLength;
        }

        bool hasSelEnd = endPos >= multiLineEndLength
            && isComment(endText, endPos - multiLineEndLength, definition.multiLineEnd);

        doMultiLineStyleUncomment = hasSelStart && hasSelEnd;
        doMultiLineStyleComment = !doMultiLineStyleUncomment
            && (hasLeadingCharacters
                || hasTrailingCharacters
                || !definition.hasSingleLineStyle());
    } else if (!hasSelection && !definition.hasSingleLineStyle()) {

        QString text = startBlock.text().trimmed();
        doMultiLineStyleUncomment = text.startsWith(definition.multiLineStart)
            && text.endsWith(definition.multiLineEnd);
        doMultiLineStyleComment = !doMultiLineStyleUncomment && !text.isEmpty();

        start = startBlock.position();
        end = endBlock.position() + endBlock.length() - 1;

        if (doMultiLineStyleUncomment) {
            int offset = 0;
            text = startBlock.text();
            const int length = text.length();
            while (offset < length && text.at(offset).isSpace())
                ++offset;
            start += offset;
        }
    }

    if (doMultiLineStyleUncomment) {
        cursor.setPosition(end);
        cursor.movePosition(QTextCursor::PreviousCharacter,
                            QTextCursor::KeepAnchor,
                            definition.multiLineEnd.length());
        cursor.removeSelectedText();
        cursor.setPosition(start);
        cursor.movePosition(QTextCursor::NextCharacter,
                            QTextCursor::KeepAnchor,
                            definition.multiLineStart.length());
        cursor.removeSelectedText();
    } else if (doMultiLineStyleComment) {
        cursor.setPosition(end);
        cursor.insertText(definition.multiLineEnd);
        cursor.setPosition(start);
        cursor.insertText(definition.multiLineStart);
    } else {
        endBlock = endBlock.next();
        doSingleLineStyleUncomment = true;
        for (QTextBlock block = startBlock; block != endBlock; block = block.next()) {
            QString text = block.text().trimmed();
            if (!text.isEmpty() && !text.startsWith(definition.singleLine)) {
                doSingleLineStyleUncomment = false;
                break;
            }
        }

        const int singleLineLength = definition.singleLine.length();
        for (QTextBlock block = startBlock; block != endBlock; block = block.next()) {
            if (doSingleLineStyleUncomment) {
                QString text = block.text();
                int i = 0;
                while (i <= text.size() - singleLineLength) {
                    if (isComment(text, i, definition.singleLine)) {
                        cursor.setPosition(block.position() + i);
                        cursor.movePosition(QTextCursor::NextCharacter,
                                            QTextCursor::KeepAnchor,
                                            singleLineLength);
                        cursor.removeSelectedText();
                        break;
                    }
                    if (!text.at(i).isSpace())
                        break;
                    ++i;
                }
            } else {
                const QString text = block.text();
                foreach (QChar c, text) {
                    if (!c.isSpace()) {
                        if (definition.isAfterWhiteSpaces)
                            cursor.setPosition(block.position() + text.indexOf(c));
                        else
                            cursor.setPosition(block.position());
                        cursor.insertText(definition.singleLine);
                        break;
                    }
                }
            }
        }
    }

    cursor.endEditBlock();

    // adjust selection when commenting out
    if (hasSelection && !doMultiLineStyleUncomment && !doSingleLineStyleUncomment) {
        cursor = edit->textCursor();
        if (!doMultiLineStyleComment)
            start = startBlock.position(); // move the comment into the selection
        int lastSelPos = anchorIsStart ? cursor.position() : cursor.anchor();
        if (anchorIsStart) {
            cursor.setPosition(start);
            cursor.setPosition(lastSelPos, QTextCursor::KeepAnchor);
        } else {
            cursor.setPosition(lastSelPos);
            cursor.setPosition(start, QTextCursor::KeepAnchor);
        }
        edit->setTextCursor(cursor);
    }
}

void Comment::removeComment(QPlainTextEdit *edit, const CommentDefinition &definition,QString name)
{
    int tmp = 0;//备注偏移量，判断备注标记后面有没有空格
    //此函数是删除了unCommentSelection()的if-else的comment分支得来的
    if (!definition.isValid())
        return;

    qDebug()<<definition.multiLineStart<<definition.multiLineEnd<<definition.singleLine;
    //definition.singleLine="//";
    QString tep = definition.singleLine;
    QString abb = tep.remove(QRegExp("\\s"));


    QTextCursor cursor = edit->textCursor();
    QTextDocument *doc = cursor.document();
    cursor.beginEditBlock();

    int pos = cursor.position();
    int anchor = cursor.anchor();
    int start = qMin(anchor, pos);
    int end = qMax(anchor, pos);
    bool anchorIsStart = (anchor == start);

    QTextBlock startBlock = doc->findBlock(start);
    QTextBlock endBlock = doc->findBlock(end);

    if (end > start && endBlock.position() == end) {
        --end;
        endBlock = endBlock.previous();
    }

    bool doMultiLineStyleUncomment = false;
    bool doMultiLineStyleComment = false;
    bool doSingleLineStyleUncomment = false;

    bool hasSelection = cursor.hasSelection();

    if (hasSelection && definition.hasMultiLineStyle()) {

        QString startText = startBlock.text();
        int startPos = start - startBlock.position();
        const int multiLineStartLength = definition.multiLineStart.length();
        bool hasLeadingCharacters = !startText.left(startPos).trimmed().isEmpty();

        if (startPos >= multiLineStartLength
            && isComment(startText,
                         startPos - multiLineStartLength,
                         definition.multiLineStart)) {
            startPos -= multiLineStartLength;
            start -= multiLineStartLength;
        }

        bool hasSelStart = startPos <= startText.length() - multiLineStartLength
            && isComment(startText, startPos, definition.multiLineStart);

        QString endText = endBlock.text();
        int endPos = end - endBlock.position();
        const int multiLineEndLength = definition.multiLineEnd.length();
        bool hasTrailingCharacters =
            !endText.left(endPos).remove(definition.singleLine).trimmed().isEmpty()
            && !endText.mid(endPos).trimmed().isEmpty();

        if (endPos <= endText.length() - multiLineEndLength
            && isComment(endText, endPos, definition.multiLineEnd)) {
            endPos += multiLineEndLength;
            end += multiLineEndLength;
        }

        bool hasSelEnd = endPos >= multiLineEndLength
            && isComment(endText, endPos - multiLineEndLength, definition.multiLineEnd);

        doMultiLineStyleUncomment = hasSelStart && hasSelEnd;
        doMultiLineStyleComment = !doMultiLineStyleUncomment
            && (hasLeadingCharacters
                || hasTrailingCharacters
                || !definition.hasSingleLineStyle());
    } else if (!hasSelection && (!definition.hasSingleLineStyle()||name=="HTML"||name=="CSS")) {

        QString text = startBlock.text().trimmed();
        doMultiLineStyleUncomment = text.startsWith(definition.multiLineStart)
            && text.endsWith(definition.multiLineEnd);
        doMultiLineStyleComment = !doMultiLineStyleUncomment && !text.isEmpty();
        start = startBlock.position();
        end = endBlock.position() + endBlock.length() - 1;

        if (doMultiLineStyleUncomment) {
            int offset = 0;
            text = startBlock.text();
            const int length = text.length();
            while (offset < length && text.at(offset).isSpace())
                ++offset;
            start += offset;
        }
    }


    if (doMultiLineStyleUncomment) {
        cursor.setPosition(end);
        cursor.movePosition(QTextCursor::PreviousCharacter,
                            QTextCursor::KeepAnchor,
                            definition.multiLineEnd.length());
        cursor.removeSelectedText();
        cursor.setPosition(start);
        cursor.movePosition(QTextCursor::NextCharacter,
                            QTextCursor::KeepAnchor,
                            definition.multiLineStart.length());
        cursor.removeSelectedText();
    }
     else {
        endBlock = endBlock.next();
        doSingleLineStyleUncomment = true;
        for (QTextBlock block = startBlock; block != endBlock; block = block.next()) {
            QString text = block.text().trimmed();
            if (!text.isEmpty() && (!text.startsWith(definition.singleLine))) {
                if(!text.startsWith(abb)){
                doSingleLineStyleUncomment = false;
                break;
                }
            }
        }

        const int singleLineLength = definition.singleLine.length();
        QString text = startBlock.text().trimmed();
        if(name=="HTML"){
            if(text.startsWith("<!--")&&text.endsWith("-->")){
                cursor.setPosition(end);
                cursor.movePosition(QTextCursor::PreviousCharacter,
                                    QTextCursor::KeepAnchor,
                                    4);
                cursor.removeSelectedText();
                cursor.setPosition(start);
                cursor.movePosition(QTextCursor::NextCharacter,
                                    QTextCursor::KeepAnchor,
                                    4);
                cursor.removeSelectedText();
            }
        }
        if(name=="CSS"){
            if(text.startsWith("/*")&&text.endsWith("*/")){
                cursor.setPosition(end);
                cursor.movePosition(QTextCursor::PreviousCharacter,
                                    QTextCursor::KeepAnchor,
                                    2);
                cursor.removeSelectedText();
                cursor.setPosition(start);
                cursor.movePosition(QTextCursor::NextCharacter,
                                    QTextCursor::KeepAnchor,
                                    2);
                cursor.removeSelectedText();
            }
        }
        if(text.startsWith(definition.singleLine)){
                tmp=0;
        }
        else if(text.startsWith(abb)){
            tmp=1;
        }
            QString check="";
        for (QTextBlock block = startBlock; block != endBlock; block = block.next()) {
            if (doSingleLineStyleUncomment) {
                QString text = block.text();
                int i = 0;
                if(tmp==1)
                check = abb;
                else {
                    check=definition.singleLine;
                }
                while (i <= text.size() - singleLineLength) {
                    if (isComment(text, i, check)) {
                        cursor.setPosition(block.position() + i);
                        cursor.movePosition(QTextCursor::NextCharacter,
                                            QTextCursor::KeepAnchor,
                                            singleLineLength-tmp);
                        cursor.removeSelectedText();
                        break;
                    }
                    if (!text.at(i).isSpace())
                        break;
                    ++i;
                }
            }
        }
    }

    cursor.endEditBlock();

    // adjust selection when commenting out
    if (hasSelection && !doMultiLineStyleUncomment && !doSingleLineStyleUncomment) {
        cursor = edit->textCursor();
        if (!doMultiLineStyleComment)
            start = startBlock.position(); // move the comment into the selection
        int lastSelPos = anchorIsStart ? cursor.position() : cursor.anchor();
        if (anchorIsStart) {
            cursor.setPosition(start);
            cursor.setPosition(lastSelPos, QTextCursor::KeepAnchor);
        } else {
            cursor.setPosition(lastSelPos);
            cursor.setPosition(start, QTextCursor::KeepAnchor);
        }
        edit->setTextCursor(cursor);
    }

}


void Comment::setComment(QPlainTextEdit *edit, const CommentDefinition &definiton,QString name)
{
    //此函数是删除了unCommentSelection()的if-else的uncomment分支得来的
    if (!definiton.isValid())
        return;
    QTextCursor cursor = edit->textCursor();
    QTextDocument *doc = cursor.document();
    cursor.beginEditBlock();

    int pos = cursor.position();
    int anchor = cursor.anchor();
    int start = qMin(anchor, pos);
    int end = qMax(anchor, pos);
    bool anchorIsStart = (anchor == start);

    QTextBlock startBlock = doc->findBlock(start);
    QTextBlock endBlock = doc->findBlock(end);
    if (end > start && endBlock.position() == end) {
        --end;
        endBlock = endBlock.previous();
    }
    bool doMultiLineStyleUncomment = false;
    bool doMultiLineStyleComment = false;
    bool doSingleLineStyleUncomment = false;

    bool hasSelection = cursor.hasSelection();

    if (hasSelection && definiton.hasMultiLineStyle()) {

        QString startText = startBlock.text();
        int startPos = start - startBlock.position();
        const int multiLineStartLength = definiton.multiLineStart.length();
        bool hasLeadingCharacters = !startText.left(startPos).trimmed().isEmpty();

        if (startPos >= multiLineStartLength
            && isComment(startText,
                         startPos - multiLineStartLength,
                         definiton.multiLineStart)) {
            startPos -= multiLineStartLength;
            start -= multiLineStartLength;
        }

        bool hasSelStart = startPos <= startText.length() - multiLineStartLength
            && isComment(startText, startPos, definiton.multiLineStart);

        QString endText = endBlock.text();
        int endPos = end - endBlock.position();
        const int multiLineEndLength = definiton.multiLineEnd.length();
        bool hasTrailingCharacters =
            !endText.left(endPos).remove(definiton.singleLine).trimmed().isEmpty()
            && !endText.mid(endPos).trimmed().isEmpty();

        if (endPos <= endText.length() - multiLineEndLength
            && isComment(endText, endPos, definiton.multiLineEnd)) {
            endPos += multiLineEndLength;
            end += multiLineEndLength;
        }

        bool hasSelEnd = endPos >= multiLineEndLength
            && isComment(endText, endPos - multiLineEndLength, definiton.multiLineEnd);

        doMultiLineStyleUncomment = hasSelStart && hasSelEnd;
        doMultiLineStyleComment = !doMultiLineStyleUncomment
            && (hasLeadingCharacters
                || hasTrailingCharacters
                || !definiton.hasSingleLineStyle());
    } else if (!hasSelection && (!definiton.hasSingleLineStyle()||name=="HTML"||name=="CSS")) {

        QString text = startBlock.text().trimmed();
        doMultiLineStyleUncomment = text.startsWith(definiton.multiLineStart)
            && text.endsWith(definiton.multiLineEnd);
        qDebug()<<definiton.multiLineStart;
        doMultiLineStyleComment = !doMultiLineStyleUncomment && !text.isEmpty();

        qDebug()<<",,,,,,,,,,,,,,,,,,,,,,,,,,没有" <<!text.isEmpty()<<text<<end<<start;

        start = startBlock.position();
        end = endBlock.position() + endBlock.length() - 1;

        if (doMultiLineStyleUncomment) {
            int offset = 0;
            text = startBlock.text();
            const int length = text.length();
            while (offset < length && text.at(offset).isSpace())
                ++offset;
            start += offset;
        }
    }

     if (doMultiLineStyleComment) {
         if(name=="HTML")
         {
             cursor.setPosition(end);
             cursor.insertText("-->");
             cursor.setPosition(start);
             cursor.insertText("<!--");
         }
         else if(name=="CSS")
         {
             cursor.setPosition(end);
             cursor.insertText("*/");
             cursor.setPosition(start);
             cursor.insertText("/*");
         }
         else {
             cursor.setPosition(end);
             cursor.insertText(definiton.multiLineEnd);
             cursor.setPosition(start);
             cursor.insertText(definiton.multiLineStart);
         }
    } else {
        endBlock = endBlock.next();
        doSingleLineStyleUncomment = true;
        for (QTextBlock block = startBlock; block != endBlock; block = block.next()) {
            QString text = block.text().trimmed();
            if (!text.isEmpty() && !text.startsWith(definiton.singleLine)) {
                doSingleLineStyleUncomment = false;
                break;
            }
        }
        if(name=="HTML")
        {
            QString  text = startBlock.text().trimmed();
            qDebug()<<"光标有没有" <<!text.isEmpty()<<text<<end<<start;
            cursor.setPosition(end);
            cursor.insertText("-->");
            cursor.setPosition(start);
            cursor.insertText("<!--");
        }
        else if(name=="CSS")
        {
            cursor.setPosition(end);
            cursor.insertText("*/");
            cursor.setPosition(start);
            cursor.insertText("/*");
        }
        else
        {
            for (QTextBlock block = startBlock; block != endBlock; block = block.next()) {
                const QString text = block.text();
                foreach (QChar c, text) {
                    if (!c.isSpace()) {
                        if (definiton.isAfterWhiteSpaces)
                            cursor.setPosition(block.position() + text.indexOf(c));
                        else
                            cursor.setPosition(block.position());
                            cursor.insertText(definiton.singleLine);

                        break;
                    }
                }
            }
        }
    }

    cursor.endEditBlock();

    // adjust selection when commenting out
    if (hasSelection && !doMultiLineStyleUncomment && !doSingleLineStyleUncomment) {
        cursor = edit->textCursor();
        if (!doMultiLineStyleComment)
            start = startBlock.position(); // move the comment into the selection
        int lastSelPos = anchorIsStart ? cursor.position() : cursor.anchor();
        if (anchorIsStart) {
            cursor.setPosition(start);
            cursor.setPosition(lastSelPos, QTextCursor::KeepAnchor);
        } else {
            cursor.setPosition(lastSelPos);
            cursor.setPosition(start, QTextCursor::KeepAnchor);
        }
        edit->setTextCursor(cursor);
    }
}
