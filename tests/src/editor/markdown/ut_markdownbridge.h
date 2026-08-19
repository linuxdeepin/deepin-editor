// SPDX-FileCopyrightText: 2026 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

#ifndef UT_MARKDOWNBRIDGE_H
#define UT_MARKDOWNBRIDGE_H

#include "gtest/gtest.h"
#include <QObject>

class UT_MarkdownBridge : public QObject, public ::testing::Test
{
    Q_OBJECT
public:
    UT_MarkdownBridge();
};

#endif // UT_MARKDOWNBRIDGE_H
