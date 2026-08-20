// SPDX-FileCopyrightText: 2026 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include "ut_markdownpreview.h"

UT_MarkdownPreview::UT_MarkdownPreview()
{

}

TEST(UT_MarkdownPreview_isSupported, isSupported)
{
#if QT_VERSION >= QT_VERSION_CHECK(6, 5, 0)
    EXPECT_TRUE(MarkdownPreview::isSupported());
#else
    EXPECT_FALSE(MarkdownPreview::isSupported());
#endif
}

TEST(UT_MarkdownPreview_isMarkdownFile, isMarkdownFile)
{
    EXPECT_TRUE(MarkdownPreview::isMarkdownFile("/tmp/test.md"));
    EXPECT_TRUE(MarkdownPreview::isMarkdownFile("/tmp/README.MD"));
    EXPECT_TRUE(MarkdownPreview::isMarkdownFile("/tmp/doc.markdown"));
    EXPECT_FALSE(MarkdownPreview::isMarkdownFile("/tmp/test.txt"));
    EXPECT_FALSE(MarkdownPreview::isMarkdownFile("/tmp/test"));
    EXPECT_FALSE(MarkdownPreview::isMarkdownFile(QString()));
}

TEST(UT_MarkdownPreview_updatePreview, updatePreview)
{
    if (!MarkdownPreview::isSupported()) {
        return;
    }
    MarkdownPreview preview;
    preview.updatePreview(QStringLiteral("# Title\n\n**bold** text\n\n- item1\n- item2"));
    // 渲染结果应包含标题、强调文本与列表内容
    const QString plain = preview.toPlainText();
    EXPECT_TRUE(plain.contains(QStringLiteral("Title")));
    EXPECT_TRUE(plain.contains(QStringLiteral("bold")));
    EXPECT_TRUE(plain.contains(QStringLiteral("item1")));
    EXPECT_TRUE(plain.contains(QStringLiteral("item2")));
}

TEST(UT_MarkdownPreview_setSource, setSourceNoNavigate)
{
    MarkdownPreview preview;
    // 预览区只读展示，doSetSource 不产生跳转
    preview.doSetSource(QUrl(QStringLiteral("https://example.com")));
    EXPECT_TRUE(preview.source().isEmpty());
}
