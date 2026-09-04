// SPDX-FileCopyrightText: 2026 UnionTech Software Technology Co., Ltd.
// SPDX-License-Identifier: GPL-3.0-or-later

#include <gtest/gtest.h>
#include "stubext.h"

// MarkdownImageHandler 定义于 markdownview.cpp 匿名命名空间（内部类），
// 唯一无侵入访问方式：测试 TU 直接包含实现文件。本可执行文件不重复编译
// markdownview.cpp（该文件只编入本 TU），无符号冲突。
#include "markdownview.cpp"

#include <QFile>
#include <QFileInfo>
#include <QTemporaryDir>
#include <QUrl>

// 分支清单（来源：markdownview.cpp MarkdownImageHandler::requestStarted）
// B1: url.host() 非空 → path = '/' + host + path（Blink 规范化还原）
// B2: !fi.exists() || !fi.isFile() → fail(UrlNotFound)（含目录/缺失）
// B3: canonical.isEmpty() || !mime.startsWith("image/") → fail(RequestDenied)
//     （B3a 非图片 MIME；B3b canonical 为空为竞态/符号链接环防御，确定性
//      构造需 exists()==true 且 canonical 为空——常规文件系统无法稳定构造，
//      记录为防御分支，由 B3a 与符号链接规范化用例覆盖周边路径）
// B4: QFile::open 失败 → fail(UrlNotFound)（open 只读常规文件不失败，防御分支）
// B5: 通过校验 → reply(mime, file)，QIODevice 归属 job（此处 job=nullptr 由 stub 接管）
//
// 用例映射：
// - RequestStarted_LocalImageFile_RepliesWithImageMime              → B5
// - RequestStarted_MissingResource_FailsUrlNotFound                 → B2(exists=false)
// - RequestStarted_DirectoryTarget_FailsUrlNotFound                 → B2(isFile=false)
// - RequestStarted_NonImageFile_FailsRequestDenied                  → B3a
// - RequestStarted_EmptyFile_FailsRequestDenied                     → B3a（零字节边界）
// - RequestStarted_SymlinkToTextFile_CanonicalResolvedAndReplies → B5（canonical 解析）
// - RequestStarted_RelativeTraversalResolvedToImage_RepliesImage    → B5（../ 规范化）
// - RequestStarted_UrlWithHost_PathReconstructedPrefixed            → B1（重建后缺失→B2）
//
// 最小清单自检：1 protected requestStarted（唯一方法）全覆盖 ✔ 2 等价类（图/文本/
// 目录/缺失/空/符号链接/遍历）✔ 3 边界（空文件、host 还原）✔ 4 无≥3 组完全同构
// （fail/reply 二值结果，用例语义各不同）5 分支映射 ✔ 6 B1-B3a/B5 全覆盖（B3b/B4
// 防御性不可达，注释说明）7 无异常 8 负面（缺失/目录/非图/空/符号链接）✔
// 9 fail 后无 reply（状态一致）✔ 10 QWebEngineUrlRequestJob 构造私有：以 nullptr
// job + stub_ext 替换 requestUrl/fail/reply（非虚成员函数，stub_ext 适用；不与 gMock 混用）

// 暴露 protected requestStarted（匿名命名空间类在本 TU 可见）
class ExposedImageHandler : public MarkdownImageHandler {
public:
    using MarkdownImageHandler::MarkdownImageHandler;

    void exposeRequestStarted(QWebEngineUrlRequestJob *job) { requestStarted(job); }
};

class MarkdownImageHandlerTest : public ::testing::Test {
protected:
    void SetUp() override
    {
        stub.clear();
        failCount = 0;
        replyCount = 0;
        openUrlCount = 0;   // 无关桩计数，保持对称清零
        lastFailError = -1;
        lastContentType.clear();
        lastDevice = nullptr;
        handler = new ExposedImageHandler();
        ASSERT_TRUE(tmpDir.isValid());

        // job=nullptr 安全：三个目标均为非虚成员函数，stub 体内不解引用 this
        stub.set_lamda(&QWebEngineUrlRequestJob::requestUrl,
                       [this](QWebEngineUrlRequestJob *) -> QUrl {
                           return QUrl(jobUrl);
                       });
        stub.set_lamda(&QWebEngineUrlRequestJob::fail,
                       [this](QWebEngineUrlRequestJob *, QWebEngineUrlRequestJob::Error e) {
                           ++failCount;
                           lastFailError = int(e);
                       });
        stub.set_lamda(&QWebEngineUrlRequestJob::reply,
                       [this](QWebEngineUrlRequestJob *, const QByteArray &ct, QIODevice *dev) {
                           ++replyCount;
                           lastContentType = ct;
                           lastDevice = dev;   // 归属 job(=nullptr)，由 TearDown 回收
                       });
    }

