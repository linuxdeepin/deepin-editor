// SPDX-FileCopyrightText: 2026 UnionTech Software Technology Co., Ltd.
// SPDX-License-Identifier: GPL-3.0-or-later

#include <gtest/gtest.h>
#include "stubext.h"
#include "markdownlogic.h"

#include <QDir>
#include <QTemporaryDir>
#include <QUrl>

// 分支清单（来源：markdownlogic.h）
// isMarkdownByDefinitionName:
//   D1: definitionName == "Markdown"（大小写敏感精确）→ true
//   D2: 其他（小写/空/带后缀）→ false
// isMarkdownByFileName:
//   F1: fileName.isEmpty() → false（提前 return）
//   F2: lower endsWith .md/.markdown/.mdown → true
//   F3: 其他扩展 → false
// isMarkdown:
//   M1: definition 命中 → true（优先）
//   M2: definition 未命中但扩展命中 → true
//   M3: 两者都未命中 → false
// resolveImagePaths:
//   R1: md.isEmpty() || baseDir.isEmpty() → 原样返回（提前 return）
//   R2: path 为 http:/https:/data:/mdimg: → 原样保留（4 个 || 短路条件）
//   R3: 本地相对/绝对路径 → 改写 mdimg://<percent-encode(abs)>
//   R4: 带 title 的图片语法 → title 原样保留
//   R5: 无图片 → 文本原样返回（循环 0 次）
//   R6: 多图片/中间文本 → 拼接正确（循环 N 次 + last 尾段）
//   R7: 路径带空白 → QString(path).trimmed()
//
// 用例映射：
// - IsMarkdownByDefinitionName_ExactAndCaseVariants_ReturnsExpected（TEST_P 6 组）  → D1/D2
// - IsMarkdownByFileName_ExtensionVariants_ReturnsExpected（TEST_P 8 组）           → F1/F2/F3
// - IsMarkdown_DefinitionPriorityOverExtension_ReturnsExpected（TEST_P 5 组）        → M1/M2/M3
// - ResolveImagePaths_EmptyMdOrBaseDir_ReturnsInputUnchanged（TEST_P 3 组）          → R1
// - ResolveImagePaths_NetworkAndDataSchemes_PreservedAsIs（TEST_P 4 组）             → R2
// - ResolveImagePaths_RelativeLocalPath_RewrittenToMdimgUrl                          → R3
// - ResolveImagePaths_AbsoluteLocalPath_RewrittenToMdimgUrl                           → R3
// - ResolveImagePaths_TitleSuffix_PreservedInRebuild                                 → R4
// - ResolveImagePaths_NoImageSyntax_TextUnchanged                                    → R5
// - ResolveImagePaths_MultipleImagesWithText_InterleavedCorrectly                    → R6
// - ResolveImagePaths_PathWithSurroundingSpaces_TrimmedBeforeResolve                 → R7
//
// 最小清单自检：1 每公开方法≥1用例 ✔（4 个静态方法全覆盖，private 无）
// 2 等价类划分 ✔ 3 边界（空串/大小写/多图/尾段）✔ 4 TEST_P ×4（各≥3 组）✔
// 5 分支清单映射 ✔ 6 全部分支有用例 ✔ 7 无 throw 8 空输入负面 ✔ 9 纯函数无状态损坏 ✔
// 10 Qt 静态工具类直接调用 + QTemporaryDir 隔离文件系统 ✔（无 stub 目标）

// —— 等价类/边界成组：definition 名 ——
namespace {
struct NameCase {
    QString name;
    bool expected;
};
const NameCase kDefCases[] = {
    { QStringLiteral("Markdown"), true },            // 唯一有效等价类
    { QStringLiteral("markdown"), false },           // 大小写敏感：小写拒绝
    { QStringLiteral("MARKDOWN"), false },           // 大写拒绝
    { QStringLiteral("Markdown Extra"), false },      // 非精确匹配
    { QStringLiteral(""), false },                    // 空串边界
    { QStringLiteral("Markdowns"), false },           // 前缀+后缀近似
};
} // namespace

