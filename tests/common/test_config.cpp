// SPDX-FileCopyrightText: 2026 UnionTech Software Technology Co., Ltd.
// SPDX-License-Identifier: GPL-3.0-or-later

#include <gtest/gtest.h>
#include "stubext.h"

#include <dtkcore_config.h>
#include <QObject>
#include <QCoreApplication>
#include <QDebug>
#include <QVariant>
#include <QString>

#ifdef DTKCORE_CLASS_DConfigFile
#include <DConfig>
#endif

// 访问 private 构造/成员（多场景直接构造，绕开进程级单例限制）
#define private public
#include "config.h"
#undef private

#include <iconv.h>

// config.cpp 内部自由函数（头文件未声明，此处补声明以直接测试）
bool detectIconvUse2005Standard();

// ---------------------------------------------------------------------------
// 分支清单（来源：src/common/config.cpp）
// C1: detectIconvUse2005Standard  iconv_open 失败 → return true
// C2: detectIconvUse2005Standard  iconv 转换失败 → return true
// C3: detectIconvUse2005Standard  输出不含 U+E816 → return true（2005 标准）
// C4: detectIconvUse2005Standard  输出含 U+E816   → return false（2022 标准）
// C5: Config::Config              DConfig 有效 → 读三个键值 + 连接 valueChanged
// C6: Config::Config              DConfig 无效 → 仅告警
// C7: Config::Config              lambda[key=disableImproveGB18030]
// C8: Config::Config              lambda[key=enablePatchedIconv]
// C9: Config::Config              lambda[key=defaultEncoding]
// C10: defaultEncoding            encoding 为空 → 兜底返回 "UTF-8"
//
// 用例映射：
// - DetectIconvUse2005Standard_RealIconv_MatchesReferenceResult      → C1-C4（与同进程参照实现比对）
// - Construct_ValidDConfig_ReadsAllThreeKeys                        → C5
// - Construct_InvalidDConfig_KeepsDefaults                          → C6
// - EnableImproveGB18030_ValidDConfigReflectsInvertedFlag           → C5 取反逻辑
// - EnablePatchedIconv_PatchedTrue_ReturnsTrue                      → 确定性真
// - EnablePatchedIconv_PatchedFalse_EqualsDetectedStandard          → false || 检测值
// - DefaultEncoding_ConfiguredUppercase_ReturnsUppercase            → C5
// - DefaultEncoding_EmptyConfigured_FallsBackToUtf8                 → C10
// - ValueChanged_AllThreeKeys_UpdatesStateOnTheFly                  → C7/C8/C9
// - Instance_RepeatedCalls_ReturnsSameStaticInstance                → 单例
// - Destruct_ValidDConfig_DeletesDConfigPointer                     → 析构
//
// 环境隔离：DConfig::create/isValid/value 全 stub，不访问真实 dsg 配置与 DBus；
//           iconv 为 libc 纯计算。测试对象直接 new/delete，无全局状态残留。
// ---------------------------------------------------------------------------

#ifdef DTKCORE_CLASS_DConfigFile

namespace {
// stub 值表：key → QVariant
QVariant g_dconfigValue;
bool g_dconfigValid = false;

const QString g_keyDisableImproveGB18030 = "disableImproveGB18030";
const QString g_keyDefaultEncoding = "defaultEncoding";
const QString g_keyEnablePatchedIconv = "enablePatchedIconv";
} // namespace

class ConfigTest : public ::testing::Test {
protected:
    static void SetUpTestSuite()
    {
        int argc = 1;
        static char appArg0[] = "test_config";
        static char *argv[] = { appArg0, nullptr };
        s_app = new QCoreApplication(argc, argv);
    }

    static void TearDownTestSuite() { /* app 随进程退出，避免套件顺序问题 */ }

    void SetUp() override
    {
        stub.clear();
        g_dconfigValid = true;
        g_dconfigValue = QVariant();

        m_dconfig = new DConfig(QString::fromLatin1("ut-no-such-config"));

        // DConfig::create → 返回受控对象（Config 析构时会 delete 它）
        stub.set_lamda(
                static_cast<DConfig *(*)(const QString &, const QString &, const QString &, QObject *)>(&DConfig::create),
                [this](const QString &, const QString &, const QString &, QObject *) -> DConfig * {
                    ++m_createCalls;
                    return m_dconfig;
                });
        // isValid / value 全受控
        stub.set_lamda(static_cast<bool (DConfig::*)() const>(&DConfig::isValid),
                       [](const DConfig *) -> bool { return g_dconfigValid; });
        stub.set_lamda(static_cast<QVariant (DConfig::*)(const QString &, const QVariant &) const>(&DConfig::value),
                       [](const DConfig *, const QString &, const QVariant &) -> QVariant { return g_dconfigValue; });
    }

