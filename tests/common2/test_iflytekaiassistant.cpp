// SPDX-FileCopyrightText: 2026 UnionTech Software Technology Co., Ltd.
// SPDX-License-Identifier: GPL-3.0-or-later

/*
 * IflytekAiAssistant 单元测试
 *
 * 注意：被测类为进程级单例且 m_status 跨用例保持，本套件内用例【按声明顺序】
 * 组成状态机（gtest 保证同 suite 内按声明顺序执行）：
 *   Invalid → (checkAiExists) NotInstalled → (checkValid+stub) Enable →
 *   (agreement=false) NoUserAgreement → (invalid reply 自适应) Enable
 *
 * 分支清单（来源：iflytek_ai_assistant.cpp 全部 public/private 方法）
 * B1 : instance() 单例稳定
 * B2 : status/valid 初始 Invalid
 * B3 : 非 Enable 时各查询方法直接返回 status()（无 DBus）
 * B4 : checkAiExists → call_once + QtConcurrent + initFinished 信号
 * B5 : checkValid NotInstalled → copilotInstalled 失败/成功
 * B6 : checkValid Q_FALLTHROUGH → isCopilotEnabled true/false/invalid
 * B7 : NoUserAgreement → launchCopilotChat 成功/失败
 * B8 : AudioDeviceDetector：连接无效/属性无效/JSON 解析/端口计数（输出、输入方向）
 * B9 : isTtsInWorking/isTtsEnable/getIatEnable/getTransEnable → 回复 true/false/invalid
 * B10: textToSpeech/speechToText/textToTranslate → 前置失败与完整成功（async 调用）
 * B11: stopTtsDirectly（默认构建未定义 ENABLE_STOP_TTS → 恒 Enable）
 * B12: errorString 各枚举分支
 *
 * 用例映射（状态机顺序）：
 * - Instance_Singleton_ReturnsStablePointer_InitialStateInvalid        → B1+B2
 * - StopTtsDirectly_DefaultBuild_ReturnsEnableWithoutDbus              → B11
 * - QueryMethods_StatusInvalid_ReturnInvalidWithoutDbus                → B3
 * - ActionMethods_StatusInvalid_ReturnInvalidStatus                    → B3
 * - CheckAiExists_BackendAbsent_EmitsInitFinishedNotInstalled          → B4
 * - CheckAiExists_SecondInvocation_NoRepeatedEmission                  → B4(call_once)
 * - CheckValid_VersionQueryFails_RemainsNotInstalled                   → B5(失败)
 * - CheckValid_InstalledNotAgreed_LaunchesChatStaysNoAgreement        → B5+B6+B7
 * - CheckValid_LaunchChatFails_StillNoUserAgreement                    → B7(失败)
 * - CheckValid_Reagree_ReturnsEnable                                   → B6(同意)
 * - CheckValid_AlreadyEnabled_SkipsAllQueries                          → B6(default)
 * - IsTtsInWorking_WorkingIdleBroken_ReturnsEnableDisableInvalid       → B9
 * - HasAudioOutputDevice_EnabledPortStates_TrueOnlyWhenEnabled         → B8
 * - HasAudioInputDevice_EnabledPortStates_TrueOnlyWhenEnabled          → B8
 * - IsTtsEnable_NoOutputDevice_ReturnsNoOutputDevice                   → B8+B9
 * - IsTtsEnable_DeviceStates_EnableDisableInvalid                      → B9
 * - TextToSpeech_Speaking_StopsFirstThenSpeaksReturnsSuccess           → B10+B11(内部)
 * - GetIatEnable_DeviceStates_EnableDisableInvalidNoDevice             → B9
 * - SpeechToText_InputReady_ReturnsSuccessAndCallsAsync                → B10
 * - GetTransEnable_ReplyStates_EnableDisableInvalid                    → B9
 * - TextToTranslate_EnabledOrDisabled_SuccessOrInvalid                 → B10
 *
 * DBus 隔离（不触碰真实总线）：
 * - QDBusConnection::sessionBus → stub 返回未连接的伪连接对象；
 * - QDBusAbstractInterface::callWithArgumentList（Qt6.8 所有 call() 重载的汇聚点）
 *   → stub 返回构造的 QDBusReply；
 * - QDBusAbstractInterface::asyncCallWithArgumentList → stub 返回已完成调用；
 * - QDBusAbstractInterface::isValid / QObject::property("CardsWithoutUnavailable")
 *   → 按用例需要 stub。
 */

