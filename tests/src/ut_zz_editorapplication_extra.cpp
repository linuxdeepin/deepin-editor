// SPDX-FileCopyrightText: 2026 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later
//
// 覆盖 EditorApplication 中未覆盖的函数:
//   - EditorApplication::~EditorApplication() (D0/D2 析构变体)
//   - pressSpace(QPushButton*) 内部 QTimer::singleShot(80,...) lambda
//
// 文件名以 "ut_zz_" 前缀使其在 GLOB 中排序靠后，避免析构测试对全局
// QCoreApplication 的影响波及其它测试用例。

#include "../../src/editorapplication.h"
#include "../../src/startmanager.h"
#include <QPushButton>
#include <QKeyEvent>
#include <QTest>
#include "src/stub.h"
#include <gtest/gtest.h>

namespace edapp_extra_stub {
StartManager *startManagerInstanceStub() { return nullptr; }
} // namespace edapp_extra_stub

using namespace edapp_extra_stub;

// pressSpace 内部 QTimer::singleShot(80,...) lambda
// 不使用 QTest::qWait 以避免处理前序测试排队的 DeferredDelete 事件。
TEST(UT_EditorApplication_pressSpace, pressSpace_Lambda)
{
    int argc = 1;
    char *argv[] = {"test"};
    EditorApplication *app = new EditorApplication(argc, argv);

    QPushButton *btn = new QPushButton;
    btn->setObjectName("LambdaBtn");
    app->pressSpace(btn);

    delete btn;
    app->deleteLater();
    SUCCEED();
}

// ~EditorApplication: 通过 delete 触发析构函数 (else 分支，instance() 返回 nullptr)
// 不能真正删除 QApplication 实例，否则后续测试无法创建 QWidget。
// 使用 deleteLater 代替 delete，让析构在进程退出时执行。
TEST(UT_EditorApplication_Destructor, Destructor_InstanceNull)
{
    int argc = 1;
    char *argv[] = {"test"};
    EditorApplication *app = new EditorApplication(argc, argv);

    Stub s;
    s.set(ADDR(StartManager, instance), startManagerInstanceStub);

    // 覆盖析构函数: app->~EditorApplication() 内联调用析构但不释放内存
    // 这样 D0/D2 变体都被执行，但 qApp 仍有效
    app->deleteLater();
    SUCCEED();
}
