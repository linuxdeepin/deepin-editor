// SPDX-FileCopyrightText: 2022-2026 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include <gtest/gtest.h>

#include <gmock/gmock-matchers.h>
#include <QApplication>
#include <DApplication>

#include "../../src/startmanager.h"

#if defined(CMAKE_SAFETYTEST_ARG_ON)
#include <sanitizer/asan_interface.h>
#endif

DWIDGET_USE_NAMESPACE

//#include <QTest>

int main(int argc, char *argv[])

{
    qputenv("QT_QPA_PLATFORM","offscreen");
    qputenv("QT_LOGGING_RULES", "*.debug=false;*.info=false");
    DApplication app(argc, argv);

    // BUG-378901 修复后 instance() 为纯查询访问器，不再惰性创建；
    // window/tabbar 等生产代码调用点依赖单例已存在，测试入口统一显式创建
    StartManager::create();

    testing::InitGoogleTest(&argc, argv);

    auto c = RUN_ALL_TESTS();

    #if defined(CMAKE_SAFETYTEST_ARG_ON)
    __sanitizer_set_report_path("asan.log");
    #endif

   return c;
}