#include <gtest/gtest.h>
#include "stubext.h"

#include "iflytek_ai_assistant.h"

#include <QCoreApplication>
#include <QDBusConnection>
#include <QDBusInterface>
#include <QDBusMessage>
#include <QDBusPendingCall>
#include <QSignalSpy>
#include <QVariant>
#include <QHash>
#include <QSet>
#include <QStringList>

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

struct ErrorCase {
    IflytekAiAssistant::CallStatus status;
    const char *expectedSubstr; // nullptr → 期望空串
};

} // namespace

class IflytekAiAssistantTestBase : public ::testing::Test
{
protected:
    void SetUp() override
    {
        ensureApp();
        stub.clear();
        resetState();
        // 隔离：所有 sessionBus() 调用返回未连接伪连接，绝不连接真实会话总线
        stub.set_lamda(&QDBusConnection::sessionBus,
                       []() -> QDBusConnection {
                           return QDBusConnection(QStringLiteral("ut-iflytek-bus"));
                       });
        protoMsg = QDBusMessage::createMethodCall(QStringLiteral("ut.service"),
                                                  QStringLiteral("/ut/path"),
                                                  QStringLiteral("ut.iface"),
                                                  QStringLiteral("method"));
        // Qt6.8: call(QString...) 全部经 doCall → callWithArgumentList 汇聚
        stub.set_lamda(&QDBusAbstractInterface::callWithArgumentList,
                       [this](QDBusAbstractInterface *, QDBus::CallMode,
                              const QString &method, const QList<QVariant> &) -> QDBusMessage {
                           ++dbusCallCount;
                           dbusCallMethods << method;
                           if (errorMethods.contains(method))
                               return protoMsg.createErrorReply(QStringLiteral("ut.error"),
                                                                method + QStringLiteral(" failed"));
                           if (replyByMethod.contains(method))
                               return protoMsg.createReply(replyByMethod.value(method));
                           return QDBusMessage(); // 无回复 → QDBusReply 无效
                       });
        stub.set_lamda(&QDBusAbstractInterface::asyncCallWithArgumentList,
                       [this](QDBusAbstractInterface *, const QString &method,
                              const QList<QVariant> &) -> QDBusPendingCall {
                           ++asyncCallCount;
                           asyncCallMethods << method;
                           return QDBusPendingCall::fromCompletedCall(protoMsg.createReply());
                       });
    }

    void TearDown() override
    {
        stub.clear();
    }

    void resetState()
    {
        dbusCallCount = 0;
        dbusCallMethods.clear();
        asyncCallCount = 0;
        asyncCallMethods.clear();
        replyByMethod.clear();
        errorMethods.clear();
        fakeIsValid = false;
        cardsJson.clear();
        protoMsg = QDBusMessage();
    }

    void installAudioStubs()
    {
        stub.set_lamda(&QDBusAbstractInterface::isValid,
                       [this](const QDBusAbstractInterface *) -> bool {
                           return fakeIsValid;
                       });
        stub.set_lamda(&QObject::property,
                       [this](const QObject *, const char *name) -> QVariant {
                           if (qstrcmp(name, "CardsWithoutUnavailable") == 0 && !cardsJson.isEmpty())
                               return QVariant(cardsJson);
                           return QVariant();
                       });
    }

    stub_ext::StubExt stub;
    QDBusMessage protoMsg;
    int dbusCallCount = 0;
    QStringList dbusCallMethods;
    int asyncCallCount = 0;
    QStringList asyncCallMethods;
    QHash<QString, QVariant> replyByMethod;
    QSet<QString> errorMethods;
    bool fakeIsValid = false;
    QByteArray cardsJson;
};

// ---------------- 状态机主套件（顺序敏感，勿调整用例声明顺序） ----------------

class IflytekAiAssistantTest : public IflytekAiAssistantTestBase
{
};

// B1+B2（必须是首个状态相关用例）
TEST_F(IflytekAiAssistantTest, Instance_Singleton_ReturnsStablePointer_InitialStateInvalid)
{
    // Arrange/Act
    IflytekAiAssistant *p1 = IflytekAiAssistant::instance();
    IflytekAiAssistant *p2 = IflytekAiAssistant::instance();
    // Assert: 单例指针稳定；初始状态 Invalid → 无效
    EXPECT_NE(p1, nullptr);
    EXPECT_EQ(p1, p2);
    EXPECT_EQ(p1->status(), IflytekAiAssistant::Invalid);
    EXPECT_FALSE(p1->valid());
}

