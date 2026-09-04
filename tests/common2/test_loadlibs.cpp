// SPDX-FileCopyrightText: 2026 UnionTech Software Technology Co., Ltd.
// SPDX-License-Identifier: GPL-3.0-or-later

/*
 * load_libs(src/basepub/load_libs.c) 单元测试
 *
 * 注意：getLoadZPDLibsInstance 为饿汉式全局单例（pLibs 静态指针），
 * 本套件内用例【按声明顺序】组成状态机（gtest 保证同 suite 声明顺序）：
 *   setLibNames(不存在路径) → 首次 getLoadZPDLibsInstance（dlopen 失败，函数指针为 NULL）
 *   → 再次调用（缓存直返）→ setLibNames(NULL)（不重载）→ 并发双检锁一致性
 *
 * 分支清单（来源：load_libs.c）
 * B1 : setLibNames(非空) → malloc 拷贝路径
 * B2 : setLibNames(NULL) → 置空
 * B3 : newClass: chZPDDLL 非空 + dlopen 失败 → PrintError + 返回空函数指针结构
 * B4 : getLoadZPDLibsInstance: pLibs==NULL → 加锁创建（双检锁）
 * B5 : getLoadZPDLibsInstance: pLibs!=NULL → 直接返回缓存
 * B6 : PrintError: dlerror() 非空 → 打印；为空 → 静默
 *
 * 用例映射：
 * - GetInstance_NonexistentLibPath_ReturnsNullFunctionPointers  → B1+B3+B4
 * - GetInstance_SecondCall_ReturnsCachedPointer                 → B5
 * - SetLibNames_NullAfterInit_InstanceUnchanged                 → B2+B5
 * - PrintError_DlErrorSetOrCleared_PrintsOrSilent               → B6
 * - GetInstance_ConcurrentCalls_SamePointerReturned             → B4/B5 并发
 *
 * 隔离：dlopen 仅使用 QTemporaryDir 下【不存在】的 .so 路径（必失败，不加载任何库）；
 *      PrintError 的 stderr 输出重定向到临时文件捕获。
 */

#include <gtest/gtest.h>
#include "stubext.h"

#include <QCoreApplication>
#include <QTemporaryDir>
#include <QFile>

#include <fcntl.h>
#include <unistd.h>
#include <dlfcn.h>
#include <pthread.h>
#include <cstdio>
#include <cstring>
#include <thread>
#include <vector>

// C 头以 extern "C" 引入，保证与 load_libs.c 的链接符号一致
extern "C" {
#include "load_libs.h"
void PrintError();
}

namespace {

QCoreApplication *ensureApp()
{
    if (!QCoreApplication::instance()) {
        static int argc = 1;
        static char argv0[] = "test_common2";
        static char *argv[2] = { argv0, nullptr };
        static QCoreApplication app(argc, argv);
        return &app;
    }
    return QCoreApplication::instance();
}

// stderr → 文件捕获；返回原 fd 备份
int captureStderrBegin(const QString &path)
{
    fflush(stderr);
    const int saved = dup(fileno(stderr));
    const int fd = open(path.toLocal8Bit().constData(), O_WRONLY | O_CREAT | O_TRUNC, 0600);
    dup2(fd, fileno(stderr));
    close(fd);
    return saved;
}

void captureStderrEnd(int saved)
{
    fflush(stderr);
    dup2(saved, fileno(stderr));
    close(saved);
}

} // namespace

class LoadLibsTest : public ::testing::Test
{
protected:
    void SetUp() override
    {
        ensureApp();
        tmpDir = std::make_unique<QTemporaryDir>();
        ASSERT_TRUE(tmpDir->isValid());
    }

    std::unique_ptr<QTemporaryDir> tmpDir;
};

