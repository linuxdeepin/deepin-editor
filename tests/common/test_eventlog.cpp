// SPDX-FileCopyrightText: 2026 UnionTech Software Technology Co., Ltd.
// SPDX-License-Identifier: GPL-3.0-or-later

#include <gtest/gtest.h>
#include "stubext.h"

#include <QJsonObject>
#include <QJsonDocument>
#include <QLibrary>
#include <QCoreApplication>
#include <QFunctionPointer>
#include <QString>
#include <cstring>
#include <string>

// 访问 private 静态成员 m_pInstance（单例重置，用例间隔离）
// 说明：eventlogutils.h 仅依赖 <QJsonObject>/<string>，先正常包含依赖头，
//       再 #define private public 解析目标头，不影响 Qt 头的首次解析。
#define private public
#include "eventlogutils.h"
#undef private

// ---------------------------------------------------------------------------
// 分支清单（来源：src/common/eventlogutils.cpp）
// E1: Eventlogutils::GetInstance()      m_pInstance == nullptr → new
// E2: Eventlogutils::writeLogs()        writeEventLogFunc 为空 → 提前 return
// E3: Eventlogutils::writeLogs()        writeEventLogFunc 非空 → 调用上报
// E4: Eventlogutils::Eventlogutils()    initFunc 为空 → 提前 return（不初始化）
// E5: Eventlogutils::Eventlogutils()    initFunc 非空 → initFunc("deepin-editor", true)
//
// 用例映射：
// - GetInstance_FirstCall_CreatesInitializedSingleton     → E1 + E5
// - GetInstance_SecondCall_ReturnsSameInstance            → E1（false 分支）
// - WriteLogs_ValidData_CallsWriteEventLogWithCompactJson → E3
// - WriteLogs_MultipleJsonPayloads_PayloadPassedVerbatim  → E3（TEST_P 多载荷）
// - WriteLogs_WriteFuncNull_SkipsReportAndKeepsState      → E2 + E4
//
// 环境隔离：QLibrary::resolve 全程 stub，绝不 dlopen 真实 libdeepin-event-log.so，
//           无任何网络/动态库副作用；计数器为文件级静态（QFunctionPointer::fromFunction
//           只能绑定无捕获自由函数），每个用例 SetUp 内重置，杜绝用例间污染。
// ---------------------------------------------------------------------------

namespace {
int g_initCalls = 0;
std::string g_initPackageName;
bool g_initEnableSig = false;
int g_writeCalls = 0;
std::string g_lastWrittenPayload;

bool fakeInitialize(const std::string &packagename, bool enable_sig)
{
    ++g_initCalls;
    g_initPackageName = packagename;
    g_initEnableSig = enable_sig;
    return true;
}

void fakeWriteEventLog(const std::string &eventdata)
{
    ++g_writeCalls;
    g_lastWrittenPayload = eventdata;
}

void resetCounters()
{
    g_initCalls = 0;
    g_initPackageName.clear();
    g_initEnableSig = false;
    g_writeCalls = 0;
    g_lastWrittenPayload.clear();
}
} // namespace

class EventlogutilsTest : public ::testing::Test {
protected:
    static void SetUpTestSuite()
    {
        int argc = 1;
        static char appArg0[] = "test_eventlog";
        static char *argv[] = { appArg0, nullptr };
        s_app = new QCoreApplication(argc, argv);
    }

    static void TearDownTestSuite() { delete s_app; }

    void SetUp() override
    {
        resetCounters();
        stub.clear();
        // 重置单例，保证每个用例从干净状态开始
        delete Eventlogutils::m_pInstance;
        Eventlogutils::m_pInstance = nullptr;
    }

    void TearDown() override
    {
        stub.clear();
        delete Eventlogutils::m_pInstance;
        Eventlogutils::m_pInstance = nullptr;
        resetCounters();
    }

    void stubResolveSuccess()
    {
        // Qt6：QFunctionPointer 是 void(*)() typedef，函数指针经 reinterpret_cast 还原原签名调用
        stub.set_lamda(static_cast<QFunctionPointer (QLibrary::*)(const char *)>(&QLibrary::resolve),
                       [](QLibrary *, const char *symbol) -> QFunctionPointer {
                           if (std::strcmp(symbol, "Initialize") == 0)
                               return reinterpret_cast<QFunctionPointer>(&fakeInitialize);
                           if (std::strcmp(symbol, "WriteEventLog") == 0)
                               return reinterpret_cast<QFunctionPointer>(&fakeWriteEventLog);
                           return nullptr;
                       });
    }