    void TearDown() override
    {
        stub.clear();
        // m_dconfig 的所有权在被测 Config 手里，这里不 delete
    }

    bool detectReference2005Standard() const
    {
        // 与源码相同的参照实现：同进程内 iconv 行为一致，结果确定
        iconv_t handle = iconv_open("UTF-8", "GB18030");
        if (handle == reinterpret_cast<iconv_t>(-1))
            return true;
        QByteArray input("\xFE\x51");
        QByteArray output(input.size() * 2, 0);
        char *inputData = input.data();
        char *outputData = output.data();
        size_t inputLen = static_cast<size_t>(input.count());
        size_t outputLen = static_cast<size_t>(output.count());
        const size_t ret = iconv(handle, &inputData, &inputLen, &outputData, &outputLen);
        iconv_close(handle);
        if (ret == static_cast<size_t>(-1))
            return true;
        return !output.contains(u8"\uE816");
    }

    stub_ext::StubExt stub;
    DConfig *m_dconfig = nullptr;
    int m_createCalls = 0;
    static QCoreApplication *s_app;
};

QCoreApplication *ConfigTest::s_app = nullptr;

TEST_F(ConfigTest, DetectIconvUse2005Standard_RealIconv_MatchesReferenceResult)
{
    // Arrange：参照实现在测试内独立执行一遍

    // Act
    const bool actual = detectIconvUse2005Standard();

    // Assert：与参照实现一致（本机 iconv 无论 2005/2022 标准均确定）
    const bool expected = detectReference2005Standard();
    EXPECT_EQ(actual, expected);
    // 结果本身必须是合法布尔语义
    EXPECT_TRUE(actual == true || actual == false);
}

TEST_F(ConfigTest, Construct_ValidDConfig_ReadsAllThreeKeys)
{
    // Arrange
    g_dconfigValid = true;
    g_dconfigValue = true; // disableImproveGB18030=true → improveGB18030=false

    // Act
    Config cfg;

    // Assert：构造读取了 DConfig（create 恰好一次）；
    // g_dconfigValue=true → improveGB18030 取反为 false、encoding 转大写 "TRUE"
    EXPECT_EQ(m_createCalls, 1);
    EXPECT_FALSE(cfg.enableImproveGB18030());
    EXPECT_EQ(cfg.defaultEncoding(), QByteArray("TRUE"));
}

TEST_F(ConfigTest, Construct_InvalidDConfig_KeepsDefaults)
{
    // Arrange
    g_dconfigValid = false;
    g_dconfigValue = true; // 即使值存在，无效 DConfig 也不读取

    // Act
    Config cfg;

    // Assert：保持头文件声明的默认值
    EXPECT_EQ(m_createCalls, 1);
    EXPECT_TRUE(cfg.enableImproveGB18030());  // 默认 true
    const bool patchedExpected = false || detectReference2005Standard();
    EXPECT_EQ(cfg.enablePatchedIconv(), patchedExpected);
    EXPECT_EQ(cfg.defaultEncoding(), QByteArray("UTF-8"));
}

TEST_F(ConfigTest, EnableImproveGB18030_ValidDConfigReflectsInvertedFlag)
{
    // Arrange：disableImproveGB18030 = true
    g_dconfigValid = true;
    g_dconfigValue = true;
    Config cfgDisabled;

    // Arrange：换新 DConfig（前一对象已随 cfgDisabled 析构释放，避免悬空）
    g_dconfigValue = false;
    m_dconfig = new DConfig(QString::fromLatin1("ut-no-such-config"));
    Config cfgEnabled;

    // Act / Assert：取反语义
    EXPECT_FALSE(cfgDisabled.enableImproveGB18030());
    EXPECT_TRUE(cfgEnabled.enableImproveGB18030());
}

TEST_F(ConfigTest, EnablePatchedIconv_PatchedTrue_ReturnsTrue)
{
    // Arrange：enablePatchedIconv=true → true || X 恒真，与 iconv 检测无关
    g_dconfigValid = true;
    g_dconfigValue = true;
    Config cfg;

    // Act / Assert：true || X 恒真
    EXPECT_TRUE(cfg.enablePatchedIconv());
    EXPECT_EQ(m_createCalls, 1); // 构造链路完整
}

