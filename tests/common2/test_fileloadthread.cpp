// SPDX-FileCopyrightText: 2026 UnionTech Software Technology Co., Ltd.
// SPDX-License-Identifier: GPL-3.0-or-later

/*
 * FileLoadThread 单元测试
 *
 * 分支清单（来源：FileLoadThread::run / setEncodeHint）
 * B1 : 文件打开失败                       → sigLoadFinished("", "", error=true, hasNul=false)
 * B2 : 小文件 + 无 hint + 检出 UTF-8      → sigLoadFinished(encode, 原始内容, false, false)
 * B3 : 内容含 NUL 字节                    → \x00 转义 "\\00"，hasNul=true
 * B4 : 空文件(file.size()==0)             → readAll 分支，内容为空
 * B5 : encodeHint=GB18030                 → 全量转码为 UTF-8 后发射
 * B6 : encodeHint=UTF-8                   → 不转码，encode=hint
 * B7 : 大文件(>40MB) ASCII/UTF-8 头       → 先发 sigPreProcess(1MB 头)，再发全量 sigLoadFinished
 * B8 : 大文件头含 NUL                     → 头部 \x00 转义后发射 sigPreProcess
 * B9 : 大文件 GB18030 hint                → 头部先转码 UTF-8 再发射 sigPreProcess
 * B10: start() 线程方式运行               → 跨线程信号正常送达
 *
 * 用例映射：
 * - Run_FileMissing_EmitsLoadFinishedError            → B1
 * - Run_SmallUtf8File_EmitsRawContent                 → B2
 * - Run_NulBytes_EscapesContentAndFlagsHasNul         → B3
 * - Run_EmptyFile_EmitsEmptyContent                   → B4
 * - Run_EncodeHintGb18030_ConvertsContentToUtf8       → B5
 * - Run_EncodeHintUtf8_NoConversion                   → B6
 * - Run_LargeUtf8HintFile_EmitsPreProcessHead         → B7
 * - Run_LargeFileNulInHead_EscapesHeadNulBytes        → B8
 * - Run_LargeGb18030HintFile_ConvertsHeadToUtf8       → B9
 * - Start_FromWorkerThread_EmitsLoadFinished          → B10
 *
 * 隔离：全部文件经由 QTemporaryDir 创建；编码探测 DetectCode::GetFileEncodingFormat
 * 默认 stub 为固定 "UTF-8"（探测逻辑由 DetectCode 自身套件覆盖），hint 用例不触发探测。
 */

#include <gtest/gtest.h>
#include "stubext.h"

#include "fileloadthread.h"
#include "../encodes/detectcode.h"

#include <QCoreApplication>
#include <QTemporaryDir>
#include <QTemporaryFile>
#include <QFile>
#include <QSignalSpy>
#include <QPointer>

#include <memory>

namespace {

QCoreApplication *ensureApp()
{
    if (!QCoreApplication::instance()) {
        if (qEnvironmentVariableIsEmpty("QT_QPA_PLATFORM"))
            qputenv("QT_QPA_PLATFORM", "offscreen");
        static int argc = 1;
        static char argv0[] = "test_common2";
        static char *argv[2] = { argv0, nullptr };
        static QCoreApplication app(argc, argv);
        return &app;
    }
    return QCoreApplication::instance();
}

} // namespace

class FileLoadThreadTest : public ::testing::Test
{
protected:
    void SetUp() override
    {
        ensureApp();
        stub.clear();
        detectResult = "UTF-8";
        detectCalls = 0;
        tmpDir = std::make_unique<QTemporaryDir>();
        ASSERT_TRUE(tmpDir->isValid());
        // 默认 stub 编码探测，保证本套件对探测结果完全确定
        stub.set_lamda(&DetectCode::GetFileEncodingFormat,
                       [this](QString, QByteArray) -> QByteArray {
                           ++detectCalls;
                           return detectResult;
                       });
    }

    void TearDown() override
    {
        stub.clear();
    }

