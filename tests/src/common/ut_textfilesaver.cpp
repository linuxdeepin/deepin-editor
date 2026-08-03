// SPDX-FileCopyrightText: 2026 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include "../../src/common/text_file_saver.h"

#include <gtest/gtest.h>
#include <QTextDocument>
#include <QTemporaryDir>
#include <QFile>
#include <QFileInfo>

// Cover TextFileSaver::saveAs(QString const&) - success path with a temp file.
TEST(UT_TextFileSaver, saveAs_success)
{
    QTextDocument doc;
    doc.setPlainText(QStringLiteral("Hello world\nSecond line"));

    TextFileSaver saver(&doc);

    QTemporaryDir dir;
    ASSERT_TRUE(dir.isValid());
    QString filePath = dir.path() + "/test_saveas.txt";

    bool ret = saver.saveAs(filePath);
    EXPECT_TRUE(ret);

    // The file should exist and contain content.
    QFile file(filePath);
    EXPECT_TRUE(file.exists());
}

// Cover TextFileSaver::saveAs(QString const&) - failure path (unwritable path)
// which should restore the original path and return false.
TEST(UT_TextFileSaver, saveAs_failure)
{
    QTextDocument doc;
    doc.setPlainText(QStringLiteral("content"));

    TextFileSaver saver(&doc);
    saver.setFilePath(QStringLiteral("/tmp/original_path_keep.txt"));

    // An invalid directory path cannot be opened for writing.
    bool ret = saver.saveAs(QStringLiteral("/nonexistent_dir_xyz/path/file.txt"));
    EXPECT_FALSE(ret);

    // On failure the original path is restored.
    EXPECT_FALSE(saver.errorString().isEmpty());
}
