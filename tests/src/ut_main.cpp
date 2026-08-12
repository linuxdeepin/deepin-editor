// SPDX-FileCopyrightText: 2022-2026 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include <gtest/gtest.h>

#include <gmock/gmock-matchers.h>
#include <QApplication>
#include <DApplication>
#include <cstdlib>
#include <cstdio>

#if defined(CMAKE_SAFETYTEST_ARG_ON)
#include <sanitizer/asan_interface.h>
#endif

extern "C" void __gcov_dump(void);

DWIDGET_USE_NAMESPACE

//#include <QTest>

int main(int argc, char *argv[])

{
    qputenv("QT_QPA_PLATFORM","offscreen");
    qputenv("QT_LOGGING_RULES", "*.debug=false;*.info=false");
    DApplication app(argc, argv);

    testing::InitGoogleTest(&argc, argv);

    auto c = RUN_ALL_TESTS();

    __gcov_dump();

    fflush(nullptr);
    std::_Exit(c);
}
