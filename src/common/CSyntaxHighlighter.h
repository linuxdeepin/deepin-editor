// SPDX-FileCopyrightText: 2017 - 2026 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once
#include <KSyntaxHighlighting/Repository>
#include <KSyntaxHighlighting/Definition>
#include <KSyntaxHighlighting/SyntaxHighlighter>
#include <QRegularExpression>
#include <QTextCharFormat>
#include <QColor>

using namespace KSyntaxHighlighting;
class CSyntaxHighlighter : public SyntaxHighlighter
{
    Q_OBJECT
public:
    explicit CSyntaxHighlighter(QObject *parent = nullptr);
    explicit CSyntaxHighlighter(QTextDocument *pDocument);
    void setEnableHighlight(bool isEnable);
    void setInvalidCharHighlight(bool enable);

protected:
    virtual void highlightBlock(const QString & text) override;

private:
    bool m_bHighlight = false;
    bool m_bInvalidCharHighlight = false;
};