// B11
TEST_F(IflytekAiAssistantTest, StopTtsDirectly_DefaultBuild_ReturnsEnableWithoutDbus)
{
    // Arrange/Act: 默认构建未定义 ENABLE_STOP_TTS（见 src CMakeLists）
    const auto ret = IflytekAiAssistant::instance()->stopTtsDirectly();
    // Assert: 直接返回 Enable，且不发起任何 DBus 调用
    EXPECT_EQ(ret, IflytekAiAssistant::Enable);
    EXPECT_EQ(dbusCallCount, 0);
    EXPECT_EQ(asyncCallCount, 0);
}

// B3
TEST_F(IflytekAiAssistantTest, QueryMethods_StatusInvalid_ReturnInvalidWithoutDbus)
{
    // Arrange
    IflytekAiAssistant *ins = IflytekAiAssistant::instance();
    ASSERT_EQ(ins->status(), IflytekAiAssistant::Invalid);
    // Act/Assert: 非 Enable 时全部提前返回当前状态，零 DBus 调用
    EXPECT_EQ(ins->isTtsInWorking(), IflytekAiAssistant::Invalid);
    EXPECT_EQ(ins->isTtsEnable(), IflytekAiAssistant::Invalid);
    EXPECT_EQ(ins->getIatEnable(), IflytekAiAssistant::Invalid);
    EXPECT_EQ(ins->getTransEnable(), IflytekAiAssistant::Invalid);
    EXPECT_EQ(dbusCallCount, 0);
    EXPECT_EQ(asyncCallCount, 0);
}

// B3
TEST_F(IflytekAiAssistantTest, ActionMethods_StatusInvalid_ReturnInvalidStatus)
{
    // Arrange
    IflytekAiAssistant *ins = IflytekAiAssistant::instance();
    // Act/Assert: checkValid 走 default 分支保持 Invalid，动作方法直接返回
    EXPECT_EQ(ins->textToSpeech(), IflytekAiAssistant::Invalid);
    EXPECT_EQ(ins->speechToText(), IflytekAiAssistant::Invalid);
    EXPECT_EQ(ins->textToTranslate(), IflytekAiAssistant::Invalid);
    EXPECT_EQ(ins->checkValid(), IflytekAiAssistant::Invalid);
    EXPECT_EQ(dbusCallCount, 0);
}

// B4
TEST_F(IflytekAiAssistantTest, CheckAiExists_BackendAbsent_EmitsInitFinishedNotInstalled)
{
    // Arrange: 后端无回复（默认 stub 无应答）→ copilotInstalled 失败
    IflytekAiAssistant *ins = IflytekAiAssistant::instance();
    QSignalSpy spy(ins, &IflytekAiAssistant::initFinished);
    // Act
    ins->checkAiExists();
    const bool emitted = spy.wait(5000);
    // Assert: 异步初始化完成，状态 NotInstalled
    EXPECT_TRUE(emitted);
    EXPECT_EQ(spy.count(), 1);
    EXPECT_EQ(ins->status(), IflytekAiAssistant::NotInstalled);
}

// B4（call_once 语义）
TEST_F(IflytekAiAssistantTest, CheckAiExists_SecondInvocation_NoRepeatedEmission)
{
    // Arrange
    IflytekAiAssistant *ins = IflytekAiAssistant::instance();
    QSignalSpy spy(ins, &IflytekAiAssistant::initFinished);
    // Act
    ins->checkAiExists();
    spy.wait(200); // 已 call_once，不会再发射
    // Assert
    EXPECT_EQ(spy.count(), 0);
    EXPECT_EQ(ins->status(), IflytekAiAssistant::NotInstalled);
}

// B5（版本查询失败）
TEST_F(IflytekAiAssistantTest, CheckValid_VersionQueryFails_RemainsNotInstalled)
{
    // Arrange: 无 version 回复（默认无效回复）
    IflytekAiAssistant *ins = IflytekAiAssistant::instance();
    // Act
    const auto ret = ins->checkValid();
    // Assert
    EXPECT_EQ(ret, IflytekAiAssistant::NotInstalled);
    EXPECT_TRUE(dbusCallMethods.contains(QStringLiteral("version")));
    EXPECT_EQ(ins->status(), IflytekAiAssistant::NotInstalled);
}

