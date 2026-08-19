// SPDX-FileCopyrightText: 2026 UnionCTechnology Software Technology Co., Ltd.
// SPDX-License-Identifier: GPL-3.0-or-later

#ifndef UT_SCROLLSYNC_H
#define UT_SCROLLSYNC_H

#include "gtest/gtest.h"
#include <QObject>

class UT_ScrollSync : public QObject, public ::testing::Test
{
    Q_OBJECT
public:
    UT_ScrollSync();
};

#endif // UT_SCROLLSYNC_H
