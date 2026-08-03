// SPDX-FileCopyrightText: 2026 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

#ifndef TEST_PATHSETTINTWGT_H
#define TEST_PATHSETTINTWGT_H

#include "../../src/widgets/pathsettintwgt.h"

#include <gtest/gtest.h>

class test_pathsettintwgt : public testing::Test
{
protected:
    void SetUp()
    {
        m_wgt = new PathSettingWgt();
    }
    void TearDown()
    {
        delete m_wgt;
    }

    PathSettingWgt *m_wgt = nullptr;
};

#endif // TEST_PATHSETTINTWGT_H