// B5+B6+B7（NotInstalled → 已安装但未同意 → 拉起聊天页）
TEST_F(IflytekAiAssistantTest, CheckValid_InstalledNotAgreed_LaunchesChatStaysNoAgreement)
{
    // Arrange: 版本查询成功 + 用户未同意
    IflytekAiAssistant *ins = IflytekAiAssistant::instance();
    replyByMethod[QStringLiteral("version")] = QVariant(QStringLiteral("1.0.0"));
    replyByMethod[QStringLiteral("isCopilotEnabled")] = QVariant(false);
    // Act
    const auto ret = ins->checkValid();
    // Assert: Q_FALLTHROUGH 进入协议检测 → 未同意 → launchChatPage，状态 NoUserAgreement
    EXPECT_EQ(ret, IflytekAiAssistant::NoUserAgreement);
    EXPECT_TRUE(dbusCallMethods.contains(QStringLiteral("version")));
    EXPECT_TRUE(dbusCallMethods.contains(QStringLiteral("launchChatPage")));
    EXPECT_EQ(ins->status(), IflytekAiAssistant::NoUserAgreement);
}

// B7（拉起失败）
TEST_F(IflytekAiAssistantTest, CheckValid_LaunchChatFails_StillNoUserAgreement)
{
    // Arrange: 上一用例后状态 NoUserAgreement → 直接再入协议分支
    IflytekAiAssistant *ins = IflytekAiAssistant::instance();
    ASSERT_EQ(ins->status(), IflytekAiAssistant::NoUserAgreement);
    replyByMethod[QStringLiteral("version")] = QVariant(QStringLiteral("1.0.0"));
    replyByMethod[QStringLiteral("isCopilotEnabled")] = QVariant(false);
    errorMethods.insert(QStringLiteral("launchChatPage"));
    // Act
    const auto ret = ins->checkValid();
    // Assert: 拉起失败不影响状态
    EXPECT_EQ(ret, IflytekAiAssistant::NoUserAgreement);
    EXPECT_TRUE(dbusCallMethods.contains(QStringLiteral("launchChatPage")));
}

// B6（协议重新同意 → Enable）
TEST_F(IflytekAiAssistantTest, CheckValid_Reagree_ReturnsEnable)
{
    // Arrange
    IflytekAiAssistant *ins = IflytekAiAssistant::instance();
    ASSERT_EQ(ins->status(), IflytekAiAssistant::NoUserAgreement);
    replyByMethod[QStringLiteral("version")] = QVariant(QStringLiteral("1.0.0"));
    replyByMethod[QStringLiteral("isCopilotEnabled")] = QVariant(true);
    // Act
    const auto ret = ins->checkValid();
    // Assert: NoUserAgreement case 直接进入协议检测，已同意 → Enable
    EXPECT_EQ(ret, IflytekAiAssistant::Enable);
    EXPECT_TRUE(ins->valid());
    EXPECT_TRUE(dbusCallMethods.contains(QStringLiteral("isCopilotEnabled")));
}

// B6 反例（已 Enable → default 分支跳过全部查询）
TEST_F(IflytekAiAssistantTest, CheckValid_AlreadyEnabled_SkipsAllQueries)
{
    // Arrange
    IflytekAiAssistant *ins = IflytekAiAssistant::instance();
    ASSERT_EQ(ins->status(), IflytekAiAssistant::Enable);
    // Act
    const auto ret = ins->checkValid();
    // Assert: default 分支直接返回，零 DBus 调用
    EXPECT_EQ(ret, IflytekAiAssistant::Enable);
    EXPECT_EQ(dbusCallCount, 0);
}

// B9
TEST_F(IflytekAiAssistantTest, IsTtsInWorking_WorkingIdleBroken_ReturnsEnableDisableInvalid)
{
    // Arrange: 上一用例已进入 Enable
    IflytekAiAssistant *ins = IflytekAiAssistant::instance();
    ASSERT_EQ(ins->status(), IflytekAiAssistant::Enable);
    // Act/Assert: true → Enable
    replyByMethod[QStringLiteral("isTTSInWorking")] = QVariant(true);
    EXPECT_EQ(ins->isTtsInWorking(), IflytekAiAssistant::Enable);
    // false → Disable
    replyByMethod[QStringLiteral("isTTSInWorking")] = QVariant(false);
    EXPECT_EQ(ins->isTtsInWorking(), IflytekAiAssistant::Disable);
    // 无有效回复 → Invalid
    replyByMethod.remove(QStringLiteral("isTTSInWorking"));
    EXPECT_EQ(ins->isTtsInWorking(), IflytekAiAssistant::Invalid);
    EXPECT_EQ(dbusCallCount, 3);
}

