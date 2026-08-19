// SPDX-FileCopyrightText: 2026 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

#ifndef UT_EDITWRAPPER_MARKDOWN_H
#define UT_EDITWRAPPER_MARKDOWN_H

#include "gtest/gtest.h"
#include <gmock/gmock.h>
#include <QObject>
#include "markdown/imarkdownrenderer.h"

// GMock 替身：记录所有 IMarkdownRenderer 调用，无需启动 WebEngine
class MockMarkdownRenderer : public IMarkdownRenderer
{
public:
    // gmock 无法直接 mock const method，用 MOCK_CONST_METHOD0
    MOCK_CONST_METHOD0(isReady, bool());
    MOCK_METHOD1(setMarkdown, void(const QString &md));
    MOCK_METHOD1(setMode, void(int mode));
    MOCK_METHOD1(applyTheme, void(const QVariantMap &themeMap));
    MOCK_METHOD2(setLayout, void(int maxContentWidth, bool center));
    MOCK_METHOD1(scrollToRatio, void(double ratio));
};

class UT_EditWrapper_Markdown : public QObject, public ::testing::Test
{
    Q_OBJECT
public:
    UT_EditWrapper_Markdown();
};

#endif // UT_EDITWRAPPER_MARKDOWN_H
