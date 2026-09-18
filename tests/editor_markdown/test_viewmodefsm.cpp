// SPDX-FileCopyrightText: 2026 UnionTech Software Technology Co., Ltd.
// SPDX-License-Identifier: GPL-3.0-or-later

#include <gtest/gtest.h>
#include "stubext.h"
#include "viewmodefsm.h"

// 分支清单（来源：viewmodefsm.h）
// resolveDefaultMode:
//   A1: isMarkdown → LivePreview；A2: 非 → Edit
// isLivePreviewAvailable: L1: isMarkdown 直通
// isEditViewAvailable / isReadViewAvailable: 恒 true（入口始终显示）
// canSwitchTo switch:
//   C1: Edit → true；C2: ReadView → true；C3: LivePreview → isMarkdown；C4: Wysiwyg → false
// fallbackWhenMarkdownLost:
//   F1: LivePreview → Edit；F2: 其他保持
// elevateWhenMarkdownGained:
//   E1: Edit → LivePreview；E2: 其他保持
// isReadOnlyTextMode:
//   R1: ReadView && !isMarkdown → true；R2: 其他 → false
//
// 用例映射：
// - ResolveDefaultMode_MarkdownOrNot_ReturnsExpected（TEST_P 2 组，双色分支各一侧）→ A1/A2
// - IsLivePreviewAvailable_MarkdownOrNot_ReturnsIsMarkdown（TEST_P 2 组）           → L1
// - EditAndReadViewEntries_AnyFileKind_AlwaysAvailable（TEST_P 4 组）                → 恒真分支
// - CanSwitchTo_AllModeFileCombinations_ReturnsExpected（TEST_P 8 组）               → C1-C4 + default
// - FallbackWhenMarkdownLost_AllModes_ReturnsExpected（TEST_P 4 组）                 → F1/F2
// - ElevateWhenMarkdownGained_AllModes_ReturnsExpected（TEST_P 4 组）                → E1/E2
// - IsReadOnlyTextMode_ModeFileCombinations_ReturnsExpected（TEST_P 4 组）           → R1/R2
//
// 最小清单自检：1 每公开方法≥1用例 ✔（8 方法全覆盖）2 等价类（模式×是否md 全网格）✔
// 3 边界（Wysiwyg 预留态）✔ 4 TEST_P ×6（≥3 组）✔ 5 分支映射 ✔ 6 全分支 ✔
// 7 无异常 8 无效/预留模式负面 ✔ 9 纯函数 ✔ 10 无外部依赖 ✔

// —— A1/A2 ——
namespace {
struct BoolCase {
    bool isMarkdown;
};
const BoolCase kBoolCases[] = {
    { true },
    { false },
};
} // namespace

class DefaultModeParamTest : public ::testing::TestWithParam<BoolCase> {
};

TEST_P(DefaultModeParamTest, ResolveDefaultMode_MarkdownOrNot_ReturnsExpected)
{
    // Arrange
    const bool isMarkdown = GetParam().isMarkdown;

    // Act
    const ViewMode mode = ViewModeFsm::resolveDefaultMode(isMarkdown);

    // Assert
    EXPECT_EQ(mode, isMarkdown ? ViewMode::LivePreview : ViewMode::Edit);
    // 与置灰规则一致性：默认模式必为可切换目标（不变式）
    EXPECT_TRUE(ViewModeFsm::canSwitchTo(mode, isMarkdown));
}

INSTANTIATE_TEST_SUITE_P(DefaultModes, DefaultModeParamTest,
                         ::testing::ValuesIn(kBoolCases));

// —— L1 ——
class LivePreviewAvailParamTest : public ::testing::TestWithParam<BoolCase> {
};

TEST_P(LivePreviewAvailParamTest, IsLivePreviewAvailable_MarkdownOrNot_ReturnsIsMarkdown)
{
    // Arrange
    const bool isMarkdown = GetParam().isMarkdown;

    // Act
    const bool ret = ViewModeFsm::isLivePreviewAvailable(isMarkdown);

    // Assert：md 专属入口（非 md 置灰）
    EXPECT_EQ(ret, isMarkdown);
    if (isMarkdown)
        EXPECT_TRUE(ret);   // md → 可用边
    else
        EXPECT_FALSE(ret);  // 非 md → 置灰边
}

INSTANTIATE_TEST_SUITE_P(LivePreviewAvailability, LivePreviewAvailParamTest,
                         ::testing::ValuesIn(kBoolCases));

// —— 恒真入口 ——
namespace {
struct ModeBoolCase {
    ViewMode mode;
    bool isMarkdown;
};
const ModeBoolCase kEntryCases[] = {
    { ViewMode::Edit, true },
    { ViewMode::Edit, false },
    { ViewMode::ReadView, true },
    { ViewMode::ReadView, false },
};
} // namespace

class EntryAvailParamTest : public ::testing::TestWithParam<ModeBoolCase> {
};

TEST_P(EntryAvailParamTest, EditAndReadViewEntries_AnyFileKind_AlwaysAvailable)
{
    // Arrange
    const auto &c = GetParam();

    // Act
    const bool edit = ViewModeFsm::isEditViewAvailable(c.isMarkdown);
    const bool read = ViewModeFsm::isReadViewAvailable(c.isMarkdown);

    // Assert：入口始终显示（含新建 .txt），与文件类型无关
    EXPECT_TRUE(edit);
    EXPECT_TRUE(read);
}

INSTANTIATE_TEST_SUITE_P(AlwaysAvailableEntries, EntryAvailParamTest,
                         ::testing::ValuesIn(kEntryCases));

