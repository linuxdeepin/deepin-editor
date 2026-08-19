// SPDX-FileCopyrightText: 2026 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

#ifndef UT_MARKDOWNLOGIC_H
#define UT_MARKDOWNLOGIC_H

#include "gtest/gtest.h"
#include <QObject>

class UT_MarkdownLogic : public QObject, public ::testing::Test
{
    Q_OBJECT
public:
    UT_MarkdownLogic();
};

#endif // UT_MARKDOWNLOGIC_H
