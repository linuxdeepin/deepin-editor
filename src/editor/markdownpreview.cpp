// SPDX-FileCopyrightText: 2026 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include "markdownpreview.h"

#include <QFileInfo>
#include <QTextDocument>
#include <QUrl>

MarkdownPreview::MarkdownPreview(QWidget *parent)
    : QTextBrowser(parent)
{
    setAccessibleName("MarkdownPreviewView");
    setFrameShape(QFrame::NoFrame);
    setOpenLinks(false);
    setOpenExternalLinks(false);
    setReadOnly(true);
}

bool MarkdownPreview::isSupported()
{
#if QT_VERSION >= QT_VERSION_CHECK(6, 5, 0)
    return true;
#else
    return false;
#endif
}

bool MarkdownPreview::isMarkdownFile(const QString &filePath)
{
    const QString suffix = QFileInfo(filePath).suffix().toLower();
    return suffix == QStringLiteral("md") || suffix == QStringLiteral("markdown");
}

void MarkdownPreview::updatePreview(const QString &markdown)
{
#if QT_VERSION >= QT_VERSION_CHECK(6, 5, 0)
    QTextDocument *newDoc = new QTextDocument(this);
    // 主题无关的基础样式：标题分级 + 等宽代码，颜色随应用调色板自适应
    newDoc->setDefaultStyleSheet(QStringLiteral(
        "h1 { font-size: 26px; font-weight: 600; }"
        "h2 { font-size: 22px; font-weight: 600; }"
        "h3 { font-size: 19px; font-weight: 600; }"
        "h4 { font-size: 16px; font-weight: 600; }"
        "h5 { font-size: 14px; font-weight: 600; }"
        "h6 { font-size: 13px; font-weight: 600; }"
        "code, pre { font-family: 'monospace'; }"
        "blockquote { margin-left: 16px; }"));
    newDoc->setMarkdown(markdown);

    // setDocument 会接管新文档（父对象为 this）并释放上一份文档，无需手动 delete
    setDocument(newDoc);
#else
    Q_UNUSED(markdown)
    setPlainText(markdown);
#endif
}

void MarkdownPreview::doSetSource(const QUrl &url, QTextDocument::ResourceType type)
{
    Q_UNUSED(url)
    Q_UNUSED(type)
    // 预览只读，不进行链接跳转
}
