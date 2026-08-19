// SPDX-FileCopyrightText: 2026 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include "ut_viewmodefsm.h"
#include "viewmodefsm.h"

UT_ViewModeFsm::UT_ViewModeFsm()
{
}

// 打开 .md → 默认 LivePreview
TEST_F(UT_ViewModeFsm, DefaultMode_MarkdownFile_LivePreview)
{
    EXPECT_EQ(ViewModeFsm::resolveDefaultMode(true), ViewMode::LivePreview);
}

// 打开非 md → 默认 Edit
TEST_F(UT_ViewModeFsm, DefaultMode_NonMarkdownFile_Edit)
{
    EXPECT_EQ(ViewModeFsm::resolveDefaultMode(false), ViewMode::Edit);
}

// 「实时预览」入口可用性：仅 md 文件可用
TEST_F(UT_ViewModeFsm, LivePreviewAvailable_Markdown_True)
{
    EXPECT_TRUE(ViewModeFsm::isLivePreviewAvailable(true));
}
TEST_F(UT_ViewModeFsm, LivePreviewAvailable_NonMarkdown_False)
{
    EXPECT_FALSE(ViewModeFsm::isLivePreviewAvailable(false));
}

// 「编辑模式」「查看视图」始终可用（需求：入口始终显示，非 md 仅 LivePreview 置灰）
TEST_F(UT_ViewModeFsm, EditViewAvailable_AlwaysTrue)
{
    EXPECT_TRUE(ViewModeFsm::isEditViewAvailable(true));
    EXPECT_TRUE(ViewModeFsm::isEditViewAvailable(false));
}
TEST_F(UT_ViewModeFsm, ReadViewAvailable_AlwaysTrue)
{
    EXPECT_TRUE(ViewModeFsm::isReadViewAvailable(true));
    EXPECT_TRUE(ViewModeFsm::isReadViewAvailable(false));
}

// 切换合法性：md 文件可切到任意三个视图
TEST_F(UT_ViewModeFsm, CanSwitch_Markdown_AllThreeViews)
{
    EXPECT_TRUE(ViewModeFsm::canSwitchTo(ViewMode::Edit, true));
    EXPECT_TRUE(ViewModeFsm::canSwitchTo(ViewMode::ReadView, true));
    EXPECT_TRUE(ViewModeFsm::canSwitchTo(ViewMode::LivePreview, true));
}

// 切换合法性：非 md 文件不能切到 LivePreview
TEST_F(UT_ViewModeFsm, CanSwitch_NonMarkdown_LivePreviewFalse)
{
    EXPECT_FALSE(ViewModeFsm::canSwitchTo(ViewMode::LivePreview, false));
}

// 切换合法性：非 md 文件可切到 Edit / ReadView
TEST_F(UT_ViewModeFsm, CanSwitch_NonMarkdown_EditAndReadViewTrue)
{
    EXPECT_TRUE(ViewModeFsm::canSwitchTo(ViewMode::Edit, false));
    EXPECT_TRUE(ViewModeFsm::canSwitchTo(ViewMode::ReadView, false));
}

// Wysiwyg 阶段二不可达（本期 ReadOnly）
TEST_F(UT_ViewModeFsm, CanSwitch_Wysiwyg_AlwaysFalse)
{
    EXPECT_FALSE(ViewModeFsm::canSwitchTo(ViewMode::Wysiwyg, true));
    EXPECT_FALSE(ViewModeFsm::canSwitchTo(ViewMode::Wysiwyg, false));
}

// 语言切走导致的回退：非 md 在 LivePreview 时退回 Edit
TEST_F(UT_ViewModeFsm, Fallback_NonMarkdownFromLivePreview_Edit)
{
    EXPECT_EQ(ViewModeFsm::fallbackWhenMarkdownLost(ViewMode::LivePreview), ViewMode::Edit);
}

// 语言切走：非 md 在 ReadView 时保持 ReadView（查看视图对非 md 有效，纯文本只读）
TEST_F(UT_ViewModeFsm, Fallback_NonMarkdownFromReadView_StayReadView)
{
    EXPECT_EQ(ViewModeFsm::fallbackWhenMarkdownLost(ViewMode::ReadView), ViewMode::ReadView);
}

// 语言切走：在 Edit 时保持 Edit
TEST_F(UT_ViewModeFsm, Fallback_FromEdit_StayEdit)
{
    EXPECT_EQ(ViewModeFsm::fallbackWhenMarkdownLost(ViewMode::Edit), ViewMode::Edit);
}

// 语言切到（非 md → md）的跃迁：Edit → LivePreview（对齐 md 默认视图，2026-08-19）
TEST_F(UT_ViewModeFsm, Elevate_MarkdownGainedFromEdit_LivePreview)
{
    EXPECT_EQ(ViewModeFsm::elevateWhenMarkdownGained(ViewMode::Edit), ViewMode::LivePreview);
}

// 语言切到：ReadView / LivePreview 保持（ReadView 仅切换实现，模式不变）
TEST_F(UT_ViewModeFsm, Elevate_MarkdownGainedFromReadOrLive_Stay)
{
    EXPECT_EQ(ViewModeFsm::elevateWhenMarkdownGained(ViewMode::ReadView), ViewMode::ReadView);
    EXPECT_EQ(ViewModeFsm::elevateWhenMarkdownGained(ViewMode::LivePreview), ViewMode::LivePreview);
}

// 「查看视图」在非 md 文件下走纯文本只读（非 Milkdown 渲染）
TEST_F(UT_ViewModeFsm, ReadOnlyTextMode_ReadViewNonMarkdown_True)
{
    EXPECT_TRUE(ViewModeFsm::isReadOnlyTextMode(ViewMode::ReadView, false));
}

// 「查看视图」在 md 文件下走 Milkdown 渲染
TEST_F(UT_ViewModeFsm, ReadOnlyTextMode_ReadViewMarkdown_False)
{
    EXPECT_FALSE(ViewModeFsm::isReadOnlyTextMode(ViewMode::ReadView, true));
}

// 其他视图不是纯文本只读
TEST_F(UT_ViewModeFsm, ReadOnlyTextMode_EditOrLive_False)
{
    EXPECT_FALSE(ViewModeFsm::isReadOnlyTextMode(ViewMode::Edit, true));
    EXPECT_FALSE(ViewModeFsm::isReadOnlyTextMode(ViewMode::LivePreview, true));
}
