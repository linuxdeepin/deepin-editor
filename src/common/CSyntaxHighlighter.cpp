// SPDX-FileCopyrightText: 2017 - 2022 UnionTech Software Technology Co., Ltd.
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
}

void CSyntaxHighlighter::highlightBlock(const QString &text)
{
    if (!m_bHighlight) {
        qDebug() << "CSyntaxHighlighter::highlightBlock() is disabled";
        return;
    }

    qDebug() << "CSyntaxHighlighter::highlightBlock()";
    KSyntaxHighlighting::SyntaxHighlighter::highlightBlock(text);
}