class DefNameParamTest : public ::testing::TestWithParam<NameCase> {
};

TEST_P(DefNameParamTest, IsMarkdownByDefinitionName_ExactAndCaseVariants_ReturnsExpected)
{
    // Arrange
    const auto &c = GetParam();

    // Act
    const bool ret = MarkdownLogic::isMarkdownByDefinitionName(c.name);

    // Assert
    EXPECT_EQ(ret, c.expected);   // 期望值见参数表注释
    EXPECT_EQ(ret, c.name == QStringLiteral("Markdown"));
}

INSTANTIATE_TEST_SUITE_P(DefinitionNames, DefNameParamTest,
                         ::testing::ValuesIn(kDefCases));

// —— 等价类/边界成组：文件名扩展 ——
namespace {
const NameCase kFileCases[] = {
    { QStringLiteral("README.md"), true },
    { QStringLiteral("note.MARKDOWN"), true },        // 扩展名不区分大小写
    { QStringLiteral("a.MdOwN"), true },
    { QStringLiteral("b.mdown"), true },
    { QStringLiteral("c.txt"), false },
    { QStringLiteral("d.md5"), false },               // 近似扩展非 .md
    { QStringLiteral("md"), false },                   // 无点边界
    { QStringLiteral(""), false },                     // 空串提前 return
};
} // namespace

class FileNameParamTest : public ::testing::TestWithParam<NameCase> {
};

TEST_P(FileNameParamTest, IsMarkdownByFileName_ExtensionVariants_ReturnsExpected)
{
    // Arrange
    const auto &c = GetParam();

    // Act
    const bool ret = MarkdownLogic::isMarkdownByFileName(c.name);

    // Assert
    EXPECT_EQ(ret, c.expected);
    // 与底层 toLower+endsWith 行为一致性（第二条独立断言）
    const QString lower = c.name.toLower();
    EXPECT_EQ(ret, lower.endsWith(".md") || lower.endsWith(".markdown") || lower.endsWith(".mdown"));
}

INSTANTIATE_TEST_SUITE_P(FileNames, FileNameParamTest,
                         ::testing::ValuesIn(kFileCases));

// —— 综合：definition 优先于扩展 ——
namespace {
struct ComboCase {
    QString fileName;
    QString definitionName;
    bool expected;
};
const ComboCase kComboCases[] = {
    { QStringLiteral("x.txt"), QStringLiteral("Markdown"), true },    // definition 优先
    { QStringLiteral("x.md"), QStringLiteral("C++"), true },          // 扩展兜底
    { QStringLiteral("x.txt"), QStringLiteral("C++"), false },        // 双未命中
    { QStringLiteral(""), QStringLiteral(""), false },                 // 双空边界
    { QStringLiteral("y.md"), QStringLiteral("Markdown"), true },      // 双命中
};
} // namespace

class ComboParamTest : public ::testing::TestWithParam<ComboCase> {
};

TEST_P(ComboParamTest, IsMarkdown_DefinitionPriorityOverExtension_ReturnsExpected)
{
    // Arrange
    const auto &c = GetParam();

    // Act
    const bool ret = MarkdownLogic::isMarkdown(c.fileName, c.definitionName);

    // Assert
    EXPECT_EQ(ret, c.expected);
    // definition 命中时与文件名无关（优先级验证）
    if (c.definitionName == QStringLiteral("Markdown"))
        EXPECT_TRUE(ret);
}

INSTANTIATE_TEST_SUITE_P(Combinations, ComboParamTest,
                         ::testing::ValuesIn(kComboCases));

// —— R1：空输入提前返回 ——
namespace {
struct EmptyCase {
    QString md;
    QString baseDir;
};
const EmptyCase kEmptyCases[] = {
    { QString(), QStringLiteral("/any/dir") },   // 空 md
    { QStringLiteral("# t"), QString() },        // 空 baseDir
    { QString(), QString() },                    // 双空边界
};
} // namespace