    void stubResolveFailure()
    {
        stub.set_lamda(static_cast<QFunctionPointer (QLibrary::*)(const char *)>(&QLibrary::resolve),
                       [](QLibrary *, const char *) -> QFunctionPointer {
                           return nullptr;
                       });
    }

    stub_ext::StubExt stub;
    static QCoreApplication *s_app;
};

QCoreApplication *EventlogutilsTest::s_app = nullptr;

TEST_F(EventlogutilsTest, GetInstance_FirstCall_CreatesInitializedSingleton)
{
    // Arrange
    stubResolveSuccess();

    // Act
    Eventlogutils *inst = Eventlogutils::GetInstance();

    // Assert：单例创建成功且库初始化按约定参数执行一次
    ASSERT_NE(inst, nullptr);
    EXPECT_EQ(g_initCalls, 1);                                  // Initialize 恰好调用一次
    EXPECT_EQ(g_initPackageName, std::string("deepin-editor")); // 包名固定
    EXPECT_TRUE(g_initEnableSig);                               // enable_sig 固定为 true
}

TEST_F(EventlogutilsTest, GetInstance_SecondCall_ReturnsSameInstance)
{
    // Arrange
    stubResolveSuccess();
    Eventlogutils *first = Eventlogutils::GetInstance();

    // Act
    Eventlogutils *second = Eventlogutils::GetInstance();

    // Assert：懒汉单例复用同一实例，不重复初始化
    EXPECT_EQ(first, second);
    EXPECT_EQ(g_initCalls, 1); // 只在首次创建时初始化
}

TEST_F(EventlogutilsTest, WriteLogs_ValidData_CallsWriteEventLogWithCompactJson)
{
    // Arrange
    stubResolveSuccess();
    Eventlogutils *inst = Eventlogutils::GetInstance();
    ASSERT_NE(inst, nullptr);
    QJsonObject data;
    data.insert("tid", static_cast<double>(Eventlogutils::StartUp));
    data.insert("version", QString("1.0.0"));
    const QByteArray expected =
            QJsonDocument(data).toJson(QJsonDocument::Compact);

    // Act
    inst->writeLogs(data);

    // Assert：紧凑 JSON 原样传给 WriteEventLog
    EXPECT_EQ(g_writeCalls, 1);
    EXPECT_EQ(g_lastWrittenPayload, expected.toStdString());
}

namespace {
struct WriteLogsCase {
    int tid;
    const char *extraKey;
    QString extraValue;
};
} // namespace

class EventlogutilsParamTest : public EventlogutilsTest,
                               public ::testing::WithParamInterface<WriteLogsCase> {
};

TEST_P(EventlogutilsParamTest, WriteLogs_MultipleJsonPayloads_PayloadPassedVerbatim)
{
    // Arrange
    stubResolveSuccess();
    Eventlogutils *inst = Eventlogutils::GetInstance();
    ASSERT_NE(inst, nullptr);
    QJsonObject data;
    data.insert("tid", static_cast<double>(GetParam().tid));
    data.insert(QString::fromLatin1(GetParam().extraKey), GetParam().extraValue);
    const QByteArray expected =
            QJsonDocument(data).toJson(QJsonDocument::Compact);

    // Act
    inst->writeLogs(data);

    // Assert：任意事件载荷均按紧凑 JSON 上报
    EXPECT_EQ(g_writeCalls, 1);
    EXPECT_EQ(g_lastWrittenPayload, expected.toStdString());
}

INSTANTIATE_TEST_SUITE_P(
        EventLogPayloads, EventlogutilsParamTest,
        ::testing::Values(
                WriteLogsCase{ Eventlogutils::OpenTime, "time", QString("1234") },
                WriteLogsCase{ Eventlogutils::CloseTime, "message", QString("hello world") },
                WriteLogsCase{ Eventlogutils::Quit, "user", QString("ut-user") }));

TEST_F(EventlogutilsTest, WriteLogs_WriteFuncNull_SkipsReportAndKeepsState)
{
    // Arrange：resolve 失败 → initFunc/writeEventLogFunc 均为空
    stubResolveFailure();
    Eventlogutils *inst = Eventlogutils::GetInstance();
    ASSERT_NE(inst, nullptr);
    EXPECT_EQ(g_initCalls, 0); // E4：初始化函数为空，未调用
    QJsonObject data;
    data.insert("tid", static_cast<double>(Eventlogutils::StartUp));

    // Act
    inst->writeLogs(data);

    // Assert：E2 路径——未触发任何上报（强异常安全：无副作用）
    EXPECT_EQ(g_writeCalls, 0);
    EXPECT_TRUE(g_lastWrittenPayload.empty());
}