    QString writeTempFile(const QString &name, const QByteArray &data)
    {
        const QString path = tmpDir->path() + QLatin1Char('/') + name;
        QFile f(path);
        if (!f.open(QIODevice::WriteOnly | QIODevice::Truncate))
            return QString();
        f.write(data);
        f.close();
        return path;
    }

    // 直接调用 run()（public）同步执行，随后派发 deleteLater 的 DeferredDelete
    void runSync(FileLoadThread *t)
    {
        t->run();
        QCoreApplication::processEvents();
        QCoreApplication::sendPostedEvents(nullptr, QEvent::DeferredDelete);
    }

    stub_ext::StubExt stub;
    std::unique_ptr<QTemporaryDir> tmpDir;
    QByteArray detectResult;
    int detectCalls = 0;
};

// B1
TEST_F(FileLoadThreadTest, Run_FileMissing_EmitsLoadFinishedError)
{
    // Arrange
    const QString missing = tmpDir->path() + QStringLiteral("/not-exist.txt");
    FileLoadThread *t = new FileLoadThread(missing);
    QPointer<FileLoadThread> guard(t);
    QSignalSpy spy(t, &FileLoadThread::sigLoadFinished);
    // Act
    runSync(t);
    // Assert
    EXPECT_EQ(spy.count(), 1);
    if (spy.count() == 1) {
        EXPECT_EQ(spy.at(0).at(0).toByteArray(), QByteArrayLiteral(""));
        EXPECT_EQ(spy.at(0).at(1).toByteArray(), QByteArrayLiteral(""));
        EXPECT_TRUE(spy.at(0).at(2).toBool());   // error = true
        EXPECT_FALSE(spy.at(0).at(3).toBool());  // hasNul = false
    }
    EXPECT_EQ(detectCalls, 0); // 打开失败不触发探测
    EXPECT_TRUE(guard.isNull()); // run() 末尾 deleteLater 已被处理
}

// B2
TEST_F(FileLoadThreadTest, Run_SmallUtf8File_EmitsRawContent)
{
    // Arrange
    const QByteArray raw = QStringLiteral("hello 中文 world").toUtf8();
    const QString path = writeTempFile(QStringLiteral("utf8.txt"), raw);
    ASSERT_FALSE(path.isEmpty());
    FileLoadThread *t = new FileLoadThread(path);
    QPointer<FileLoadThread> guard(t);
    QSignalSpy spy(t, &FileLoadThread::sigLoadFinished);
    // Act
    runSync(t);
    // Assert: UTF-8 家族不转码，原样回传
    EXPECT_EQ(spy.count(), 1);
    if (spy.count() == 1) {
        EXPECT_EQ(spy.at(0).at(0).toByteArray(), QByteArrayLiteral("UTF-8"));
        EXPECT_EQ(spy.at(0).at(1).toByteArray(), raw);
        EXPECT_FALSE(spy.at(0).at(2).toBool());
        EXPECT_FALSE(spy.at(0).at(3).toBool());
    }
    EXPECT_EQ(detectCalls, 1);
    EXPECT_TRUE(guard.isNull());
}

// B3
TEST_F(FileLoadThreadTest, Run_NulBytes_EscapesContentAndFlagsHasNul)
{
    // Arrange
    const QByteArray raw = QByteArrayLiteral("a\0b");
    const QString path = writeTempFile(QStringLiteral("nul.txt"), raw);
    ASSERT_FALSE(path.isEmpty());
    FileLoadThread *t = new FileLoadThread(path);
    QPointer<FileLoadThread> guard(t);
    QSignalSpy spy(t, &FileLoadThread::sigLoadFinished);
    // Act
    runSync(t);
    // Assert: \x00 → "\\00"，hasNul 置位
    EXPECT_EQ(spy.count(), 1);
    if (spy.count() == 1) {
        EXPECT_EQ(spy.at(0).at(1).toByteArray(), QByteArrayLiteral("a\\00b"));
        EXPECT_FALSE(spy.at(0).at(2).toBool());
        EXPECT_TRUE(spy.at(0).at(3).toBool());
    }
    EXPECT_TRUE(guard.isNull());
}

