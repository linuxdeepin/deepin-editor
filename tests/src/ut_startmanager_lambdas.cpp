// SPDX-FileCopyrightText: 2026 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later
//
// 覆盖 StartManager 中未覆盖的函数:
//   - StartManager::tr(char const*, char const*, int)
//   - createWindow(bool) 内部 dragStarted/dragEnd 信号 lambda
//   - openFilesInTab 内部 QTimer::singleShot(50,...) lambda
//   - slotCloseWindow 内部 QTimer::singleShot(1000,...) lambda

#include "../../src/startmanager.h"
#include "../../src/editor/dtextedit.h"
#include "../../src/common/settings.h"
#include "../../src/widgets/window.h"
#include "../../src/editor/editwrapper.h"
#include "../../src/controls/tabbar.h"
#include "src/stub.h"
#include <QDir>
#include <QObject>
#include <QMetaObject>
#include <QThread>
#include <gtest/gtest.h>

namespace sm_lambda_stub {

void returnstub() { return; }
bool returntruestub() { return true; }
QStringList entryliststub() { return {"123", "456"}; }

StartManager::FileTabInfo getFileTabInfoNotFoundStub()
{
    return StartManager::FileTabInfo{-1, -1};
}

int recoverFileStub(Window *) { return 0; }

} // namespace sm_lambda_stub

using namespace sm_lambda_stub;

// Blocks DeferredDelete events so qWait can fire timer lambdas without
// processing accumulated deleteLater calls from earlier tests.
class SM_DeferredDeleteBlocker : public QObject
{
public:
    bool eventFilter(QObject *, QEvent *e) override
    {
        return e->type() == QEvent::DeferredDelete;
    }
};

// 创建一个全新的 StartManager 单例。
// 既有测试对单例调用 deleteLater() 会积压 DeferredDelete 事件，在本文件 qWait 期间
// 被处理会导致 m_instance 悬空 (heap-use-after-free)。因此每个测试都重置 m_instance
// 以获取干净实例，且不调用 deleteLater。
static StartManager *freshStartManager()
{
    StartManager::m_instance = nullptr;
    return StartManager::instance();
}

// StartManager::tr 静态翻译函数
TEST(UT_StartManager_tr, tr)
{
    EXPECT_EQ(StartManager::tr("test"), QString("test"));
    EXPECT_EQ(StartManager::tr("test", nullptr, -1), QString("test"));
}

// createWindow(bool) 内部 dragStarted/dragEnd 信号 lambda
TEST(UT_StartManager_createWindow, createWindow_DragLambdas)
{
    StartManager *startManager = freshStartManager();
    startManager->m_windows.clear();
    startManager->m_bIsTagDragging = false;

    Window *window = startManager->createWindow(true);
    ASSERT_TRUE(window != nullptr);

    Tabbar *tabbar = window->getTabbar();
    ASSERT_TRUE(tabbar != nullptr);

    QMetaObject::invokeMethod(tabbar, "dragStarted", Qt::DirectConnection);
    EXPECT_TRUE(startManager->m_bIsTagDragging);

    QMetaObject::invokeMethod(tabbar, "dragEnd", Qt::DirectConnection,
                              Q_ARG(Qt::DropAction, Qt::IgnoreAction));
    EXPECT_FALSE(startManager->m_bIsTagDragging);
}

// openFilesInTab 内部 QTimer::singleShot(50,...) lambda #1
TEST(UT_StartManager_openFilesInTab, openFilesInTab_SingleShotLambda)
{
    StartManager *startManager = freshStartManager();
    startManager->m_windows.clear();

    Stub s1;
    s1.set(ADDR(StartManager, checkPath), returntruestub);
    Stub s2;
    s2.set(ADDR(StartManager, getFileTabInfo), getFileTabInfoNotFoundStub);
    Stub s3;
    s3.set(ADDR(StartManager, recoverFile), recoverFileStub);
    Stub s4;
    s4.set(ADDR(Window, addTab), returnstub);
    Stub s5;
    s5.set(ADDR(Window, showCenterWindow), returnstub);

    startManager->openFilesInTab(QStringList("somefile.txt"));

    // Fire the singleShot(50) timer's lambda by emitting timeout directly
    for (QTimer *t : startManager->findChildren<QTimer *>()) {
        if (t->isSingleShot()) {
            QMetaObject::invokeMethod(t, "timeout", Qt::DirectConnection);
            break;
        }
    }

    EXPECT_TRUE(startManager->m_windows.count() > 0);
}

// slotCloseWindow 内部 QTimer::singleShot(1000,...) lambda #1
TEST(UT_StartManager_slotCloseWindow, slotCloseWindow_SingleShotLambda)
{
    StartManager *startManager = freshStartManager();
    startManager->m_windows.clear();

    Stub s1;
    s1.set((QStringList(QDir::*)(QDir::Filters, QDir::SortFlags) const) ADDR(QDir, entryList), entryliststub);
    Stub s2;
    s2.set((bool(QString::*)(const QString &, Qt::CaseSensitivity) const) ADDR(QString, contains), returntruestub);
    Stub s3;
    s3.set(ADDR(QList<Window *>, isEmpty), returntruestub);

    startManager->slotCloseWindow();

    // Fire the singleShot(1000) timer's lambda by emitting timeout directly
    for (QTimer *t : startManager->findChildren<QTimer *>()) {
        if (t->isSingleShot()) {
            QMetaObject::invokeMethod(t, "timeout", Qt::DirectConnection);
            break;
        }
    }
}
