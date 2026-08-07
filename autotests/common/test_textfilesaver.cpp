// SPDX-FileCopyrightText: 2026 UnionTech Software Technology Co., Ltd.
// SPDX-License-Identifier: GPL-3.0-or-later

#include "../../src/common/text_file_saver.h"

#include <gtest/gtest.h>
#include <QTextDocument>
#include <QTemporaryFile>
#include <QFile>
#include <QFileInfo>

TEST(TextFileSaverTest, Construct_WithDocument_DoesNotCrash)
{
    QTextDocument doc;
    doc.setPlainText(QStringLiteral("hello deepin-editor"));
    TextFileSaver saver(&doc);
    SUCCEED();
}

TEST(TextFileSaverTest, SetFilePath_SetEncoding_SetEndlineFormat_AreCallable)
{
    QTextDocument doc;
    doc.setPlainText(QStringLiteral("content"));
    TextFileSaver saver(&doc);
    saver.setFilePath(QStringLiteral("/tmp/de-autotest-tfs-1.txt"));
    saver.setEncoding("UTF-8");
    saver.setEndlineFormat(false);
    SUCCEED();
}

TEST(TextFileSaverTest, Save_ToValidPath_WritesFile)
{
    QTemporaryFile tmp;
    ASSERT_TRUE(tmp.open());
    const QString path = tmp.fileName();
    tmp.close();

    QTextDocument doc;
    doc.setPlainText(QStringLiteral("deepin-editor autotest content\nsecond line"));
    TextFileSaver saver(&doc);
    saver.setFilePath(path);
    saver.setEncoding("UTF-8");
    saver.setEndlineFormat(false);
    const bool ok = saver.save();
    EXPECT_TRUE(ok) << saver.errorString().toStdString();
    EXPECT_TRUE(QFileInfo::exists(path));
    EXPECT_TRUE(saver.errorString().isEmpty() || !ok);
}

TEST(TextFileSaverTest, Save_WithCRLF_WritesFile)
{
    QTemporaryFile tmp;
    ASSERT_TRUE(tmp.open());
    const QString path = tmp.fileName();
    tmp.close();

    QTextDocument doc;
    doc.setPlainText(QStringLiteral("crlf test"));
    TextFileSaver saver(&doc);
    saver.setFilePath(path);
    saver.setEncoding("UTF-8");
    saver.setEndlineFormat(true);
    EXPECT_TRUE(saver.save()) << saver.errorString().toStdString();
    EXPECT_TRUE(QFileInfo::exists(path));
}

TEST(TextFileSaverTest, SaveAs_ToNewPath_WritesFile)
{
    QTemporaryFile tmp;
    ASSERT_TRUE(tmp.open());
    const QString path = tmp.fileName();
    tmp.close();

    QTextDocument doc;
    doc.setPlainText(QStringLiteral("saveas content"));
    TextFileSaver saver(&doc);
    saver.setEncoding("UTF-8");
    EXPECT_TRUE(saver.saveAs(path)) << saver.errorString().toStdString();
    EXPECT_TRUE(QFileInfo::exists(path));
}

TEST(TextFileSaverTest, ErrorString_InitiallyEmpty)
{
    QTextDocument doc;
    TextFileSaver saver(&doc);
    EXPECT_TRUE(saver.errorString().isEmpty());
}
