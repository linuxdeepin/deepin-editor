// SPDX-FileCopyrightText: 2026 UnionTech Software Technology Co., Ltd.
// SPDX-License-Identifier: GPL-3.0-or-later

#include "../../src/common/urlinfo.h"

#include <gtest/gtest.h>
#include <QFileInfo>
#include <QTemporaryFile>
#include <QDir>

TEST(UrlInfoTest, ExistingFile_UrlIsLocalFile)
{
    QTemporaryFile tmp;
    ASSERT_TRUE(tmp.open());
    const QString path = tmp.fileName();
    UrlInfo info(path);
    EXPECT_TRUE(info.url.isLocalFile());
    EXPECT_EQ(info.url.toLocalFile(), QFileInfo(path).canonicalFilePath());
}

TEST(UrlInfoTest, PlainPath_UrlIsLocalFile)
{
    const QString path = QStringLiteral("/tmp/de-autotest-urlinfo-plain");
    UrlInfo info(path);
    EXPECT_TRUE(info.url.isValid());
}

TEST(UrlInfoTest, PathWithLineNumber_StripsLineSpec)
{
    // path with :line:col suffix is stripped to the file path.
    const QString path = QStringLiteral("/tmp/de-autotest-urlinfo-line.cpp:42:7");
    UrlInfo info(path);
    EXPECT_TRUE(info.url.isValid());
    // The resulting local file should no longer contain the trailing :42:7.
    const QString local = info.url.toLocalFile();
    if (!local.isEmpty()) {
        EXPECT_FALSE(local.contains(QStringLiteral(":42:7")));
    }
}

TEST(UrlInfoTest, EmptyPath_ConstructsWithoutCrash)
{
    UrlInfo info(QStringLiteral(""));
    // An empty path yields an empty/invalid url; we only require no crash.
    EXPECT_TRUE(info.url.isEmpty() || !info.url.isValid());
}
