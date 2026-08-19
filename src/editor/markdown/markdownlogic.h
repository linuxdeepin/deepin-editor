// SPDX-FileCopyrightText: 2026 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

#ifndef MARKDOWNLOGIC_H
#define MARKDOWNLOGIC_H

#include <QString>
#include <QDir>
#include <QUrl>
#include <QRegularExpression>

//
// MarkdownLogic —— Markdown 识别（纯函数，无 QWidget 依赖，可 100% 单测覆盖）
//
// 识别规则（与 KSyntaxHighlighting definition 体系对齐）：
//   1. definition 名为 "Markdown"（精确匹配，大小写敏感）→ 是
//   2. 文件扩展名 .md/.markdown/.mdown（不区分大小写）→ 是
//   3. 综合：definition 优先，其次扩展名
//
// §4.4 模式判定使用 isMarkdown() 的结果决定 ReadView/LivePreview 是否可用。
//
class MarkdownLogic
{
public:
    // 按 KSyntaxHighlighting definition 名判定（dtextedit.cpp:618 同类用法）
    static bool isMarkdownByDefinitionName(const QString &definitionName)
    {
        return definitionName == QStringLiteral("Markdown");
    }

    // 按文件扩展名判定（.md/.markdown/.mdown，不区分大小写）
    static bool isMarkdownByFileName(const QString &fileName)
    {
        if (fileName.isEmpty()) return false;
        const auto lower = fileName.toLower();
        return lower.endsWith(QStringLiteral(".md"))
            || lower.endsWith(QStringLiteral(".markdown"))
            || lower.endsWith(QStringLiteral(".mdown"));
    }

    // 综合：definition 优先，其次扩展名
    static bool isMarkdown(const QString &fileName, const QString &definitionName)
    {
        if (isMarkdownByDefinitionName(definitionName)) return true;
        return isMarkdownByFileName(fileName);
    }

    // 图片路径改写（供渲染）：渲染页从 qrc:/ 加载，Chromium 硬禁 qrc 页面访问 file://
    // 子资源（Not allowed to load local resource），相对路径也会被错误解析到 qrc 内。
    // 推送渲染前把非网络图片路径按 md 文件所在目录改写为 mdimg:// 绝对 URL
    // （自定义 scheme + QWebEngineUrlSchemeHandler 在 Qt 侧供给本地文件内容）。
    // 纯函数：md 为源文本，baseDir 为 md 文件绝对目录；http/https/data/mdimg 原样保留。
    static QString resolveImagePaths(const QString &md, const QString &baseDir)
    {
        if (md.isEmpty() || baseDir.isEmpty())
            return md;

        static const QRegularExpression rx(
            QStringLiteral("!\\[([^\\]]*)\\]\\(([^)]+?)(\\s+\"[^\"]*\")?\\)"));
        QString result;
        qsizetype last = 0;
        auto it = rx.globalMatch(md);
        while (it.hasNext()) {
            const auto m = it.next();
            result += QStringView(md).mid(last, m.capturedStart() - last);
            const QString path = m.captured(2);
            const QString title = m.captured(3);   // 可选 "title"，重建时原样保留
            QString resolved = path;
            if (!path.startsWith(QStringLiteral("http:"), Qt::CaseInsensitive)
                    && !path.startsWith(QStringLiteral("https:"), Qt::CaseInsensitive)
                    && !path.startsWith(QStringLiteral("data:"), Qt::CaseInsensitive)
                    && !path.startsWith(QStringLiteral("mdimg:"), Qt::CaseInsensitive)) {
                const QString abs = QDir(baseDir).absoluteFilePath(QString(path).trimmed());
                // 手工构造 mdimg:///<编码路径>（abs 自带首斜杠，拼出恰三个斜杠）：不依赖 QUrl 对自定义 scheme 的 authority 推断
                resolved = QStringLiteral("mdimg://")
                           + QString::fromUtf8(QUrl::toPercentEncoding(abs, "/"));
            }
            result += QStringLiteral("![") + m.captured(1) + QStringLiteral("](")
                      + resolved + title + QStringLiteral(")");
            last = m.capturedEnd();
        }
        result += QStringView(md).mid(last);
        return result;
    }
};

#endif // MARKDOWNLOGIC_H
