// SPDX-FileCopyrightText: 2026 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

#ifndef MARKDOWNPREVIEW_H
#define MARKDOWNPREVIEW_H

#include <QTextBrowser>

/**
 * @brief Markdown 可视化预览控件
 *
 * 将 markdown 文本实时渲染为富文本展示，基于 QTextDocument::setMarkdown（Qt >= 6.5）。
 * Qt5 构建下预览功能整体禁用（isSupported() 返回 false）。
 */
class MarkdownPreview : public QTextBrowser
{
    Q_OBJECT

public:
    explicit MarkdownPreview(QWidget *parent = nullptr);

    // 当前构建是否支持 markdown 预览（需要 Qt >= 6.5）
    static bool isSupported();

    // 是否为可预览的 markdown 文件（.md / .markdown）
    static bool isMarkdownFile(const QString &filePath);

    // 以 markdown 内容刷新预览
    void updatePreview(const QString &markdown);

protected:
    // 预览为只读展示，忽略超链接跳转
    void doSetSource(const QUrl &url, QTextDocument::ResourceType type = QTextDocument::UnknownResource) override;
};

#endif // MARKDOWNPREVIEW_H
