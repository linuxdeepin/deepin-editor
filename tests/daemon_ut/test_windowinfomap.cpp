// SPDX-FileCopyrightText: 2026 UnionTech Software Technology Co., Ltd.
// SPDX-License-Identifier: GPL-3.0-or-later
// WindowInfo / WindowInfoMap（src/dbus/types/windowinfomap.*）与
// WindowList（src/dbus/types/windowlist.*）单元测试
//
// 方法与分支清单：
// WindowInfo::operator==(rhs)
//   B1: attention 相等 && title 相等 → true
//   B2: title 不等                    → false
//   B3: attention 不等                → false
// WindowInfo::operator<<(QDebug)      → 输出 '(' title ',' attention ')'
// WindowInfo::operator<<(QDBusArgument)  → 结构体编组，签名为 "(sb)"
// WindowInfo::operator>>(QDBusArgument)  → 从流中提取 title + attention（B4）
// registerWindowInfoMetaType / registerWindowInfoMapMetaType → Qt / D-Bus 元类型登记
// registerWindowListMetaType            → Qt / D-Bus 元类型登记
//
// 用例映射：
// - Equality_Parameters_ExpectMatchingResult /* TEST_P */  → B1/B2/B3
// - DebugOperator_IncludesTitleAndAttention                  → operator<<(QDebug)
// - Marshal_ProjectType_ProducesStructSignature             → operator<<（签名 "(sb)"）
// - Demarshal_FedPrimitives_ExtractsFields                  → B4
// - RegisterWindowInfoMetaType_RegistersQtAndDBusTypes      → 登记效果
// - RegisterWindowInfoMapMetaType_RegistersQtAndDBusTypes   → 登记效果（含嵌套类型）
// - RegisterWindowListMetaType_RegistersQtAndDBusTypes      → 登记效果
//
// stub 说明（仅 Demarshal 用例，SetUp 设置 / TearDown clear）：
// QDBusArgument 的读模式对象只能由真实总线报文构造（本地写模式对象不可切换方向，
// QDBusArgument.cpp checkRead 明确拒绝），故 Demarshal 用例对 Qt 原语抽取点
// （operator>>(QString&)/operator>>(bool&) 与 const begin/endStructure）做地址级替换、
// 按序回灌字段值——被测 operator>> 函数体为真实执行，等价于总线侧喂数据。
// 环境隔离：无文件/网络/子进程/时间依赖。

#include <gtest/gtest.h>

#include <QDBusArgument>
#include <QDebug>
#include <QString>
#include <QVariant>

#include "stubext.h"
#include "types/windowinfomap.h"
#include "types/windowlist.h"

namespace {

struct EqualityCase {
    WindowInfo lhs;
    WindowInfo rhs;
    bool expected;
    const char *label;
};

class WindowInfoEqualityTest : public ::testing::TestWithParam<EqualityCase> {
protected:
    void SetUp() override { }
    void TearDown() override { }
};

// B1/B2/B3：相等与两类不等维度
TEST_P(WindowInfoEqualityTest, Equality_Parameters_ExpectMatchingResult)
{
    const EqualityCase &c = GetParam();

    // Act
    bool actual = (c.lhs == c.rhs);

    // Assert
    EXPECT_EQ(actual, c.expected) << "branch: " << c.label;
    EXPECT_EQ(c.rhs.title == c.lhs.title && c.rhs.attention == c.lhs.attention, c.expected)
        << "字段比对与 operator== 语义一致: " << c.label;
}

INSTANTIATE_TEST_SUITE_P(EqualityCases, WindowInfoEqualityTest,
    ::testing::Values(
        EqualityCase{{true, "same"}, {true, "same"}, true, "B1: all fields equal"},
        EqualityCase{{false, "a"}, {false, "b"}, false, "B2: title differs"},
        EqualityCase{{true, "a"}, {false, "a"}, false, "B3: attention differs"},
        EqualityCase{{false, ""}, {false, ""}, true, "B1 boundary: empty titles equal"}));

// operator<<(QDebug)：输出包含 title 与 attention
TEST(WindowInfoTest, DebugOperator_IncludesTitleAndAttention)
{
    // Arrange
    WindowInfo info;
    info.title = QString::fromUtf8("窗口-α");
    info.attention = true;

    // Act
    QString captured;
    {
        QDebug dbg(&captured);
        dbg << info;
    }

    // Assert
    EXPECT_TRUE(captured.contains(QString::fromUtf8("窗口-α")));  // title 出现
    EXPECT_TRUE(captured.contains("true"));                       // attention 出现（QDebug bool 文本）
}

// operator<<(QDBusArgument)：结构体编组签名 "(sb)"
TEST(WindowInfoTest, Marshal_ProjectType_ProducesStructSignature)
{
    // Arrange
    WindowInfo info;
    info.title = "marshal-title";
    info.attention = false;

    // Act
    QDBusArgument arg;
    arg << info;

    // Assert
    EXPECT_EQ(arg.currentSignature(), QString("(sb)"));  // 期望：string+bool 结构体
    EXPECT_FALSE(arg.currentSignature().isEmpty());      // 编组未失败
}

// Demarshal 夹具：替换 Qt 原语抽取点，按序回灌
class WindowInfoDemarshalTest : public ::testing::Test {
protected:
    void SetUp() override
    {
        stub.clear();
        fedStrings.clear();
        fedBools.clear();

        stub.set_lamda(
            static_cast<void (QDBusArgument::*)() const>(&QDBusArgument::beginStructure),
            [](const QDBusArgument *) { __DBG_STUB_INVOKE__ });
        stub.set_lamda(
            static_cast<void (QDBusArgument::*)() const>(&QDBusArgument::endStructure),
            [](const QDBusArgument *) { __DBG_STUB_INVOKE__ });
        stub.set_lamda(
            static_cast<const QDBusArgument &(QDBusArgument::*)(QString &) const>(&QDBusArgument::operator>>),
            [this](const QDBusArgument *self, QString &out) -> const QDBusArgument & {
                __DBG_STUB_INVOKE__
                out = fedStrings.isEmpty() ? QString() : fedStrings.takeFirst();
                return *self;
            });
        stub.set_lamda(
            static_cast<const QDBusArgument &(QDBusArgument::*)(bool &) const>(&QDBusArgument::operator>>),
            [this](const QDBusArgument *self, bool &out) -> const QDBusArgument & {
                __DBG_STUB_INVOKE__
                out = fedBools.isEmpty() ? false : fedBools.takeFirst();
                return *self;
            });
    }

