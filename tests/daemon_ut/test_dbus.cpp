// SPDX-FileCopyrightText: 2026 UnionTech Software Technology Co., Ltd.
// SPDX-License-Identifier: GPL-3.0-or-later

// DBus（daemon/src/dbus.h）单元测试
//
// 分支清单（来源：DBus::saveFile）：
// B1: PolicyKitHelper::checkAuthorization(...) == true   → 进入保存流程
// B2: !Utils::fileExists(filepath)                        → mkpath(父目录) + QFile(ReadWrite) 尝试创建
// B3: QFile(filepath).open(WriteOnly|Text) 失败            → qDebug + return false
// B4: 打开成功 → QTextStream 写入 text → close → return true
// B5: checkAuthorization(...) == false                    → return false（不触碰文件系统）
//
// 用例映射：
// - Constructor_NoParent_CreatesDetachedObject                             → 构造
// - SaveFile_AuthorizationDenied_ReturnsFalseAndCreatesNothing             → B5
// - SaveFile_AuthorizationDenied_PassesDaemonActionIdAndCurrentPid         → B5（参数透传）
// - SaveFile_NewFileInNonexistentNestedDir_CreatesAndWrites_ReturnsTrue    → B1+B2+B4
// - SaveFile_ExistingFile_OverwritesContent_ReturnsTrue                    → B1+B4（跳过 B2）
// - SaveFile_PathIsDirectory_ReturnsFalseNegativeBranch                    → B1+B2(创建失败忽略)+B3
// - SaveFile_EmptyPath_ReturnsFalseNegativeBranch                          → B1+B2+B3
// - SaveFile_TextPayloads_WrittenContentMatchesInput /* TEST_P */          → B1+B2+B4（输入等价类）
//
// stub 说明（stub_ext，SetUp 设置 / TearDown clear）：
// - PolicyKitHelper::checkAuthorization（真实符号已在 daemon_ut_src 编入，此处运行期
//   地址级替换）→ 受控返回值，并记录 actionId / pid / 调用次数；据此 B5 分支可零文件系统副作用。
// 文件系统隔离：全部路径来自夹具 QTemporaryDir 成员（RAII，TearDown 自动释放）。
// 编译说明：dbus.cpp 的 QTextStream::setCodec 为 Qt6 移除 API，测试构建经预处理宏映射为
// setEncoding(Utf8)（模块 CMakeLists 注释 1），测试统一传 "UTF-8" encoding，语义一致。

#include <gtest/gtest.h>

#include <QByteArray>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QTemporaryDir>

#include "dbus.h"
#include "policykithelper.h"
#include "stubext.h"

namespace {

struct TextPayloadCase {
    QByteArray text;      // 保存内容
    const char *label;    // 输入等价类标识
};

class DBusSaveFilePayloadTest : public ::testing::TestWithParam<TextPayloadCase> {
protected:
    void SetUp() override
    {
        stub.clear();
        installAuthorizationStub(true);
        ASSERT_TRUE(tmpDir.isValid());
        obj = new DBus();
    }

    void TearDown() override
    {
        delete obj;
        stub.clear();
    }

    void installAuthorizationStub(bool result)
    {
        stub.set_lamda(&PolicyKitHelper::checkAuthorization,
                       [this](PolicyKitHelper *, const QString &actionId, qint64 pid) -> bool {
                           __DBG_STUB_INVOKE__
                           ++authCalls;
                           lastActionId = actionId;
                           lastPid = pid;
                           return authResult;
                       });
        authResult = result;
    }

    stub_ext::StubExt stub;
    DBus *obj = nullptr;
    QTemporaryDir tmpDir;
    int authCalls = 0;
    QString lastActionId;
    qint64 lastPid = -1;
    bool authResult = true;
};

// B1+B2+B4：不同等价类文本（空 / 单字符 / ASCII / UTF-8 多字节 / 超长）写入内容一致
TEST_P(DBusSaveFilePayloadTest, SaveFile_TextPayloads_WrittenContentMatchesInput)
{
    const TextPayloadCase &c = GetParam();
    const QString path = tmpDir.path() + "/payload.txt";

    // Act
    bool ret = obj->saveFile(path.toUtf8(), c.text, "UTF-8");

    // Assert
    EXPECT_TRUE(ret) << "payload: " << c.label;               // 期望：保存成功（B4）
    QFile f(path);
    ASSERT_TRUE(f.open(QIODevice::ReadOnly));
    const QByteArray written = f.readAll();
    f.close();
    EXPECT_EQ(written, c.text) << "payload: " << c.label;     // 文件字节与输入一致
    EXPECT_EQ(authCalls, 1);                                  // 鉴权恰好一次（B1）
}

INSTANTIATE_TEST_SUITE_P(TextPayloads, DBusSaveFilePayloadTest,
    ::testing::Values(
        TextPayloadCase{QByteArray(""), "empty"},
        TextPayloadCase{QByteArray("a"), "single char"},
        TextPayloadCase{QByteArray("hello deepin editor"), "ascii"},
        TextPayloadCase{QByteArray("中文内容-éàü"), "utf8 multibyte"},
        TextPayloadCase{QByteArray(64 * 1024, 'x'), "large 64KiB"}));

class DBusTest : public ::testing::Test {
protected:
    void SetUp() override
    {
        stub.clear();
        authCalls = 0;
        lastActionId.clear();
        lastPid = -1;
        authResult = true;
        installAuthorizationStub(true);
        ASSERT_TRUE(tmpDir.isValid());
        obj = new DBus();
    }

    void TearDown() override
    {
        delete obj;
        stub.clear();
    }