// B4
TEST_F(FileLoadThreadTest, Run_EmptyFile_EmitsEmptyContent)
{
    // Arrange: 0 字节文件触发 file.size()==0 的 readAll 分支
    const QString path = writeTempFile(QStringLiteral("empty.txt"), QByteArray());
    ASSERT_FALSE(path.isEmpty());
    FileLoadThread *t = new FileLoadThread(path);
    QPointer<FileLoadThread> guard(t);
    QSignalSpy spy(t, &FileLoadThread::sigLoadFinished);
    // Act
    runSync(t);
    // Assert
    EXPECT_EQ(spy.count(), 1);
    if (spy.count() == 1) {
        EXPECT_EQ(spy.at(0).at(1).toByteArray(), QByteArrayLiteral(""));
        EXPECT_FALSE(spy.at(0).at(2).toBool());
        EXPECT_FALSE(spy.at(0).at(3).toBool());
    }
    EXPECT_EQ(detectCalls, 1);
    EXPECT_TRUE(guard.isNull());
}

// B5
TEST_F(FileLoadThreadTest, Run_EncodeHintGb18030_ConvertsContentToUtf8)
{
    // Arrange: GB18030 编码的 "中文测试"
    const QByteArray gbBytes = QByteArray::fromHex("D6D0CEC4B2E2CAD4");
    const QString path = writeTempFile(QStringLiteral("gb.txt"), gbBytes);
    ASSERT_FALSE(path.isEmpty());
    FileLoadThread *t = new FileLoadThread(path);
    QPointer<FileLoadThread> guard(t);
    t->setEncodeHint("GB18030");
    QSignalSpy spy(t, &FileLoadThread::sigLoadFinished);
    // Act
    runSync(t);
    // Assert: 内容转码为 UTF-8，encode 采用 hint，不触发自动探测
    EXPECT_EQ(spy.count(), 1);
    if (spy.count() == 1) {
        EXPECT_EQ(spy.at(0).at(0).toByteArray(), QByteArrayLiteral("GB18030"));
        EXPECT_EQ(spy.at(0).at(1).toByteArray(), QStringLiteral("中文测试").toUtf8());
        EXPECT_FALSE(spy.at(0).at(2).toBool());
    }
    EXPECT_EQ(detectCalls, 0);
    EXPECT_TRUE(guard.isNull());
}

// B6
TEST_F(FileLoadThreadTest, Run_EncodeHintUtf8_NoConversion)
{
    // Arrange
    const QByteArray raw = QByteArrayLiteral("plain ascii bytes");
    const QString path = writeTempFile(QStringLiteral("hint.txt"), raw);
    ASSERT_FALSE(path.isEmpty());
    FileLoadThread *t = new FileLoadThread(path);
    QPointer<FileLoadThread> guard(t);
    t->setEncodeHint("UTF-8");
    QSignalSpy spy(t, &FileLoadThread::sigLoadFinished);
    // Act
    runSync(t);
    // Assert
    EXPECT_EQ(spy.count(), 1);
    if (spy.count() == 1) {
        EXPECT_EQ(spy.at(0).at(0).toByteArray(), QByteArrayLiteral("UTF-8"));
        EXPECT_EQ(spy.at(0).at(1).toByteArray(), raw);
        EXPECT_FALSE(spy.at(0).at(2).toBool());
    }
    EXPECT_EQ(detectCalls, 0);
    EXPECT_TRUE(guard.isNull());
}

