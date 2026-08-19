// SPDX-FileCopyrightText: 2026 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include "ut_markdownlogic.h"
#include "markdownlogic.h"

UT_MarkdownLogic::UT_MarkdownLogic()
{
}

// definition 名为 "Markdown" → 是 markdown
TEST_F(UT_MarkdownLogic, IsMarkdown_DefinitionNameMarkdown_True)
{
    EXPECT_TRUE(MarkdownLogic::isMarkdownByDefinitionName(QStringLiteral("Markdown")));
}

// definition 名大小写敏感（KSyntaxHighlighting 规范名为 "Markdown"）
TEST_F(UT_MarkdownLogic, IsMarkdown_DefinitionNameLowercase_False)
{
    EXPECT_FALSE(MarkdownLogic::isMarkdownByDefinitionName(QStringLiteral("markdown")));
}

// definition 名为空 → false
TEST_F(UT_MarkdownLogic, IsMarkdown_EmptyDefinitionName_False)
{
    EXPECT_FALSE(MarkdownLogic::isMarkdownByDefinitionName(QStringLiteral("")));
}

// definition 名为其他语言 → false
TEST_F(UT_MarkdownLogic, IsMarkdown_OtherLanguage_False)
{
    EXPECT_FALSE(MarkdownLogic::isMarkdownByDefinitionName(QStringLiteral("C++")));
}

// 扩展名 .md → markdown
TEST_F(UT_MarkdownLogic, IsMarkdown_DotMd_True)
{
    EXPECT_TRUE(MarkdownLogic::isMarkdownByFileName(QStringLiteral("readme.md")));
    EXPECT_TRUE(MarkdownLogic::isMarkdownByFileName(QStringLiteral("README.MD")));
}

// 扩展名 .markdown → markdown
TEST_F(UT_MarkdownLogic, IsMarkdown_DotMarkdown_True)
{
    EXPECT_TRUE(MarkdownLogic::isMarkdownByFileName(QStringLiteral("notes.markdown")));
}

// 扩展名 .mdown → markdown
TEST_F(UT_MarkdownLogic, IsMarkdown_DotMdown_True)
{
    EXPECT_TRUE(MarkdownLogic::isMarkdownByFileName(QStringLiteral("doc.mdown")));
}

// 扩展名 .txt → 不是 markdown
TEST_F(UT_MarkdownLogic, IsMarkdown_DotTxt_False)
{
    EXPECT_FALSE(MarkdownLogic::isMarkdownByFileName(QStringLiteral("notes.txt")));
}

// 扩展名 .cpp → 不是 markdown
TEST_F(UT_MarkdownLogic, IsMarkdown_DotCpp_False)
{
    EXPECT_FALSE(MarkdownLogic::isMarkdownByFileName(QStringLiteral("main.cpp")));
}

// 无扩展名 → false
TEST_F(UT_MarkdownLogic, IsMarkdown_NoExtension_False)
{
    EXPECT_FALSE(MarkdownLogic::isMarkdownByFileName(QStringLiteral("Makefile")));
}

// 综合判定：definition 优先，其次扩展名
TEST_F(UT_MarkdownLogic, IsMarkdown_Combined_DefinitionFirst)
{
    // definition 名为 Markdown，即使文件名无扩展名也判为 true
    EXPECT_TRUE(MarkdownLogic::isMarkdown(QStringLiteral("Makefile"), QStringLiteral("Markdown")));
    // definition 名为空，扩展名 .md → true
    EXPECT_TRUE(MarkdownLogic::isMarkdown(QStringLiteral("x.md"), QStringLiteral("")));
    // 都不匹配 → false
    EXPECT_FALSE(MarkdownLogic::isMarkdown(QStringLiteral("x.txt"), QStringLiteral("Plain Text")));
}

// 新建 .txt 不应识别为 markdown（需求：新建 txt 不触发渲染）
TEST_F(UT_MarkdownLogic, IsMarkdown_NewTxt_False)
{
    EXPECT_FALSE(MarkdownLogic::isMarkdown(QStringLiteral("untitled.txt"), QStringLiteral("Plain Text")));
}

// —— 图片路径解析（渲染页从 qrc:/ 加载，相对路径会错误解析到 qrc，须改写为 file:// 绝对 URL）——

// 相对路径 → baseDir 下的 file:// 绝对 URL
TEST_F(UT_MarkdownLogic, ResolveImagePaths_RelativeToBaseDir)
{
    const QString md = QStringLiteral("![alt](sample.png)");
    const QString out = MarkdownLogic::resolveImagePaths(md, QStringLiteral("/tmp/demo"));
    EXPECT_EQ(out, QStringLiteral("![alt](mdimg:///tmp/demo/sample.png)"));
}

// 绝对本地路径 → file:// URL（含中文/空格需百分号编码）
TEST_F(UT_MarkdownLogic, ResolveImagePaths_AbsoluteLocalPath)
{
    const QString out = MarkdownLogic::resolveImagePaths(
        QStringLiteral("![x](/home/uos/我的 图片/a.png)"), QStringLiteral("/tmp"));
    EXPECT_EQ(out, QStringLiteral("![x](mdimg:///home/uos/%E6%88%91%E7%9A%84%20%E5%9B%BE%E7%89%87/a.png)"));
}

// 网络与 data URI 原样保留
TEST_F(UT_MarkdownLogic, ResolveImagePaths_RemoteAndDataUnchanged)
{
    const QString md = QStringLiteral("![a](https://x.org/i.png) ![b](http://y.org/j.png) ![c](data:image/png;base64,AAAA)");
    EXPECT_EQ(MarkdownLogic::resolveImagePaths(md, QStringLiteral("/tmp")), md);
}

// 非 image 的普通链接不改动；带标题的图片语法保留标题
TEST_F(UT_MarkdownLogic, ResolveImagePaths_NonImageLinkAndTitle)
{
    const QString md = QStringLiteral("[link](sample.png) ![t](pic.png \"title\")");
    const QString out = MarkdownLogic::resolveImagePaths(md, QStringLiteral("/d"));
    EXPECT_EQ(out, QStringLiteral("[link](sample.png) ![t](mdimg:///d/pic.png \"title\")"));
}
