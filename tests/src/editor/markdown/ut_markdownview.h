// SPDX-FileCopyrightText: 2026 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

#ifndef UT_MARKDOWNVIEW_H
#define UT_MARKDOWNVIEW_H

#include "gtest/gtest.h"
#include <QObject>

class UT_MarkdownView : public QObject, public ::testing::Test
{
    Q_OBJECT
public:
    UT_MarkdownView();
};

#endif // UT_MARKDOWNVIEW_H