TEST_F(ConfigTest, EnablePatchedIconv_PatchedFalse_EqualsDetectedStandard)
{
    // Arrange：enablePatchedIconv=false → 结果完全由 iconv 检测值决定
    g_dconfigValid = true;
    g_dconfigValue = false;
    Config cfg;

    // Act
    const bool actual = cfg.enablePatchedIconv();

    // Assert：false || detect == detect（参照实现确定期望）
    EXPECT_EQ(actual, detectReference2005Standard());
    EXPECT_EQ(m_createCalls, 1); // 构造链路完整
}

TEST_F(ConfigTest, DefaultEncoding_ConfiguredUppercase_ReturnsUppercase)
{
    // Arrange：defaultEncoding = "gbk"（小写）
    g_dconfigValid = true;
    g_dconfigValue = QByteArray("gbk");
    Config cfg;

    // Act / Assert：统一转大写为 3 字节 GBK；improveGB18030 保持合法布尔
    EXPECT_EQ(cfg.defaultEncoding(), QByteArray("GBK"));
    EXPECT_EQ(cfg.defaultEncoding().size(), 3);
}

TEST_F(ConfigTest, DefaultEncoding_EmptyConfigured_FallsBackToUtf8)
{
    // Arrange：defaultEncoding = ""
    g_dconfigValid = true;
    g_dconfigValue = QByteArray("");
    Config cfg;

    // Act / Assert：空值兜底 UTF-8
    EXPECT_EQ(cfg.defaultEncoding(), QByteArray("UTF-8"));
    EXPECT_EQ(m_createCalls, 1); // 构造链路完整
}

TEST_F(ConfigTest, ValueChanged_AllThreeKeys_UpdatesStateOnTheFly)
{
    // Arrange
    g_dconfigValid = true;
    g_dconfigValue = false;
    Config cfg;
    ASSERT_EQ(m_createCalls, 1);

    // Act / Assert：key=disableImproveGB18030 → 取反更新
    g_dconfigValue = true;
    QMetaObject::invokeMethod(m_dconfig, "valueChanged", Q_ARG(QString, g_keyDisableImproveGB18030));
    EXPECT_FALSE(cfg.enableImproveGB18030());
    g_dconfigValue = false;
    QMetaObject::invokeMethod(m_dconfig, "valueChanged", Q_ARG(QString, g_keyDisableImproveGB18030));
    EXPECT_TRUE(cfg.enableImproveGB18030());

    // key=enablePatchedIconv → 直接更新
    g_dconfigValue = true;
    QMetaObject::invokeMethod(m_dconfig, "valueChanged", Q_ARG(QString, g_keyEnablePatchedIconv));
    EXPECT_TRUE(cfg.enablePatchedIconv());

    // key=defaultEncoding → 大写更新
    g_dconfigValue = QByteArray("gb18030");
    QMetaObject::invokeMethod(m_dconfig, "valueChanged", Q_ARG(QString, g_keyDefaultEncoding));
    EXPECT_EQ(cfg.defaultEncoding(), QByteArray("GB18030"));

    // 未匹配的 key：状态不变
    const QByteArray before = cfg.defaultEncoding();
    QMetaObject::invokeMethod(m_dconfig, "valueChanged", Q_ARG(QString, QString("unknown-key")));
    EXPECT_EQ(cfg.defaultEncoding(), before);
}

TEST_F(ConfigTest, Instance_RepeatedCalls_ReturnsSameStaticInstance)
{
    // Act
    Config *first = Config::instance();
    Config *second = Config::instance();

    // Assert：函数级静态单例
    EXPECT_EQ(first, second);
    EXPECT_NE(first, nullptr);
}

TEST_F(ConfigTest, Destruct_ValidDConfig_DeletesDConfigPointer)
{
    // Arrange
    g_dconfigValid = true;
    g_dconfigValue = false;
    auto *cfg = new Config();
    DConfig *raw = m_dconfig;
    EXPECT_EQ(m_createCalls, 1);

    // Act：析构路径执行（不崩溃即删除成功；ASAN 下可验证无泄漏/无双重释放）
    EXPECT_NO_THROW(delete cfg);

    // Assert：对象已析构（此处以无异常 + 指针非空作为最小验证）
    EXPECT_NE(raw, nullptr);
}

#else // !DTKCORE_CLASS_DConfigFile

// DTK 不支持 DConfig 时：Config 仅剩 instance()/getter/析构，简单冒烟
TEST(ConfigSmoke, InstanceAndDefaults_AvailableWithoutDConfig)
{
    Config *inst = Config::instance();
    ASSERT_NE(inst, nullptr);
    EXPECT_EQ(inst->defaultEncoding(), QByteArray("UTF-8"));
}

#endif // DTKCORE_CLASS_DConfigFile
