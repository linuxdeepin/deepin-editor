// SPDX-FileCopyrightText: 2026 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

#ifndef UT_VIEWMODEFSM_H
#define UT_VIEWMODEFSM_H

#include "gtest/gtest.h"
#include <QObject>

class UT_ViewModeFsm : public QObject, public ::testing::Test
{
    Q_OBJECT
public:
    UT_ViewModeFsm();
};

#endif // UT_VIEWMODEFSM_H