// B7
TEST_F(FileLoadThreadTest, Run_LargeUtf8HintFile_EmitsPreProcessHead)
{
    // Arrange: 41MB 'a'，hint UTF-8 → 先发 1MB 头，再发全量
    const qint64 headLen = 1024 * 1024;
    const qint64 total = 41LL * 1024 * 1024;
    const QString path = tmpDir->path() + QStringLiteral("/big_utf8.bin");
    {
        QFile f(path);
        ASSERT_TRUE(f.open(QIODevice::WriteOnly | QIODevice::Truncate));
        QByteArray chunk(1024 * 1024, 'a');
        for (qint64 written = 0; written < total; written += chunk.size())
            ASSERT_GT(f.write(chunk), 0);
        f.close();
    }
    FileLoadThread *t = new FileLoadThread(path);
    QPointer<FileLoadThread> guard(t);
    t->setEncodeHint("UTF-8");
    QSignalSpy spyPre(t, &FileLoadThread::sigPreProcess);
    QSignalSpy spyFin(t, &FileLoadThread::sigLoadFinished);
    // Act
    runSync(t);
    // Assert
    EXPECT_EQ(spyPre.count(), 1);
    if (spyPre.count() == 1) {
        EXPECT_EQ(spyPre.at(0).at(0).toByteArray(), QByteArrayLiteral("UTF-8"));
        EXPECT_EQ(spyPre.at(0).at(1).toByteArray().size(), int(headLen));
        EXPECT_EQ(spyPre.at(0).at(1).toByteArray().at(0), 'a');
    }
    EXPECT_EQ(spyFin.count(), 1);
    if (spyFin.count() == 1) {
        EXPECT_EQ(spyFin.at(0).at(1).toByteArray().size(), int(total));
        EXPECT_FALSE(spyFin.at(0).at(2).toBool());
        EXPECT_FALSE(spyFin.at(0).at(3).toBool());
    }
    EXPECT_TRUE(guard.isNull());
}

// B8
TEST_F(FileLoadThreadTest, Run_LargeFileNulInHead_EscapesHeadNulBytes)
{
    // Arrange: 头 1MB 内含 2 个 NUL，其余 'a'，总 41MB
    const qint64 total = 41LL * 1024 * 1024;
    const QString path = tmpDir->path() + QStringLiteral("/big_nul.bin");
    {
        QFile f(path);
        ASSERT_TRUE(f.open(QIODevice::WriteOnly | QIODevice::Truncate));
        QByteArray head(1024 * 1024, 'a');
        head[1] = '\0';
        head[3] = '\0';
        ASSERT_GT(f.write(head), 0);
        QByteArray chunk(1024 * 1024, 'a');
        for (qint64 written = head.size(); written < total; written += chunk.size())
            ASSERT_GT(f.write(chunk), 0);
        f.close();
    }
    FileLoadThread *t = new FileLoadThread(path);
    QPointer<FileLoadThread> guard(t);
    t->setEncodeHint("UTF-8");
    QSignalSpy spyPre(t, &FileLoadThread::sigPreProcess);
    QSignalSpy spyFin(t, &FileLoadThread::sigLoadFinished);
    // Act
    runSync(t);
    // Assert: 头部 2 个 NUL 各膨胀 2 字节（"\\00" 三字节）
    EXPECT_EQ(spyPre.count(), 1);
    if (spyPre.count() == 1) {
        const QByteArray headOut = spyPre.at(0).at(1).toByteArray();
        EXPECT_EQ(headOut.size(), int(1024 * 1024 + 2 * 2));
        EXPECT_TRUE(headOut.contains(QByteArrayLiteral("\\00")));
    }
    EXPECT_EQ(spyFin.count(), 1);
    if (spyFin.count() == 1) {
        EXPECT_TRUE(spyFin.at(0).at(3).toBool()); // hasNul
        EXPECT_TRUE(spyFin.at(0).at(1).toByteArray().contains(QByteArrayLiteral("\\00")));
    }
    EXPECT_TRUE(guard.isNull());
}

