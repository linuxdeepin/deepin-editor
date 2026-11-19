// SPDX-FileCopyrightText: 2026 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

// 回归用例：PMS BUG#383519【安全专项】【模糊测试】deepin-editor 存在 4 个安全问题。
// 与安全团队 fuzz 用例一一对应：
//   fuzz_tabbar.cpp/tabbar_update_tab_fuzz()            -> Tabbar::updateTab() 越界写 (CWE-787)
//   fuzz_themelistmodel.cpp/themelistmodel_data_fuzz()  -> ThemeListModel::data() 越界读 (CWE-125)
//   fuzz_tabbar.cpp/tabbar_tab_size_hint_fuzz()         -> Tabbar::tabSizeHint() 除零 (CWE-369)
//   fuzz_tabbar.cpp/tabbar_handle_tab_is_removed_fuzz() -> Tabbar::handleTabIsRemoved() 越界读 (CWE-125)

#include <gtest/gtest.h>
#include "../src/controls/tabbar.h"
#include "../src/thememodule/themelistmodel.h"
#include "../src/widgets/window.h"
#include "src/stub.h"

namespace fuzz383519 {

void removeWrapper_stub()
{
}

}

using namespace fuzz383519;

// 用例1：updateTab() 对越界 index 直接写 m_tabPaths[index]/m_tabTruePaths[index]（CWE-787 越界写）
TEST(UT_Tabbar_Fuzz383519, updateTab_outOfRange)
{
    Tabbar *tab = new Tabbar;

    // 空 tabbar 上传越界 index：负值、超出尾部、超大值
    tab->updateTab(-1, "/tmp/fuzz383519.txt", "fuzz");
    tab->updateTab(3, "/tmp/fuzz383519.txt", "fuzz");
    tab->updateTab(100000, "/tmp/fuzz383519.txt", "fuzz");
    EXPECT_EQ(0, tab->count());
    EXPECT_TRUE(tab->m_tabPaths.isEmpty());
    EXPECT_TRUE(tab->m_tabTruePaths.isEmpty());

    // 正向对照：合法 index 行为不变
    tab->addTab("/tmp/fuzz383519.txt", "fuzz");
    tab->updateTab(0, "/tmp/fuzz383519_other.txt", "fuzz2");
    EXPECT_EQ(1, tab->count());
    EXPECT_EQ(QString("/tmp/fuzz383519_other.txt"), tab->m_tabPaths.value(0));

    tab->deleteLater();
}

// 用例2：data() 对越界 row 直接 m_themes.at(r)（CWE-125 越界读）
TEST(UT_Tabbar_Fuzz383519, themelistmodelData_outOfRange)
{
    ThemeListModel model;
    const int rows = model.rowCount(QModelIndex());

    // 无效 QModelIndex（row == -1）
    EXPECT_FALSE(model.data(QModelIndex(), ThemeListModel::ThemeName).isValid());
    // row 越过尾部：QAbstractListModel::index() 对越界行返回无效索引
    EXPECT_FALSE(model.data(model.index(rows, 0), ThemeListModel::ThemeName).isValid());
    EXPECT_FALSE(model.data(model.index(rows + 3, 0), ThemeListModel::ThemePath).isValid());
    EXPECT_FALSE(model.data(model.index(rows + 100000, 0), ThemeListModel::ThemeName).isValid());

    // 正向对照：合法行返回主题名（环境未安装主题文件时跳过）
    if (rows > 0) {
        EXPECT_TRUE(model.data(model.index(0, 0), ThemeListModel::ThemeName).isValid());
    }
}

// 用例3：tabSizeHint() 在 count()==0 时 total / count() 整数除零（CWE-369）
TEST(UT_Tabbar_Fuzz383519, tabSizeHint_divideByZero)
{
    Tabbar *tab = new Tabbar;

    const QSize size = tab->tabSizeHint(0);
    EXPECT_GE(size.width(), 110);
    EXPECT_LE(size.width(), 160);

    tab->deleteLater();
}

// 用例4：handleTabIsRemoved() 对越界 index 直接 m_tabPaths.at(index)（CWE-125 越界读）
TEST(UT_Tabbar_Fuzz383519, handleTabIsRemoved_outOfRange)
{
    // 打桩隔离 Window::removeWrapper 依赖，聚焦越界防护
    Stub stub;
    stub.set(ADDR(Window, removeWrapper), removeWrapper_stub);

    Tabbar *tab = new Tabbar;

    // 空 tabbar 上移除：index 0 与越界值
    tab->handleTabIsRemoved(0);
    tab->handleTabIsRemoved(3);
    EXPECT_EQ(0, tab->count());
    EXPECT_TRUE(tab->m_tabPaths.isEmpty());

    // 正向对照：走真实移除信号路径（removeTab -> tabIsRemoved -> handleTabIsRemoved）
    tab->addTab("/tmp/fuzz383519.txt", "fuzz");
    EXPECT_EQ(1, tab->count());
    tab->removeTab(0);
    EXPECT_EQ(0, tab->count());
    EXPECT_TRUE(tab->m_tabPaths.isEmpty());

    tab->deleteLater();
}