    void TearDown() override { stub.clear(); }

    stub_ext::StubExt stub;
    QStringList fedStrings;
    QList<bool> fedBools;
};

// B4：Demarshal 提取 title + attention
TEST_F(WindowInfoDemarshalTest, Demarshal_FedPrimitives_ExtractsFields)
{
    // Arrange
    fedStrings << QString::fromUtf8("总线上来的标题");
    fedBools << true;
    QDBusArgument wire;
    WindowInfo out;
    out.title = "stale";
    out.attention = false;

    // Act
    wire >> out;

    // Assert
    EXPECT_EQ(out.title, QString::fromUtf8("总线上来的标题"));  // title 提取
    EXPECT_TRUE(out.attention);                                 // attention 提取
}

// registerWindowInfoMetaType：Qt 元类型登记生效
TEST(WindowInfoTest, RegisterWindowInfoMetaType_RegistersQtAndDBusTypes)
{
    // Act
    registerWindowInfoMetaType();

    // Assert
    EXPECT_NE(QMetaType::fromName("WindowInfo").id(), QMetaType::UnknownType);  // Qt 侧已登记
    QVariant boxed = QVariant::fromValue(WindowInfo{true, "boxed"});
    EXPECT_EQ(boxed.metaType().name(), QByteArray("WindowInfo"));               // 可装入 QVariant
}

// registerWindowInfoMapMetaType：嵌套登记（Info + Map）
TEST(WindowInfoTest, RegisterWindowInfoMapMetaType_RegistersQtAndDBusTypes)
{
    // Arrange
    WindowInfoMap sample;
    sample.insert(1u, WindowInfo{false, "one"});

    // Act
    registerWindowInfoMapMetaType();

    // Assert
    EXPECT_NE(QMetaType::fromName("WindowInfoMap").id(), QMetaType::UnknownType);  // Map 已登记
    QDBusArgument arg;
    arg << sample;
    EXPECT_EQ(arg.currentSignature(), QString("a{u(sb)}"));  // D-Bus 侧签名可用（登记链完整）
}

// registerWindowListMetaType：QList<quint32> 别名登记（Qt 内建类型经别名注册，DBus 数组可用）
TEST(WindowListTest, RegisterWindowListMetaType_RegistersQtAndDBusTypes)
{
    // Arrange
    WindowList sample{1u, 2u, 3u};

    // Act
    registerWindowListMetaType();

    // Assert
    // WindowList 为 QList<quint32> 透明别名（qRegisterMetaType 归一到内建类型名）
    EXPECT_STREQ(QMetaType::fromType<WindowList>().name(), "QList<uint>");
    EXPECT_NE(qMetaTypeId<WindowList>(), QMetaType::UnknownType);  // Q_DECLARE_METATYPE 登记生效
    QVariant boxed = QVariant::fromValue(sample);
    EXPECT_EQ(boxed.value<WindowList>(), sample);         // QVariant 往返保持内容
    QDBusArgument arg;
    arg << sample;
    EXPECT_EQ(arg.currentSignature(), QString("au"));     // D-Bus 侧 uint 数组签名可用
}

} // namespace
