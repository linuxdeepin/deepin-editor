// SPDX-FileCopyrightText: 2026 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

#ifndef UT_RENDERTHROTTLE_H
#define UT_RENDERTHROTTLE_H

#include "gtest/gtest.h"
#include <QObject>

class UT_RenderThrottle : public QObject, public ::testing::Test
{
    Q_OBJECT
public:
    UT_RenderThrottle();
};

#endif // UT_RENDERTHROTTLE_H
