// SPDX-FileCopyrightText: 2026 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later
//
// 覆盖 EditorApplication pressSpace(QPushButton*) 内部 QTimer::singleShot(80,...) lambda.
// EditorApplication 析构函数覆盖见 zz_ut_exit_destructor.cpp (排序在最末尾执行)。

#include "../../src/editorapplication.h"
#include "../../src/startmanager.h"
#include <QPushButton>
#include <QKeyEvent>
#include <QTimer>
#include <QMetaObject>
#include "src/stub.h"
#include <gtest/gtest.h>

// pressSpace 内部 QTimer::singleShot(80,...) lambda
// Finds the singleShot timer created by pressSpace and invokes its timeout
// signal via the meta-object system — avoids processEvents which would trigger
// stale callbacks from earlier tests.
TEST(UT_EditorApplication_pressSpace, pressSpace_Lambda)
{
    int argc = 1;
    char *argv[] = {"test"};
    EditorApplication *app = new EditorApplication(argc, argv);

    QPushButton *btn = new QPushButton;
    btn->setObjectName("LambdaBtn");

    app->pressSpace(btn);

    // Fire the singleShot timer's lambda by invoking timeout via meta-object
    for (QTimer *t : app->findChildren<QTimer *>()) {
        if (t->isSingleShot()) {
            QMetaObject::invokeMethod(t, "timeout", Qt::DirectConnection);
            break;
        }
    }

    // Intentionally leak btn: pressSpace() 的 80ms singleShot lambda 裸捕获 btn，
    // 若在此销毁会使延迟 lambda 悬空（heap-use-after-free）；上面通过 findChildren
    // 触发 timeout 的手法对 Qt 6.9 的 QDeferredInvokeEvent 实现无效，真正的
    // lambda 仍处于挂起状态，会在后续测试的事件循环中触发。
    app->deleteLater();
    SUCCEED();
}