    void installAuthorizationStub(bool result)
    {
        stub.set_lamda(&PolicyKitHelper::checkAuthorization,
                       [this](PolicyKitHelper *, const QString &actionId, qint64 pid) -> bool {
                           __DBG_STUB_INVOKE__
                           ++authCalls;
                           lastActionId = actionId;
                           lastPid = pid;
                           return authResult;
                       });
        authResult = result;
    }

    stub_ext::StubExt stub;
    DBus *obj = nullptr;
    QTemporaryDir tmpDir;
    int authCalls = 0;
    QString lastActionId;
    qint64 lastPid = -1;
    bool authResult = true;
};

// 构造：无父对象
TEST_F(DBusTest, Constructor_NoParent_CreatesDetachedObject)
{
    DBus detached;

    EXPECT_EQ(detached.parent(), nullptr);   // 期望：无父对象
    EXPECT_STREQ(detached.metaObject()->className(), "DBus");  // 元对象登记正确
}

// B5：鉴权拒绝 → false 且不创建文件
TEST_F(DBusTest, SaveFile_AuthorizationDenied_ReturnsFalseAndCreatesNothing)
{
    installAuthorizationStub(false);
    const QString path = tmpDir.path() + "/denied.txt";

    // Act
    bool ret = obj->saveFile(path.toUtf8(), QByteArray("data"), "UTF-8");

    // Assert
    EXPECT_FALSE(ret);                              // 期望：拒绝分支返回 false（B5）
    EXPECT_FALSE(QFile::exists(path));              // 未产生任何文件副作用
    EXPECT_EQ(authCalls, 1);                        // 鉴权仅一次
}

// B5：拒绝路径的参数透传（actionId 固定、pid 为当前进程）
TEST_F(DBusTest, SaveFile_AuthorizationDenied_PassesDaemonActionIdAndCurrentPid)
{
    installAuthorizationStub(false);
    const QString path = tmpDir.path() + "/pid-probe.txt";

    // Act
    bool ret = obj->saveFile(path.toUtf8(), QByteArray(""), "UTF-8");

    // Assert
    EXPECT_FALSE(ret);                                              // 期望：false（B5）
    EXPECT_EQ(lastActionId, QString("com.deepin.editor.saveFile")); // 固定 actionId 透传
    EXPECT_GT(lastPid, 0);                                          // pid 为当前进程（getpid() > 0）
}

// B1+B2+B4：深层缺失目录下新建文件并写入
TEST_F(DBusTest, SaveFile_NewFileInNonexistentNestedDir_CreatesAndWrites_ReturnsTrue)
{
    const QString path = tmpDir.path() + "/d1/d2/new-file.txt";
    const QByteArray content = "created-by-daemon 中文";

    // Act
    bool ret = obj->saveFile(path.toUtf8(), content, "UTF-8");

    // Assert
    EXPECT_TRUE(ret);                                   // 期望：成功（B4）
    EXPECT_TRUE(QFileInfo(path).isFile());              // 文件确实被创建（B2）
    EXPECT_TRUE(QDir(tmpDir.path() + "/d1/d2").exists());  // 父目录链被 mkpath（B2）
    QFile f(path);
    ASSERT_TRUE(f.open(QIODevice::ReadOnly));
    EXPECT_EQ(f.readAll(), content);                    // 内容一致（B4）
    f.close();
}

// B1+B4（跳过 B2）：已存在文件被覆盖
TEST_F(DBusTest, SaveFile_ExistingFile_OverwritesContent_ReturnsTrue)
{
    const QString path = tmpDir.path() + "/overwrite.txt";
    QFile old(path);
    ASSERT_TRUE(old.open(QIODevice::WriteOnly));
    old.write("OLD-CONTENT-that-should-be-replaced");
    old.close();
    const QByteArray newContent = "NEW-CONTENT";

    // Act
    bool ret = obj->saveFile(path.toUtf8(), newContent, "UTF-8");

    // Assert
    EXPECT_TRUE(ret);                       // 期望：成功（B4）
    QFile f(path);
    ASSERT_TRUE(f.open(QIODevice::ReadOnly));
    EXPECT_EQ(f.readAll(), newContent);     // 旧内容被整体覆盖
    f.close();
    EXPECT_EQ(authCalls, 1);                // 鉴权恰好一次（B1）
}

// B1+B2(创建失败被忽略)+B3：路径是已存在目录 → 打开 WriteOnly 失败 → false
TEST_F(DBusTest, SaveFile_PathIsDirectory_ReturnsFalseNegativeBranch)
{
    const QString dirPath = tmpDir.path() + "/i-am-a-dir";
    ASSERT_TRUE(QDir(tmpDir.path()).mkdir("i-am-a-dir"));

    // Act
    bool ret = obj->saveFile(dirPath.toUtf8(), QByteArray("data"), "UTF-8");

    // Assert
    EXPECT_FALSE(ret);                          // 期望：打开失败分支（B3）
    EXPECT_TRUE(QDir(dirPath).exists());        // 目录保持原状（未损坏）
    EXPECT_EQ(authCalls, 1);                    // 仍先走鉴权（B1）
}

// B1+B2+B3：空路径 → 打开失败 → false
TEST_F(DBusTest, SaveFile_EmptyPath_ReturnsFalseNegativeBranch)
{
    // Act
    bool ret = obj->saveFile(QByteArray(""), QByteArray("data"), "UTF-8");

    // Assert
    EXPECT_FALSE(ret);          // 期望：空路径不可打开（B3）
    EXPECT_EQ(authCalls, 1);    // 鉴权仍发生（B1）
}

} // namespace
