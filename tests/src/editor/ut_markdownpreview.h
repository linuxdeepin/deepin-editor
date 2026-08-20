// SPDX-FileCopyrightText: 2026 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

#ifndef UT_MARKDOWNPREVIEW_H
#define UT_MARKDOWNPREVIEW_H

#include "gtest/gtest.h"
#include "../../src/editor/markdownpreview.h"

class UT_MarkdownPreview : public QObject, public ::testing::Test
{
public:
    UT_MarkdownPreview();
};

#endif // UT_MARKDOWNPREVIEW_H