// —— C1-C4（模式 × 文件类型全网格）——
namespace {
struct SwitchCase {
    ViewMode target;
    bool isMarkdown;
    bool expected;
    const char *note;
};
const SwitchCase kSwitchCases[] = {
    { ViewMode::Edit, true, true, "任意文件可切编辑" },
    { ViewMode::Edit, false, true, "非 md 也可切编辑" },
    { ViewMode::ReadView, true, true, "md 查看走渲染" },
    { ViewMode::ReadView, false, true, "非 md 查看走纯文本只读" },
    { ViewMode::LivePreview, true, true, "仅 md 可切实时预览" },
    { ViewMode::LivePreview, false, false, "非 md 实时预览置灰" },
    { ViewMode::Wysiwyg, true, false, "阶段二预留不可达" },
    { ViewMode::Wysiwyg, false, false, "阶段二预留不可达（非 md）" },
};
} // namespace

class CanSwitchParamTest : public ::testing::TestWithParam<SwitchCase> {
};

TEST_P(CanSwitchParamTest, CanSwitchTo_AllModeFileCombinations_ReturnsExpected)
{
    // Arrange
    const auto &c = GetParam();

    // Act
    const bool ret = ViewModeFsm::canSwitchTo(c.target, c.isMarkdown);

    // Assert
    EXPECT_EQ(ret, c.expected);   // 期望值见参数表 note
    // 与置灰规则一致性（LivePreview 唯一受文件类型约束的入口）
    EXPECT_EQ(ret, c.target == ViewMode::LivePreview
                        ? c.isMarkdown
                        : c.target != ViewMode::Wysiwyg);
}

INSTANTIATE_TEST_SUITE_P(SwitchMatrix, CanSwitchParamTest,
                         ::testing::ValuesIn(kSwitchCases));

// —— F1/F2 ——
namespace {
struct TransitionCase {
    ViewMode current;
    ViewMode expected;
};
const TransitionCase kFallbackCases[] = {
    { ViewMode::LivePreview, ViewMode::Edit },   // 唯一回退
    { ViewMode::Edit, ViewMode::Edit },          // 保持
    { ViewMode::ReadView, ViewMode::ReadView },  // 保持（纯文本只读语义）
    { ViewMode::Wysiwyg, ViewMode::Wysiwyg },    // 保持
};
} // namespace

class FallbackParamTest : public ::testing::TestWithParam<TransitionCase> {
};

TEST_P(FallbackParamTest, FallbackWhenMarkdownLost_AllModes_ReturnsExpected)
{
    // Arrange
    const auto &c = GetParam();

    // Act
    const ViewMode out = ViewModeFsm::fallbackWhenMarkdownLost(c.current);

    // Assert
    EXPECT_EQ(out, c.expected);
    // 回退结果在非 md 下必须合法可切换（Wysiwyg 为阶段二预留态，按设计不可达，豁免）
    if (out != ViewMode::Wysiwyg)
        EXPECT_TRUE(ViewModeFsm::canSwitchTo(out, false));
}

INSTANTIATE_TEST_SUITE_P(MarkdownLost, FallbackParamTest,
                         ::testing::ValuesIn(kFallbackCases));

// —— E1/E2 ——
namespace {
const TransitionCase kElevateCases[] = {
    { ViewMode::Edit, ViewMode::LivePreview },   // 唯一跃迁
    { ViewMode::LivePreview, ViewMode::LivePreview },
    { ViewMode::ReadView, ViewMode::ReadView },
    { ViewMode::Wysiwyg, ViewMode::Wysiwyg },
};
} // namespace

class ElevateParamTest : public ::testing::TestWithParam<TransitionCase> {
};

TEST_P(ElevateParamTest, ElevateWhenMarkdownGained_AllModes_ReturnsExpected)
{
    // Arrange
    const auto &c = GetParam();

    // Act
    const ViewMode out = ViewModeFsm::elevateWhenMarkdownGained(c.current);

    // Assert
    EXPECT_EQ(out, c.expected);
    // md 下结果合法（Wysiwyg 阶段二预留态按设计不可达，豁免）
    if (out != ViewMode::Wysiwyg)
        EXPECT_TRUE(ViewModeFsm::canSwitchTo(out, true));
}

INSTANTIATE_TEST_SUITE_P(MarkdownGained, ElevateParamTest,
                         ::testing::ValuesIn(kElevateCases));

// —— R1/R2 ——
namespace {
struct ReadOnlyCase {
    ViewMode mode;
    bool isMarkdown;
    bool expected;
};
const ReadOnlyCase kReadOnlyCases[] = {
    { ViewMode::ReadView, false, true },   // 唯一纯文本只读组合
    { ViewMode::ReadView, true, false },   // md 查看走渲染页
    { ViewMode::Edit, false, false },
    { ViewMode::LivePreview, true, false },
};
} // namespace

class ReadOnlyParamTest : public ::testing::TestWithParam<ReadOnlyCase> {
};

TEST_P(ReadOnlyParamTest, IsReadOnlyTextMode_ModeFileCombinations_ReturnsExpected)
{
    // Arrange
    const auto &c = GetParam();

    // Act
    const bool ret = ViewModeFsm::isReadOnlyTextMode(c.mode, c.isMarkdown);

    // Assert
    EXPECT_EQ(ret, c.expected);
    EXPECT_EQ(ret, c.mode == ViewMode::ReadView && !c.isMarkdown);   // 公式独立复核
}

INSTANTIATE_TEST_SUITE_P(ReadOnlyTextModes, ReadOnlyParamTest,
                         ::testing::ValuesIn(kReadOnlyCases));
