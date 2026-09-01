// SPDX-FileCopyrightText: 2026 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later
//
// 覆盖 EditorApplication::~EditorApplication() (D0/D2 析构变体).
//
// 文件名以 "zz_" 前缀使 GLOB 排序在 widgets/ 之后，确保析构测试在整个
// 测试套件最末尾执行。delete 触发 D0→D2，虽然会销毁 qApp，但此时所有
// 其它测试已执行完毕。

#include "../../src/editorapplication.h"
#include "../../src/startmanager.h"
#include "src/stub.h"
#include <gtest/gtest.h>

namespace exit_destructor_stub {
StartManager *startManagerInstanceStub() { return nullptr; }
} // namespace exit_destructor_stub

using namespace exit_destructor_stub;

// delete triggers D0 (deleting destructor) which internally calls D2 (complete destructor).
// StartManager::instance() is stubbed to nullptr so the else branch runs.
// This test intentionally runs last — deleting qApp makes subsequent QWidget usage unsafe.
TEST(UT_EditorApplication_Destructor, Destructor_Delete)
{
    int argc = 1;
    char *argv[] = {"test"};
    EditorApplication *app = new EditorApplication(argc, argv);

    Stub s;
    s.set(ADDR(StartManager, instance), startManagerInstanceStub);

    delete app; // triggers D0 -> D2
    SUCCEED();
}
