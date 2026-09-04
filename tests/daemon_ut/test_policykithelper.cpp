// SPDX-FileCopyrightText: 2026 UnionTech Software Technology Co., Ltd.
// SPDX-License-Identifier: GPL-3.0-or-later

// PolicyKitHelper（daemon/src/policykithelper.h）单元测试
//
// 方法与分支清单：
// PolicyKitHelper::instance()                       → 单例指针（inline static，头文件实现）
// PolicyKitHelper::checkAuthorization(actionId, pid)
//   调用链：Authority::instance()->checkAuthorizationSync(actionId, UnixProcessSubject(pid),
//           Authority::AllowUserInteraction) → result == Authority::Yes ? true : false
//   B1: result == Yes      → return true
//   B2: result == No       → return false
//   B3: result == Unknown  → return false
//   B4: result == Challenge→ return false
//   另验证：actionId 原样透传、flags == AllowUserInteraction、恰好调用一次
//
// 用例映射：
// - Instance_Singleton_ReturnsSameNonNullPointer                        → instance()
// - CheckAuthorization_ResultMapping_ReturnsExpected /* TEST_P */        → B1/B2/B3/B4
// - CheckAuthorization_ForwardsActionIdAndInteractionFlag_ReturnsTrue    → 透传 + B1
//
// stub 说明（stub_ext，SetUp 设置 / TearDown clear）：
// - PolkitQt1::Authority::instance → 返回 nullptr：阻止 polkit 单例真实构造（其构造会
//   经 GDBus 连接 system bus），测试内零 polkit 运行时接触；
// - PolkitQt1::Authority::checkAuthorizationSync（非虚成员，地址级替换）→ 返回夹具预置
//   的 Result，并记录 actionId/flags/调用次数。
// 编译说明：daemon 源码包含 polkit-qt5-1 头，本环境经 shim 转发到 polkit-qt6-1（同
// PolkitQt1 命名空间），运行期行为全部被 stub 拦截，与真实库实现无关。

#include <gtest/gtest.h>

#include <QString>

#include "policykithelper.h"
#include "stubext.h"

namespace {

struct ResultMappingCase {
    PolkitQt1::Authority::Result stubbedResult;  // Authority 返回值
    bool expected;                               // checkAuthorization 期望返回值
    const char *label;                           // 分支标识
};

class PolicyKitHelperCheckAuthorizationTest : public ::testing::TestWithParam<ResultMappingCase> {
protected:
    void SetUp() override
    {
        stub.clear();
        syncCalls = 0;
        lastActionId.clear();
        lastFlags = PolkitQt1::Authority::AuthorizationFlags();

        // 阻止真实 polkit 单例构造（避免接触 system bus）
        stub.set_lamda(&PolkitQt1::Authority::instance,
                       [](::PolkitAuthority *) -> PolkitQt1::Authority * {
                           __DBG_STUB_INVOKE__
                           return nullptr;
                       });

        stub.set_lamda(&PolkitQt1::Authority::checkAuthorizationSync,
                       [this](PolkitQt1::Authority *, const QString &actionId,
                              const PolkitQt1::Subject &,
                              PolkitQt1::Authority::AuthorizationFlags flags)
                           -> PolkitQt1::Authority::Result {
                           __DBG_STUB_INVOKE__
                           ++syncCalls;
                           lastActionId = actionId;
                           lastFlags = flags;
                           return GetParam().stubbedResult;
                       });
    }

    void TearDown() override { stub.clear(); }

    stub_ext::StubExt stub;
    int syncCalls = 0;
    QString lastActionId;
    PolkitQt1::Authority::AuthorizationFlags lastFlags;  // 默认 None
};

// B1/B2/B3/B4：Authority::Result 四个取值到 bool 的完整映射
TEST_P(PolicyKitHelperCheckAuthorizationTest, CheckAuthorization_ResultMapping_ReturnsExpected)
{
    PolicyKitHelper *helper = PolicyKitHelper::instance();

    // Act
    bool ret = helper->checkAuthorization("com.deepin.editor.saveFile", 4242);

    // Assert
    EXPECT_EQ(ret, GetParam().expected) << "branch: " << GetParam().label;
    EXPECT_EQ(syncCalls, 1) << "checkAuthorizationSync 恰好被调用一次";
}

INSTANTIATE_TEST_SUITE_P(ResultMappingCases, PolicyKitHelperCheckAuthorizationTest,
    ::testing::Values(
        ResultMappingCase{PolkitQt1::Authority::Yes, true, "B1: Yes -> true"},
        ResultMappingCase{PolkitQt1::Authority::No, false, "B2: No -> false"},
        ResultMappingCase{PolkitQt1::Authority::Unknown, false, "B3: Unknown -> false"},
        ResultMappingCase{PolkitQt1::Authority::Challenge, false, "B4: Challenge -> false"}));

// instance()：非空且两次调用同一指针（单例语义，覆盖 inline static 与私有构造）
TEST(PolicyKitHelperTest, Instance_Singleton_ReturnsSameNonNullPointer)
{
    PolicyKitHelper *first = PolicyKitHelper::instance();
    PolicyKitHelper *second = PolicyKitHelper::instance();

    EXPECT_NE(first, nullptr);       // 期望：单例指针非空
    EXPECT_EQ(first, second);        // 期望：两次获取同一实例
}

// 透传验证：actionId / AllowUserInteraction / 返回 true（B1 路径）
TEST(PolicyKitHelperTest, CheckAuthorization_ForwardsActionIdAndInteractionFlag_ReturnsTrue)
{
    stub_ext::StubExt stub;
    int syncCalls = 0;
    QString lastActionId;
    PolkitQt1::Authority::AuthorizationFlags lastFlags;  // 默认 None

    stub.set_lamda(&PolkitQt1::Authority::instance,
                   [](::PolkitAuthority *) -> PolkitQt1::Authority * {
                       __DBG_STUB_INVOKE__
                       return nullptr;
                   });
    stub.set_lamda(&PolkitQt1::Authority::checkAuthorizationSync,
                   [&](PolkitQt1::Authority *, const QString &actionId,
                       const PolkitQt1::Subject &,
                       PolkitQt1::Authority::AuthorizationFlags flags)
                       -> PolkitQt1::Authority::Result {
                       __DBG_STUB_INVOKE__
                       ++syncCalls;
                       lastActionId = actionId;
                       lastFlags = flags;
                       return PolkitQt1::Authority::Yes;
                   });

    PolicyKitHelper *helper = PolicyKitHelper::instance();

    // Act
    bool ret = helper->checkAuthorization("com.deepin.editor.saveFile", 12345);

    // Assert
    EXPECT_TRUE(ret);  // 期望：Authority::Yes → true（B1）
    EXPECT_EQ(lastActionId, QString("com.deepin.editor.saveFile"));  // actionId 原样透传
    EXPECT_EQ(syncCalls, 1);
    EXPECT_TRUE(lastFlags == PolkitQt1::Authority::AllowUserInteraction);  // 交互授权标志透传
}

} // namespace