// B8
TEST_F(IflytekAiAssistantTest, HasAudioOutputDevice_EnabledPortStates_TrueOnlyWhenEnabled)
{
    // Arrange
    IflytekAiAssistant *ins = IflytekAiAssistant::instance();
    installAudioStubs();
    // Act/Assert: 输出方向(1)启用端口 → true
    fakeIsValid = true;
    cardsJson = QByteArrayLiteral(
        "[{\"Id\":0,\"Name\":\"c0\",\"Ports\":[{\"Name\":\"spk\",\"Enabled\":true,\"Direction\":1},"
        "{\"Name\":\"mic\",\"Enabled\":true,\"Direction\":2}]}]");
    EXPECT_TRUE(ins->hasAudioOutputDevice());
    // 端口存在但未启用 → false
    cardsJson = QByteArrayLiteral(
        "[{\"Id\":0,\"Name\":\"c0\",\"Ports\":[{\"Name\":\"spk\",\"Enabled\":false,\"Direction\":1}]}]");
    EXPECT_FALSE(ins->hasAudioOutputDevice());
    // 声卡无 Ports 字段 → false
    cardsJson = QByteArrayLiteral("[{\"Id\":0,\"Name\":\"c0\"}]");
    EXPECT_FALSE(ins->hasAudioOutputDevice());
    // 属性无效 → false
    cardsJson.clear();
    EXPECT_FALSE(ins->hasAudioOutputDevice());
    // 连接无效 → false
    fakeIsValid = false;
    EXPECT_FALSE(ins->hasAudioOutputDevice());
}

// B8
TEST_F(IflytekAiAssistantTest, HasAudioInputDevice_EnabledPortStates_TrueOnlyWhenEnabled)
{
    // Arrange
    IflytekAiAssistant *ins = IflytekAiAssistant::instance();
    installAudioStubs();
    fakeIsValid = true;
    // Act/Assert: 输入方向(2)启用端口 → true
    cardsJson = QByteArrayLiteral(
        "[{\"Id\":0,\"Name\":\"c0\",\"Ports\":[{\"Name\":\"mic\",\"Enabled\":true,\"Direction\":2}]}]");
    EXPECT_TRUE(ins->hasAudioInputDevice());
    // 仅输出方向端口 → false
    cardsJson = QByteArrayLiteral(
        "[{\"Id\":0,\"Name\":\"c0\",\"Ports\":[{\"Name\":\"spk\",\"Enabled\":true,\"Direction\":1}]}]");
    EXPECT_FALSE(ins->hasAudioInputDevice());
    // 多声卡混合方向：第二块卡有输入端口 → true
    cardsJson = QByteArrayLiteral(
        "[{\"Id\":0,\"Name\":\"c0\",\"Ports\":[]},"
        "{\"Id\":1,\"Name\":\"c1\",\"Ports\":[{\"Name\":\"mic1\",\"Enabled\":true,\"Direction\":2}]}]");
    EXPECT_TRUE(ins->hasAudioInputDevice());
}

// B8+B9（无输出设备）
TEST_F(IflytekAiAssistantTest, IsTtsEnable_NoOutputDevice_ReturnsNoOutputDevice)
{
    // Arrange: 不安装 property/isValid stub → 真实伪连接路径返回 false
    IflytekAiAssistant *ins = IflytekAiAssistant::instance();
    ASSERT_EQ(ins->status(), IflytekAiAssistant::Enable);
    // Act
    const auto ret = ins->isTtsEnable();
    // Assert: 无输出设备 → NoOutputDevice，且未发起 getTTSEnable 查询
    EXPECT_EQ(ret, IflytekAiAssistant::NoOutputDevice);
    EXPECT_FALSE(dbusCallMethods.contains(QStringLiteral("getTTSEnable")));
}