// B9
TEST_F(FileLoadThreadTest, Run_LargeGb18030HintFile_ConvertsHeadToUtf8)
{
    // Arrange: 41MB 重复 GB18030 "中"(D6D0)，hint GB18030 → 头部转 UTF-8 后发 sigPreProcess
    const qint64 total = 41LL * 1024 * 1024; // 偶数字节，全部为 D6D0 序列
    const QString path = tmpDir->path() + QStringLiteral("/big_gb.bin");
    {
        QFile f(path);
        ASSERT_TRUE(f.open(QIODevice::WriteOnly | QIODevice::Truncate));
        QByteArray mb(2 * 1024 * 1024, '\0');
        for (int i = 0; i < mb.size(); i += 2) {
            mb[i] = char(0xD6);
            mb[i + 1] = char(0xD0);
        }
        for (qint64 written = 0; written < total;) {
            const qint64 n = qMin<qint64>(mb.size(), total - written);
            ASSERT_EQ(f.write(mb.constData(), n), n);
            written += n;
        }
        f.close();
    }
    FileLoadThread *t = new FileLoadThread(path);
    QPointer<FileLoadThread> guard(t);
    t->setEncodeHint("GB18030");
    QSignalSpy spyPre(t, &FileLoadThread::sigPreProcess);
    QSignalSpy spyFin(t, &FileLoadThread::sigLoadFinished);
    // Act
    runSync(t);
    // Assert: 1MB 头 = 524288 个 D6D0 → 转码为 524288 个 E4B8AD
    EXPECT_EQ(spyPre.count(), 1);
    if (spyPre.count() == 1) {
        EXPECT_EQ(spyPre.at(0).at(0).toByteArray(), QByteArrayLiteral("GB18030"));
        const QByteArray headOut = spyPre.at(0).at(1).toByteArray();
        EXPECT_EQ(headOut.size(), int(524288 * 3));
        EXPECT_TRUE(headOut.startsWith(QByteArray::fromHex("E4B8AD")));
    }
    // 全量 41MB → 21504000 * 3 字节 UTF-8
    EXPECT_EQ(spyFin.count(), 1);
    if (spyFin.count() == 1) {
        EXPECT_EQ(spyFin.at(0).at(1).toByteArray().size(), int((total / 2) * 3));
        EXPECT_FALSE(spyFin.at(0).at(2).toBool());
    }
    EXPECT_TRUE(guard.isNull());
}

// B10
TEST_F(FileLoadThreadTest, Start_FromWorkerThread_EmitsLoadFinished)
{
    // Arrange: 通过 start() 在真实线程中运行
    const QByteArray raw = QByteArrayLiteral("threaded content");
    const QString path = writeTempFile(QStringLiteral("thread.txt"), raw);
    ASSERT_FALSE(path.isEmpty());
    FileLoadThread *t = new FileLoadThread(path);
    QPointer<FileLoadThread> guard(t);
    QSignalSpy spy(t, &FileLoadThread::sigLoadFinished);
    // Act
    t->start();
    const bool got = spy.wait(5000);
    // Assert
    EXPECT_TRUE(got);
    if (got) {
        EXPECT_EQ(spy.at(0).at(1).toByteArray(), raw);
        EXPECT_FALSE(spy.at(0).at(2).toBool());
    }
    t->wait(2000);
    QCoreApplication::processEvents();
    QCoreApplication::sendPostedEvents(nullptr, QEvent::DeferredDelete);
    EXPECT_TRUE(guard.isNull()); // deleteLater 已处理
}

// setEncodeHint：未显式给出 getter，经由 B5/B6 的行为断言覆盖（见上方用例）
TEST_F(FileLoadThreadTest, SetEncodeHint_AppliesToBothDetectAndConvertPath)
{
    // Arrange: 同一 hint 作用于"大文件头探测优先"与"整体转码"两条路径已在 B9 覆盖；
    // 此处补充 hint 在小文件下的最终编码取值断言
    const QString path = writeTempFile(QStringLiteral("hint2.txt"), QByteArrayLiteral("xyz"));
    ASSERT_FALSE(path.isEmpty());
    FileLoadThread *t = new FileLoadThread(path);
    QPointer<FileLoadThread> guard(t);
    t->setEncodeHint("GB18030"); // 纯 ASCII 内容转 GB18030 后再转 UTF-8 输出不变
    QSignalSpy spy(t, &FileLoadThread::sigLoadFinished);
    // Act
    runSync(t);
    // Assert
    EXPECT_EQ(spy.count(), 1);
    if (spy.count() == 1) {
        EXPECT_EQ(spy.at(0).at(0).toByteArray(), QByteArrayLiteral("GB18030"));
        EXPECT_EQ(spy.at(0).at(1).toByteArray(), QByteArrayLiteral("xyz"));
    }
    EXPECT_TRUE(guard.isNull());
}