class EmptyInputParamTest : public ::testing::TestWithParam<EmptyCase> {
};

TEST_P(EmptyInputParamTest, ResolveImagePaths_EmptyMdOrBaseDir_ReturnsInputUnchanged)
{
    // Arrange
    const auto &c = GetParam();

    // Act
    const QString out = MarkdownLogic::resolveImagePaths(c.md, c.baseDir);

    // Assert：提前 return，原样返回（不得产生 mdimg:// 改写）
    EXPECT_EQ(out, c.md);
    EXPECT_FALSE(out.contains(QStringLiteral("mdimg://")));
}

INSTANTIATE_TEST_SUITE_P(EmptyInputs, EmptyInputParamTest,
                         ::testing::ValuesIn(kEmptyCases));

// —— R2：保留 scheme（4 组同质）——
namespace {
struct SchemeCase {
    QString path;
};
const SchemeCase kSchemeCases[] = {
    { QStringLiteral("http://example.org/a.png") },
    { QStringLiteral("https://example.org/b.png") },
    { QStringLiteral("data:image/png;base64,iVBORw0KGgo=") },
    { QStringLiteral("mdimg:///already/resolved.png") },
};
} // namespace

class SchemeParamTest : public ::testing::TestWithParam<SchemeCase> {
};

TEST_P(SchemeParamTest, ResolveImagePaths_NetworkAndDataSchemes_PreservedAsIs)
{
    // Arrange
    const auto &c = GetParam();
    const QString md = QStringLiteral("![alt](") + c.path + QStringLiteral(")");
    QTemporaryDir dir;
    ASSERT_TRUE(dir.isValid());

    // Act
    const QString out = MarkdownLogic::resolveImagePaths(md, dir.path());

    // Assert：原样保留，不重写、不 percent-encode
    EXPECT_EQ(out, md);
    EXPECT_EQ(out.count(QStringLiteral("mdimg://")), c.path.startsWith(QStringLiteral("mdimg:")) ? 1 : 0);
}

INSTANTIATE_TEST_SUITE_P(PreservedSchemes, SchemeParamTest,
                         ::testing::ValuesIn(kSchemeCases));

TEST(MarkdownLogicTest, ResolveImagePaths_RelativeLocalPath_RewrittenToMdimgUrl)
{
    // Arrange
    QTemporaryDir dir;
    ASSERT_TRUE(dir.isValid());
    const QString md = QStringLiteral("![logo](img/logo.png)");
    const QString abs = QDir(dir.path()).absoluteFilePath(QStringLiteral("img/logo.png"));

    // Act
    const QString out = MarkdownLogic::resolveImagePaths(md, dir.path());

    // Assert：改写为 mdimg:// + percent-encoding(abs)（'/' 保留，恰三个斜杠）
    const QString expected = QStringLiteral("mdimg://")
                             + QString::fromUtf8(QUrl::toPercentEncoding(abs, "/"));
    EXPECT_EQ(out, QStringLiteral("![logo](") + expected + QStringLiteral(")"));
    EXPECT_TRUE(out.startsWith(QStringLiteral("![logo](mdimg:///")));
    EXPECT_TRUE(out.contains(QStringLiteral("img/logo.png")));
}

TEST(MarkdownLogicTest, ResolveImagePaths_AbsoluteLocalPath_RewrittenToMdimgUrl)
{
    // Arrange
    QTemporaryDir dir;
    ASSERT_TRUE(dir.isValid());
    const QString abs = dir.filePath(QStringLiteral("pic.png"));   // 绝对路径输入
    const QString md = QStringLiteral("![](") + abs + QStringLiteral(")");

    // Act
    const QString out = MarkdownLogic::resolveImagePaths(md, dir.path() + QStringLiteral("/nested"));

    // Assert：absoluteFilePath 对绝对入参原样返回，仍改写为 mdimg
    const QString expected = QStringLiteral("mdimg://")
                             + QString::fromUtf8(QUrl::toPercentEncoding(abs, "/"));
    EXPECT_EQ(out, QStringLiteral("![](") + expected + QStringLiteral(")"));
    EXPECT_TRUE(out.contains(QStringLiteral("mdimg:///")));
}

