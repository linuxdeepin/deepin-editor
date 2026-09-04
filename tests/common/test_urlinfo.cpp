// SPDX-FileCopyrightText: 2026 UnionTech Software Technology Co., Ltd.
// SPDX-License-Identifier: GPL-3.0-or-later

#include <gtest/gtest.h>
#include "stubext.h"

#include <QCoreApplication>
#include <QTemporaryDir>
#include <QTemporaryFile>
#include <QFile>
#include <QFileInfo>
#include <QDir>
#include <QUrl>

#include "urlinfo.h"

// ---------------------------------------------------------------------------
// 分支清单（来源：src/common/urlinfo.h UrlInfo::UrlInfo(QString)）
// U1: QFile::exists(path) == true  → url = fromLocalFile(canonicalFilePath)，提前返回
// U2: 行列号后缀匹配 :(\d+)(:(\d+))?：?$ → path 截去后缀再解析（含 :12 / :12:34 两种）
// U3: 无后缀 → 直接 fromUserInput(path, currentPath, AssumeLocalFile)
// U4: url 无效 → 兜底 fromLocalFile(Utils::getFilePath(path))
//
// 用例映射：
// - Construct_ExistingFile_UsesCanonicalLocalFileUrl                → U1
// - Construct_ExistingFileWithLineSuffix_StillUsesCanonicalFileUrl  → U1（文件存在优先于行列号）
// - Construct_PathWithLineSuffix_StripsSuffixBeforeParsing          → U2（:12）
// - Construct_PathWithLineColumnSuffix_StripsWholeSuffix            → U2（:12:34）
// - Construct_RelativePath_ResolvesAgainstWorkingDirectory          → U3
// - Construct_NonexistentAbsolutePath_KeepsAbsolutePath             → U3
// - Construct_SuffixVariants_UrlAlwaysPointsToBasePath /* TEST_P */ → U2/U3 边界组
//
// 环境隔离：全部路径来自 QTemporaryDir/QTemporaryFile，不触碰真实用户目录。
// ---------------------------------------------------------------------------

class UrlInfoTest : public ::testing::Test {
protected:
    static void SetUpTestSuite()
    {
        int argc = 1;
        static char appArg0[] = "test_urlinfo";
        static char *argv[] = { appArg0, nullptr };
        s_app = new QCoreApplication(argc, argv);
    }

    void SetUp() override
    {
        ASSERT_TRUE(m_tmpDir.isValid());
        m_base = m_tmpDir.filePath("notes.txt");
    }

    QTemporaryDir m_tmpDir;
    QString m_base;
    static QCoreApplication *s_app;
};

QCoreApplication *UrlInfoTest::s_app = nullptr;

TEST_F(UrlInfoTest, Construct_ExistingFile_UsesCanonicalLocalFileUrl)
{
    // Arrange
    QFile f(m_base);
    ASSERT_TRUE(f.open(QIODevice::WriteOnly | QIODevice::Text));
    f.write("hello");
    f.close();
    const QString canonical = QFileInfo(m_base).canonicalFilePath();

    // Act
    UrlInfo info(m_base);

    // Assert
    EXPECT_TRUE(info.url.isValid());
    EXPECT_EQ(info.url.toLocalFile(), canonical);
    EXPECT_TRUE(info.url.isLocalFile());
}

TEST_F(UrlInfoTest, Construct_ExistingFileWithLineSuffix_StillUsesCanonicalFileUrl)
{
    // Arrange：文件真实存在，且名字本身带 ":12"（文件存在性优先）
    const QString pathWithSuffix = m_base + ":12";
    QFile f(pathWithSuffix);
    ASSERT_TRUE(f.open(QIODevice::WriteOnly));
    f.write("x");
    f.close();
    const QString canonical = QFileInfo(pathWithSuffix).canonicalFilePath();

    // Act
    UrlInfo info(pathWithSuffix);

    // Assert：按存在文件处理，不做行列号截断
    EXPECT_EQ(info.url.toLocalFile(), canonical);
    EXPECT_TRUE(info.url.isLocalFile());
}

TEST_F(UrlInfoTest, Construct_PathWithLineSuffix_StripsSuffixBeforeParsing)
{
    // Arrange：文件不存在，路径带 :12 行号
    const QString input = m_base + ":12";

    // Act
    UrlInfo info(input);

    // Assert：行列号被截去，URL 指向纯路径且有效
    EXPECT_EQ(info.url.toLocalFile(), m_base);
    EXPECT_TRUE(info.url.isValid());
}

TEST_F(UrlInfoTest, Construct_PathWithLineColumnSuffix_StripsWholeSuffix)
{
    // Arrange：文件不存在，路径带 :12:34（行+列）
    const QString input = m_base + ":12:34";

    // Act
    UrlInfo info(input);

    // Assert：整段行列后缀被截去，仍为本地文件 URL
    EXPECT_EQ(info.url.toLocalFile(), m_base);
    EXPECT_TRUE(info.url.isLocalFile());
}

TEST_F(UrlInfoTest, Construct_RelativePath_ResolvesAgainstWorkingDirectory)
{
    // Arrange：相对路径（不存在）
    const QString rel = QString("ut-urlinfo-%1.txt").arg(QCoreApplication::applicationPid());

    // Act
    UrlInfo info(rel);

    // Assert：以当前工作目录为基准展开为绝对本地路径
    const QString expected = QDir(QDir::currentPath()).filePath(rel);
    EXPECT_EQ(info.url.toLocalFile(), expected);
    EXPECT_TRUE(QFileInfo(info.url.toLocalFile()).isAbsolute()); // 相对路径被展开为绝对
}

TEST_F(UrlInfoTest, Construct_NonexistentAbsolutePath_KeepsAbsolutePath)
{
    // Arrange：不存在的绝对路径，无后缀
    const QString abs = m_tmpDir.filePath("not-created-yet.cpp");

    // Act
    UrlInfo info(abs);

    // Assert：保留绝对路径（新建文件场景），且该文件确实不存在
    EXPECT_EQ(info.url.toLocalFile(), abs);
    EXPECT_FALSE(QFile::exists(info.url.toLocalFile()));
}

namespace {
struct SuffixCase {
    QString suffix;
    const char *desc;
};
} // namespace

class UrlInfoParamTest : public UrlInfoTest, public ::testing::WithParamInterface<SuffixCase> {
};

TEST_P(UrlInfoParamTest, Construct_SuffixVariants_UrlAlwaysPointsToBasePath)
{
    // Arrange：同一基础路径 + 不同行列后缀变体（含可选冒号结尾边界）
    const QString input = m_base + GetParam().suffix;

    // Act
    UrlInfo info(input);

    // Assert：所有变体最终 URL 都指向去后缀的基础路径
    EXPECT_TRUE(info.url.isValid());
    EXPECT_EQ(info.url.toLocalFile(), m_base) << GetParam().desc;
}

INSTANTIATE_TEST_SUITE_P(
        SuffixBoundary, UrlInfoParamTest,
        ::testing::Values(
                SuffixCase{ QString(":0"), "line zero boundary" },
                SuffixCase{ QString(":1"), "line one" },
                SuffixCase{ QString(":16777215"), "large line number" },
                SuffixCase{ QString(":7:0"), "line and column zero" },
                SuffixCase{ QString(":7:9:"), "trailing colon" },
                SuffixCase{ QString(":42:"), "line with trailing colon" }));
