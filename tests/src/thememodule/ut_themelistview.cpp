// SPDX-FileCopyrightText: 2026 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include <gtest/gtest.h>
#include "../../src/thememodule/themelistview.h"
#include "../../src/thememodule/themelistmodel.h"

// adjustScrollbarMargins: 视图不可见，提前返回
TEST(UT_ThemeListView, adjustScrollbarMargins_NotVisible)
{
    ThemeListView view;
    view.adjustScrollbarMargins();
    SUCCEED();
}

// adjustScrollbarMargins: 视图可见，执行布局调整
// 不使用 QTest::qWait 以避免处理前序测试排队的 DeferredDelete 事件导致卡死。
TEST(UT_ThemeListView, adjustScrollbarMargins_Visible)
{
    ThemeListView view;
    view.setModel(new ThemeListModel(&view));
    view.resize(120, 200);
    view.show();
    view.adjustScrollbarMargins();
    view.hide();
    SUCCEED();
}