TEST(MarkdownLogicTest, ResolveImagePaths_TitleSuffix_PreservedInRebuild)
{
    // Arrange
    QTemporaryDir dir;
    ASSERT_TRUE(dir.isValid());
    const QString md = QStringLiteral("![cap]( shot.png \"截图\")");

    // Act
    const QString out = MarkdownLogic::resolveImagePaths(md, dir.path());

    // Assert：路径 trimmed + title（含引号与空格）原样保留
    const QString abs = QDir(dir.path()).absoluteFilePath(QStringLiteral("shot.png"));
    const QString expected = QStringLiteral("mdimg://")
                             + QString::fromUtf8(QUrl::toPercentEncoding(abs, "/"));
    EXPECT_EQ(out, QStringLiteral("![cap](") + expected + QStringLiteral(" \"截图\")"));
    EXPECT_TRUE(out.contains(QStringLiteral("\"截图\"")));
}

TEST(MarkdownLogicTest, ResolveImagePaths_NoImageSyntax_TextUnchanged)
{
    // Arrange
    const QString md = QString::fromUtf8("# 标题\n[链接](http://example.org)\n正文无图");
    QTemporaryDir dir;
    ASSERT_TRUE(dir.isValid());

    // Act
    const QString out = MarkdownLogic::resolveImagePaths(md, dir.path());

    // Assert：循环 0 次，全文原样（普通链接不误伤）
    EXPECT_EQ(out, md);
    EXPECT_FALSE(out.contains(QStringLiteral("mdimg")));
}

TEST(MarkdownLogicTest, ResolveImagePaths_MultipleImagesWithText_InterleavedCorrectly)
{
    // Arrange
    QTemporaryDir dir;
    ASSERT_TRUE(dir.isValid());
    const QString md = QStringLiteral("pre ![a](a.png) mid ![b](sub/b.png) tail");

    // Act
    const QString out = MarkdownLogic::resolveImagePaths(md, dir.path());

    // Assert：两图均改写，中间/首尾文本保留（尾段 last 拼接）
    const QString absA = QDir(dir.path()).absoluteFilePath(QStringLiteral("a.png"));
    const QString absB = QDir(dir.path()).absoluteFilePath(QStringLiteral("sub/b.png"));
    const QString urlA = QString::fromUtf8(QUrl::toPercentEncoding(absA, "/"));
    const QString urlB = QString::fromUtf8(QUrl::toPercentEncoding(absB, "/"));
    EXPECT_EQ(out, QStringLiteral("pre ![a](mdimg://") + urlA
                    + QStringLiteral(") mid ![b](mdimg://") + urlB
                    + QStringLiteral(") tail"));
    EXPECT_EQ(out.count(QStringLiteral("mdimg://")), 2);
}

TEST(MarkdownLogicTest, ResolveImagePaths_PathWithSurroundingSpaces_TrimmedBeforeResolve)
{
    // Arrange
    QTemporaryDir dir;
    ASSERT_TRUE(dir.isValid());
    const QString md = QStringLiteral("![x](   space.png   )");

    // Act
    const QString out = MarkdownLogic::resolveImagePaths(md, dir.path());

    // Assert：trimmed 后按 baseDir 解析（URL 不含空格与 %20 以外的空白）
    const QString abs = QDir(dir.path()).absoluteFilePath(QStringLiteral("space.png"));
    const QString expected = QString::fromUtf8(QUrl::toPercentEncoding(abs, "/"));
    EXPECT_EQ(out, QStringLiteral("![x](mdimg://") + expected + QStringLiteral(")"));
    EXPECT_FALSE(out.contains(QStringLiteral(" space")));
}