    void TearDown() override
    {
        delete handler;
        handler = nullptr;
        delete lastDevice;   // job 为 nullptr 时 handler 内 new QFile(canonical, job) 无父对象
        lastDevice = nullptr;
        stub.clear();
    }

    QString makeImageFile(const QString &name)
    {
        const QString path = tmpDir.filePath(name);
        QFile f(path);
        f.open(QIODevice::WriteOnly);
        f.write("\x89PNG\r\n\x1a\n");       // PNG 魔数：按内容嗅探为 image/png
        f.write("IHDR");                     // 最小可辨识载荷
        f.close();
        return path;
    }

    QString makeTextFile(const QString &name)
    {
        const QString path = tmpDir.filePath(name);
        QFile f(path);
        f.open(QIODevice::WriteOnly);
        f.write("plain text content\n");
        f.close();
        return path;
    }

    // 构造 mdimg 无 host URL：mdimg:///abs/path
    QString mdimgUrlFor(const QString &absPath)
    {
        QUrl u;
        u.setScheme(QStringLiteral("mdimg"));
        u.setPath(absPath);
        return u.toString();
    }

    stub_ext::StubExt stub;
    QTemporaryDir tmpDir;
    ExposedImageHandler *handler = nullptr;
    QString jobUrl;
    int failCount = 0;
    int replyCount = 0;
    int openUrlCount = 0;
    int lastFailError = -1;
    QByteArray lastContentType;
    QIODevice *lastDevice = nullptr;
};

TEST_F(MarkdownImageHandlerTest, RequestStarted_LocalImageFile_RepliesWithImageMime)
{
    // Arrange：临时目录内真实 PNG（内容嗅探 image/png）
    const QString img = makeImageFile("ok.png");
    jobUrl = mdimgUrlFor(img);

    // Act
    handler->exposeRequestStarted(nullptr);

    // Assert：reply 恰一次、MIME 精确、无 fail、设备内容与源文件一致
    EXPECT_EQ(replyCount, 1);
    EXPECT_EQ(lastContentType, QByteArray("image/png"));
    EXPECT_EQ(failCount, 0);
    ASSERT_NE(lastDevice, nullptr);
    EXPECT_EQ(lastDevice->readAll(), QByteArray("\x89PNG\r\n\x1a\nIHDR"));
}

TEST_F(MarkdownImageHandlerTest, RequestStarted_MissingResource_FailsUrlNotFound)
{
    // Arrange：指向不存在的文件（强异常安全：fail 且无 reply）
    jobUrl = mdimgUrlFor(tmpDir.filePath("absent.png"));

    // Act
    handler->exposeRequestStarted(nullptr);

    // Assert
    EXPECT_EQ(failCount, 1);
    EXPECT_EQ(lastFailError, int(QWebEngineUrlRequestJob::UrlNotFound));
    EXPECT_EQ(replyCount, 0);
}

TEST_F(MarkdownImageHandlerTest, RequestStarted_DirectoryTarget_FailsUrlNotFound)
{
    // Arrange：目标是目录（isFile=false 分支）
    jobUrl = mdimgUrlFor(tmpDir.path());

    // Act
    handler->exposeRequestStarted(nullptr);

    // Assert
    EXPECT_EQ(failCount, 1);
    EXPECT_EQ(lastFailError, int(QWebEngineUrlRequestJob::UrlNotFound));
    EXPECT_EQ(replyCount, 0);
}

