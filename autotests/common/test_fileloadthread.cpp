// SPDX-FileCopyrightText: 2026 UnionTech Software Technology Co., Ltd.
// SPDX-License-Identifier: GPL-3.0-or-later

#include "../../src/common/fileloadthread.h"

#include <gtest/gtest.h>
#include <QTemporaryFile>
#include <QFile>
#include <QTextStream>
#include <atomic>

TEST(FileLoadThreadTest, SetEncodeHint_DoesNotCrash)
{
    FileLoadThread t(QStringLiteral("/tmp/de-autotest-flt-noexist"));
    t.setEncodeHint("UTF-8");
    SUCCEED();
}

// Use a direct connection so the flag is set from within the worker thread
// without requiring a QCoreApplication event loop on the main thread.
TEST(FileLoadThreadTest, LoadValidFile_EmitsLoadFinished)
{
    QTemporaryFile tmp;
    ASSERT_TRUE(tmp.open());
    QTextStream out(&tmp);
    out << QStringLiteral("deepin-editor autotest file load content");
    out.flush();
    const QString path = tmp.fileName();
    tmp.setAutoRemove(false);
    tmp.close();

    FileLoadThread loader(path);
    std::atomic<bool> gotFinish{false};
    std::atomic<bool> gotError{true};
    QObject::connect(&loader, &FileLoadThread::sigLoadFinished,
                     &loader,
                     [&gotFinish, &gotError](const QByteArray &, const QByteArray &,
                                             bool error, bool) {
                         gotFinish.store(true);
                         gotError.store(error);
                     },
                     Qt::DirectConnection);
    loader.start();
    loader.wait(10000);  // block until run() returns
    ASSERT_TRUE(loader.isFinished());
    EXPECT_TRUE(gotFinish.load());
    EXPECT_FALSE(gotError.load());
    QFile::remove(path);
}

TEST(FileLoadThreadTest, LoadMissingFile_FinishesWithoutHang)
{
    FileLoadThread loader(QStringLiteral("/no/such/path/de-autotest-missing-xyz"));
    std::atomic<bool> gotFinish{false};
    QObject::connect(&loader, &FileLoadThread::sigLoadFinished,
                     &loader,
                     [&gotFinish](const QByteArray &, const QByteArray &, bool, bool) {
                         gotFinish.store(true);
                     },
                     Qt::DirectConnection);
    loader.start();
    loader.wait(10000);
    ASSERT_TRUE(loader.isFinished());
    // Whether it reports an error or not, it must not hang.
    SUCCEED();
}
