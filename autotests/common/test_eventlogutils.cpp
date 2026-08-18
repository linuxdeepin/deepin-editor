// SPDX-FileCopyrightText: 2026 UnionTech Software Technology Co., Ltd.
// SPDX-License-Identifier: GPL-3.0-or-later

#include "../../src/common/eventlogutils.h"

#include <gtest/gtest.h>
#include <QJsonObject>

TEST(EventlogutilsTest, GetInstance_ReturnsSameSingleton)
{
    Eventlogutils *a = Eventlogutils::GetInstance();
    Eventlogutils *b = Eventlogutils::GetInstance();
    ASSERT_NE(a, nullptr);
    EXPECT_EQ(a, b);
}

TEST(EventlogutilsTest, WriteLogs_FullObject_DoesNotCrash)
{
    QJsonObject data;
    data["tid"] = static_cast<int>(Eventlogutils::StartUp);
    data["msg"] = QStringLiteral("autotest startup");
    Eventlogutils::GetInstance()->writeLogs(data);
    SUCCEED();
}

TEST(EventlogutilsTest, WriteLogs_EmptyObject_DoesNotCrash)
{
    QJsonObject data;
    Eventlogutils::GetInstance()->writeLogs(data);
    SUCCEED();
}

TEST(EventlogutilsTest, WriteLogs_QuitEvent_DoesNotCrash)
{
    QJsonObject data;
    data["tid"] = static_cast<int>(Eventlogutils::Quit);
    Eventlogutils::GetInstance()->writeLogs(data);
    SUCCEED();
}
