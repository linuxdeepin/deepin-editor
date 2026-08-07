// SPDX-FileCopyrightText: 2026 UnionTech Software Technology Co., Ltd.
// SPDX-License-Identifier: GPL-3.0-or-later

#include <gtest/gtest.h>
#include <gmock/gmock.h>
#include <QGuiApplication>

// deepin-editor unit tests exercise Qt GUI-layer code paths (QFontDatabase,
// QPainter, QTextDocument, etc.) that require a QGuiApplication to exist.
// We construct an offscreen QGuiApplication once before running the tests.
int main(int argc, char **argv)
{
    // Force the offscreen platform if the caller did not set one, so the test
    // binary is usable on headless hosts / CI without an X server.
    qputenv("QT_QPA_PLATFORM", QByteArrayLiteral("offscreen"));
    QGuiApplication app(argc, argv);

    ::testing::InitGoogleTest(&argc, argv);
    ::testing::InitGoogleMock(&argc, argv);
    return RUN_ALL_TESTS();
}
