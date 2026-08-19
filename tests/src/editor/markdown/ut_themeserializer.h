// SPDX-FileCopyrightText: 2026 UnionCTechnology Software Technology Co., Ltd.
// SPDX-License-Identifier: GPL-3.0-or-later

#ifndef UT_THEMESERIALIZER_H
#define UT_THEMESERIALIZER_H

#include "gtest/gtest.h"
#include <QObject>

class UT_ThemeSerializer : public QObject, public ::testing::Test
{
    Q_OBJECT
public:
    UT_ThemeSerializer();
};

#endif // UT_THEMESERIALIZER_H
