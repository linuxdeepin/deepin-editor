// SPDX-FileCopyrightText: 2022-2026 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include <gtest/gtest.h>

#include <gmock/gmock-matchers.h>
#include <QApplication>
#include <DApplication>
#include <QCoreApplication>
#include "src/stub.h"
#include <cstdlib>
#include <cstdio>
#include <csignal>
#include <unistd.h>

#if defined(CMAKE_SAFETYTEST_ARG_ON)
#include <sanitizer/asan_interface.h>
#endif

extern "C" void __gcov_dump(void);

DWIDGET_USE_NAMESPACE

static void quitNoop() { return; }

static void alarmHandler(int)
{
    __gcov_dump();
    fflush(nullptr);
    std::_Exit(1);
}

static void crashHandler(int sig)
{
    __gcov_dump();
    fflush(nullptr);
    std::_Exit(sig);
}

int main(int argc, char *argv[])

{
    qputenv("QT_QPA_PLATFORM","offscreen");
    qputenv("QT_LOGGING_RULES", "*.debug=false;*.info=false");
    // 与生产 main.cpp 一致：WebEngine 渲染视图需要共享 GL 上下文（须在 QApplication 前设置）
    QCoreApplication::setAttribute(Qt::AA_ShareOpenGLContexts);
    DApplication app(argc, argv);

    // Stub QCoreApplication::quit globally to prevent tests from calling quit()
    // and corrupting the process state (slotCloseWindow timer calls quit()).
    Stub quitStub;
    quitStub.set(ADDR(QCoreApplication, quit), quitNoop);

    // Install SIGALRM handler so coverage data is preserved if the test suite hangs.
    // The alarm fires after 3600s, dumps gcov data, and exits. The full suite
    // (1291+ tests) needs ~300s; 3600s keeps hang protection without killing
    // a normal run (CI total timeout is 7200s).
    signal(SIGALRM, alarmHandler);
    signal(SIGABRT, crashHandler);
    signal(SIGSEGV, crashHandler);
    alarm(3600);

    testing::InitGoogleTest(&argc, argv);

    auto c = RUN_ALL_TESTS();

    alarm(0);  // Cancel the alarm

    __gcov_dump();

    fflush(nullptr);
    std::_Exit(c);
}