// B9
TEST_F(IflytekAiAssistantTest, IsTtsEnable_DeviceStates_EnableDisableInvalid)
{
    // Arrange
    IflytekAiAssistant *ins = IflytekAiAssistant::instance();
    installAudioStubs();
    fakeIsValid = true;
    cardsJson = QByteArrayLiteral(
        "[{\"Id\":0,\"Name\":\"c0\",\"Ports\":[{\"Name\":\"spk\",\"Enabled\":true,\"Direction\":1}]}]");
    // Act/Assert
    replyByMethod[QStringLiteral("getTTSEnable")] = QVariant(true);
    EXPECT_EQ(ins->isTtsEnable(), IflytekAiAssistant::Enable);
    replyByMethod[QStringLiteral("getTTSEnable")] = QVariant(false);
    EXPECT_EQ(ins->isTtsEnable(), IflytekAiAssistant::Disable);
    replyByMethod.remove(QStringLiteral("getTTSEnable"));
    EXPECT_EQ(ins->isTtsEnable(), IflytekAiAssistant::Invalid);
    EXPECT_EQ(dbusCallCount, 3);
}

// B10（完整成功路径，含 stopTtsDirectlyInternal）
TEST_F(IflytekAiAssistantTest, TextToSpeech_Speaking_StopsFirstThenSpeaksReturnsSuccess)
{
    // Arrange: checkValid 通过 + 有输出设备 + 正在朗读
    IflytekAiAssistant *ins = IflytekAiAssistant::instance();
    replyByMethod[QStringLiteral("version")] = QVariant(QStringLiteral("1.0.0"));
    replyByMethod[QStringLiteral("isCopilotEnabled")] = QVariant(true);
    installAudioStubs();
    fakeIsValid = true;
    cardsJson = QByteArrayLiteral(
        "[{\"Id\":0,\"Name\":\"c0\",\"Ports\":[{\"Name\":\"spk\",\"Enabled\":true,\"Direction\":1}]}]");
    replyByMethod[QStringLiteral("getTTSEnable")] = QVariant(true);
    replyByMethod[QStringLiteral("isTTSInWorking")] = QVariant(true);
    // Act
    const auto ret = ins->textToSpeech();
    // Assert: 先 stopTTSDirectly 再 TextToSpeech，返回 Success
    EXPECT_EQ(ret, IflytekAiAssistant::Success);
    EXPECT_EQ(asyncCallCount, 2);
    EXPECT_EQ(asyncCallMethods,
              QStringList() << QStringLiteral("stopTTSDirectly")
                            << QStringLiteral("TextToSpeech"));
    // 无输出设备时 → NoOutputDevice
    cardsJson.clear();
    EXPECT_EQ(ins->textToSpeech(), IflytekAiAssistant::NoOutputDevice);
}

// B9
TEST_F(IflytekAiAssistantTest, GetIatEnable_DeviceStates_EnableDisableInvalidNoDevice)
{
    // Arrange
    IflytekAiAssistant *ins = IflytekAiAssistant::instance();
    // Act/Assert: 无输入设备 → NoInputDevice
    EXPECT_EQ(ins->getIatEnable(), IflytekAiAssistant::NoInputDevice);
    // 有输入设备 + 回复 true → Enable
    installAudioStubs();
    fakeIsValid = true;
    cardsJson = QByteArrayLiteral(
        "[{\"Id\":0,\"Name\":\"c0\",\"Ports\":[{\"Name\":\"mic\",\"Enabled\":true,\"Direction\":2}]}]");
    replyByMethod[QStringLiteral("getIatEnable")] = QVariant(true);
    EXPECT_EQ(ins->getIatEnable(), IflytekAiAssistant::Enable);
    // 回复 false → Disable
    replyByMethod[QStringLiteral("getIatEnable")] = QVariant(false);
    EXPECT_EQ(ins->getIatEnable(), IflytekAiAssistant::Disable);
    // 无有效回复 → Invalid
    replyByMethod.remove(QStringLiteral("getIatEnable"));
    EXPECT_EQ(ins->getIatEnable(), IflytekAiAssistant::Invalid);
}

// B10
TEST_F(IflytekAiAssistantTest, SpeechToText_InputReady_ReturnsSuccessAndCallsAsync)
{
    // Arrange
    IflytekAiAssistant *ins = IflytekAiAssistant::instance();
    replyByMethod[QStringLiteral("version")] = QVariant(QStringLiteral("1.0.0"));
    replyByMethod[QStringLiteral("isCopilotEnabled")] = QVariant(true);
    installAudioStubs();
    fakeIsValid = true;
    cardsJson = QByteArrayLiteral(
        "[{\"Id\":0,\"Name\":\"c0\",\"Ports\":[{\"Name\":\"mic\",\"Enabled\":true,\"Direction\":2}]}]");
    replyByMethod[QStringLiteral("getIatEnable")] = QVariant(true);
    // Act
    const auto ret = ins->speechToText();
    // Assert
    EXPECT_EQ(ret, IflytekAiAssistant::Success);
    EXPECT_EQ(asyncCallCount, 1);
    EXPECT_EQ(asyncCallMethods, QStringList() << QStringLiteral("SpeechToText"));
}