// B1+B3+B4（必须是首个触发单例创建的用例）
TEST_F(LoadLibsTest, GetInstance_NonexistentLibPath_ReturnsNullFunctionPointers)
{
    // Arrange: 临时目录下不存在的 .so → dlopen 必然失败
    const QByteArray libPath = (tmpDir->path() + QStringLiteral("/libutzpd-not-exist.so")).toLocal8Bit();
    LoadLibNames names;
    names.chZPDDLL = const_cast<char *>(libPath.constData());
    setLibNames(names);
    // Act
    LoadLibs *libs = getLoadZPDLibsInstance();
    // Assert: 结构体已分配，但两个函数指针因加载失败为 NULL
    ASSERT_NE(libs, nullptr);
    EXPECT_EQ(libs->m_document_clip_copy, nullptr);
    EXPECT_EQ(libs->m_document_close, nullptr);
}

// B5
TEST_F(LoadLibsTest, GetInstance_SecondCall_ReturnsCachedPointer)
{
    // Arrange
    LoadLibs *first = getLoadZPDLibsInstance();
    ASSERT_NE(first, nullptr);
    // Act
    LoadLibs *second = getLoadZPDLibsInstance();
    // Assert: 已创建后直接返回缓存（不重新 dlopen），状态保持
    EXPECT_EQ(second, first);
    EXPECT_EQ(second->m_document_close, nullptr);
}

// B2+B5
TEST_F(LoadLibsTest, SetLibNames_NullAfterInit_InstanceUnchanged)
{
    // Arrange
    LoadLibs *before = getLoadZPDLibsInstance();
    ASSERT_NE(before, nullptr);
    LoadLibNames names;
    names.chZPDDLL = nullptr;
    // Act
    setLibNames(names);
    LoadLibs *after = getLoadZPDLibsInstance();
    // Assert: 饿汉式单例创建后 setLibNames 不触发重载
    EXPECT_EQ(after, before);
    EXPECT_EQ(after->m_document_clip_copy, nullptr);
}

// B6
TEST_F(LoadLibsTest, PrintError_DlErrorSetOrCleared_PrintsOrSilent)
{
    // Arrange: 制造 dlopen 失败使 dlerror() 非空
    const QString capture = tmpDir->path() + QStringLiteral("/stderr.txt");
    const QString missing = tmpDir->path() + QStringLiteral("/missing-lib.so");
    void *handle = dlopen(missing.toLocal8Bit().constData(), RTLD_NOW);
    EXPECT_EQ(handle, nullptr);
    // Act & Assert 1: dlerror 非空 → 打印错误
    int saved = captureStderrBegin(capture);
    PrintError();
    captureStderrEnd(saved);
    QFile f1(capture);
    ASSERT_TRUE(f1.open(QIODevice::ReadOnly));
    const QByteArray out1 = f1.readAll();
    f1.close();
    EXPECT_FALSE(out1.isEmpty()); // 打印了 dlerror 信息

    // Act & Assert 2: 消费掉错误后 → 静默
    (void)dlerror();
    saved = captureStderrBegin(capture);
    PrintError();
    captureStderrEnd(saved);
    QFile f2(capture);
    ASSERT_TRUE(f2.open(QIODevice::ReadOnly));
    const QByteArray out2 = f2.readAll();
    f2.close();
    EXPECT_TRUE(out2.isEmpty()); // 无错误可打印
}

// B4/B5 并发
TEST_F(LoadLibsTest, GetInstance_ConcurrentCalls_SamePointerReturned)
{
    // Arrange
    LoadLibs *expected = getLoadZPDLibsInstance();
    std::vector<LoadLibs *> results(8, nullptr);
    std::vector<std::thread> workers;
    // Act: 8 线程并发获取
    for (size_t i = 0; i < results.size(); ++i) {
        workers.emplace_back([&results, i]() {
            results[i] = getLoadZPDLibsInstance();
        });
    }
    for (auto &w : workers)
        w.join();
    // Assert: 双检锁保证全部返回同一实例
    for (const LoadLibs *p : results) {
        EXPECT_EQ(p, expected);
    }
    EXPECT_EQ(results.size(), 8u);
    EXPECT_NE(expected, nullptr);
}