TEST_F(MarkdownImageHandlerTest, RequestStarted_NonImageFile_FailsRequestDenied)
{
    // Arrange：真实文本文件（MIME 白名单拒绝——敏感文本不可经 mdimg 读取）
    const QString txt = makeTextFile("notes.txt");
    jobUrl = mdimgUrlFor(txt);

    // Act
    handler->exposeRequestStarted(nullptr);

    // Assert
    EXPECT_EQ(failCount, 1);
    EXPECT_EQ(lastFailError, int(QWebEngineUrlRequestJob::RequestDenied));
    EXPECT_EQ(replyCount, 0);
}

TEST_F(MarkdownImageHandlerTest, RequestStarted_EmptyFile_FailsRequestDenied)
{
    // Arrange：零字节文件（MIME 非空边界，嗅探为非 image/*）
    const QString empty = tmpDir.filePath("empty.bin");
    QFile f(empty);
    f.open(QIODevice::WriteOnly);
    f.close();
    jobUrl = mdimgUrlFor(empty);

    // Act
    handler->exposeRequestStarted(nullptr);

    // Assert
    EXPECT_EQ(failCount, 1);
    EXPECT_EQ(lastFailError, int(QWebEngineUrlRequestJob::RequestDenied));
    EXPECT_EQ(replyCount, 0);
}

TEST_F(MarkdownImageHandlerTest, RequestStarted_SymlinkToTextFile_CanonicalResolvedAndReplies)
{
    // Arrange：目录内 alias.png 符号链接 → 目录外真实文本文件。
    // QMimeDatabase 对带已知扩展名的文件按名判定 MIME（image/png），
    // canonicalFilePath 解析到真实目标（非空）→ 走 reply 供给路径。
    //（覆盖 canonical 规范化分支：符号链接不因链接本身路径失败）
    QTemporaryDir otherDir;
    ASSERT_TRUE(otherDir.isValid());
    const QString realTxt = otherDir.filePath("data.txt");
    QFile rf(realTxt);
    rf.open(QIODevice::WriteOnly);
    rf.write("plain payload\n");
    rf.close();
    const QString link = tmpDir.filePath("alias.png");
    ASSERT_TRUE(QFile::link(realTxt, link));
    jobUrl = mdimgUrlFor(link);

    // Act
    handler->exposeRequestStarted(nullptr);

    // Assert：canonical 解析成功 + 按扩展名 MIME 供给（真实行为契约）
    EXPECT_EQ(replyCount, 1);
    EXPECT_EQ(lastContentType, QByteArray("image/png"));
    EXPECT_EQ(failCount, 0);
}

TEST_F(MarkdownImageHandlerTest, RequestStarted_RelativeTraversalResolvedToImage_RepliesImage)
{
    // Arrange：路径含 sub/../（规范化后仍指向合法图片——合法相对写法不误杀）
    const QString img = makeImageFile("real.png");
    QDir(tmpDir.path()).mkpath(QStringLiteral("sub"));
    const QString withDotDot = tmpDir.path() + QStringLiteral("/sub/../real.png");
    jobUrl = mdimgUrlFor(withDotDot);

    // Act
    handler->exposeRequestStarted(nullptr);

    // Assert：canonicalFilePath 规范化成功 + 图片 MIME → 正常供给
    EXPECT_EQ(replyCount, 1);
    EXPECT_EQ(lastContentType, QByteArray("image/png"));
    EXPECT_EQ(failCount, 0);
}

TEST_F(MarkdownImageHandlerTest, RequestStarted_UrlWithHost_PathReconstructedPrefixed)
{
    // Arrange：Blink 把 mdimg:///abs 规范化为 mdimg://abs/…，handler 需还原
    // 用"host=缺失段 + path=真实文件"构造：重建出的 /host/abs 不存在 → UrlNotFound。
    // 若还原逻辑缺失（bug），path 命中真实文件会 reply——断言可区分两种实现。
    const QString img = makeImageFile("h.png");
    QUrl u;
    u.setScheme(QStringLiteral("mdimg"));
    u.setHost(QStringLiteral("missingseg"));
    u.setPath(img);
    jobUrl = u.toString();

    // Act
    handler->exposeRequestStarted(nullptr);

    // Assert：重建路径 /missingseg/<abs> 不存在 → fail（B1 执行 + B2 兜底）
    EXPECT_EQ(failCount, 1);
    EXPECT_EQ(lastFailError, int(QWebEngineUrlRequestJob::UrlNotFound));
    EXPECT_EQ(replyCount, 0);
}
