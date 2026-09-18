// SPDX-FileCopyrightText: 2017 - 2026 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include "CSyntaxHighlighter.h"
#include <QDebug>

CSyntaxHighlighter::CSyntaxHighlighter(QObject *parent):
    SyntaxHighlighter (parent),
    m_bHighlight(false)
{
    qDebug() << "CSyntaxHighlighter constructor";
}

CSyntaxHighlighter::CSyntaxHighlighter(QTextDocument *pDocument):
    SyntaxHighlighter (pDocument),m_bHighlight(false)
{
    qDebug() << "CSyntaxHighlighter constructor with QTextDocument*";
}

void CSyntaxHighlighter::setEnableHighlight(bool isEnable)
{
    qDebug() << "CSyntaxHighlighter::setEnableHighlight()" << isEnable;
    m_bHighlight = isEnable;
    // 开关变更后重新高亮，避免旧格式残留至下次文档变更，
    // 与 setInvalidCharHighlight 的自动 rehighlight 行为保持一致
    rehighlight();
}

void CSyntaxHighlighter::setInvalidCharHighlight(bool enable)
{
    qDebug() << "CSyntaxHighlighter::setInvalidCharHighlight()" << enable;
    m_bInvalidCharHighlight = enable;
    if (enable) {
        setEnableHighlight(true);
    }
    rehighlight();
}

void CSyntaxHighlighter::highlightBlock(const QString &text)
{
    if (!m_bHighlight) {
        qDebug() << "CSyntaxHighlighter::highlightBlock() is disabled";
        return;
    }

    qDebug() << "CSyntaxHighlighter::highlightBlock()";
    KSyntaxHighlighting::SyntaxHighlighter::highlightBlock(text);

    // 叠加 \00 无效字符高亮：红色背景 + 白色文字
    if (m_bInvalidCharHighlight) {
        QTextCharFormat fmt;
        fmt.setBackground(QColor("#FF0000"));
        fmt.setForeground(QColor("#FFFFFF"));
        // 匹配字面量 \00（反斜杠 + 两个零）
        static const QRegularExpression re("\\\\00");
        QRegularExpressionMatchIterator it = re.globalMatch(text);
        while (it.hasNext()) {
            QRegularExpressionMatch match = it.next();
            setFormat(match.capturedStart(), match.capturedLength(), fmt);
        }
    }
}
