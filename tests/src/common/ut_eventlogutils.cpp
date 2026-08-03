// SPDX-FileCopyrightText: 2026 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include "../../src/common/eventlogutils.h"

#include <gtest/gtest.h>
#include <QJsonObject>
#include <QJsonDocument>

// Cover Eventlogutils::GetInstance() and the private Eventlogutils::Eventlogutils()
// constructor (invoked on first GetInstance call).
TEST(UT_Eventlogutils, GetInstance)
{
    Eventlogutils *p1 = Eventlogutils::GetInstance();
    ASSERT_NE(p1, nullptr);

    Eventlogutils *p2 = Eventlogutils::GetInstance();
    EXPECT_EQ(p1, p2);
}

// Cover Eventlogutils::writeLogs(QJsonObject&).
// writeEventLogFunc may be null when libdeepin-event-log.so is unavailable,
// the function still executes and returns early in that case.
TEST(UT_Eventlogutils, writeLogs)
{
    QJsonObject data;
    data["tid"] = static_cast<int>(Eventlogutils::StartUp);
    data["msg"] = QStringLiteral("unit test message");

    Eventlogutils::GetInstance()->writeLogs(data);
    SUCCEED();
}

// Cover writeLogs with empty json object.
TEST(UT_Eventlogutils, writeLogs_empty)
{
    QJsonObject data;
    Eventlogutils::GetInstance()->writeLogs(data);
    SUCCEED();
}