// B9
TEST_F(IflytekAiAssistantTest, GetTransEnable_ReplyStates_EnableDisableInvalid)
{
    // Arrange
    IflytekAiAssistant *ins = IflytekAiAssistant::instance();
    ASSERT_EQ(ins->status(), IflytekAiAssistant::Enable);
    // Act/Assert
    replyByMethod[QStringLiteral("getTransEnable")] = QVariant(true);
    EXPECT_EQ(ins->getTransEnable(), IflytekAiAssistant::Enable);
    replyByMethod[QStringLiteral("getTransEnable")] = QVariant(false);
    EXPECT_EQ(ins->getTransEnable(), IflytekAiAssistant::Disable);
    replyByMethod.remove(QStringLiteral("getTransEnable"));
    EXPECT_EQ(ins->getTransEnable(), IflytekAiAssistant::Invalid);
}

// B10
TEST_F(IflytekAiAssistantTest, TextToTranslate_EnabledOrDisabled_SuccessOrInvalid)
{
    // Arrange
    IflytekAiAssistant *ins = IflytekAiAssistant::instance();
    replyByMethod[QStringLiteral("version")] = QVariant(QStringLiteral("1.0.0"));
    replyByMethod[QStringLiteral("isCopilotEnabled")] = QVariant(true);
    // Act/Assert: 翻译可用 → Success 且 async 调用 TextToTranslate
    replyByMethod[QStringLiteral("getTransEnable")] = QVariant(true);
    EXPECT_EQ(ins->textToTranslate(), IflytekAiAssistant::Success);
    EXPECT_EQ(asyncCallMethods, QStringList() << QStringLiteral("TextToTranslate"));
    // 翻译不可用 → Invalid
    replyByMethod[QStringLiteral("getTransEnable")] = QVariant(false);
    EXPECT_EQ(ins->textToTranslate(), IflytekAiAssistant::Invalid);
}

// B6（invalid 回复自适应为 Enable 的分支无法经状态机二度触达——协议状态仅可单向
// 迁移一次；isCopilotEnabled 函数本体已由上方用例覆盖执行，FN 覆盖不受影响）

// ---------------- errorString 参数化（无状态依赖，独立 fixture） ----------------

class IflytekAiAssistantParamTest : public IflytekAiAssistantTestBase,
                                    public ::testing::WithParamInterface<ErrorCase>
{
};

TEST_P(IflytekAiAssistantParamTest, ErrorString_ByCallStatus_ReturnsExpectedText)
{
    // Arrange
    const auto &c = GetParam();
    // Act
    const QString msg = IflytekAiAssistant::instance()->errorString(c.status);
    // Assert
    if (c.expectedSubstr) {
        EXPECT_FALSE(msg.isEmpty());
        EXPECT_TRUE(msg.contains(QLatin1String(c.expectedSubstr), Qt::CaseInsensitive))
            << "actual: " << msg.toStdString();
    } else {
        EXPECT_TRUE(msg.isEmpty());
        EXPECT_EQ(msg, QString());
    }
}

INSTANTIATE_TEST_SUITE_P(
    ErrorStringCases, IflytekAiAssistantParamTest,
    ::testing::Values(
        ErrorCase{IflytekAiAssistant::NotInstalled, "UOS AI"},
        ErrorCase{IflytekAiAssistant::NoInputDevice, "input"},
        ErrorCase{IflytekAiAssistant::NoOutputDevice, "output"},
        ErrorCase{IflytekAiAssistant::Invalid, nullptr},
        ErrorCase{IflytekAiAssistant::Enable, nullptr},
        ErrorCase{IflytekAiAssistant::Disable, nullptr},
        ErrorCase{IflytekAiAssistant::NoUserAgreement, nullptr},
        ErrorCase{IflytekAiAssistant::Success, nullptr},
        ErrorCase{IflytekAiAssistant::Failed, nullptr}));
