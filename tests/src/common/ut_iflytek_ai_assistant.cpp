// SPDX-FileCopyrightText: 2026 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include "../../src/common/iflytek_ai_assistant.h"

#include <gtest/gtest.h>
#include <QDBusInterface>
#include <QSharedPointer>
#include <QDebug>

// Cover IflytekAiAssistant::launchCopilotChat(QSharedPointer<QDBusInterface> const&)
// which is a private static method (accessible via -fno-access-control).
// Using an invalid interface, call() returns an error message so the function
// returns Disable. The exact result depends on the DBus environment, so we
// only assert the function executes and returns a known enum value.
TEST(UT_IflytekAiAssistant, launchCopilotChat)
{
    QSharedPointer<QDBusInterface> copilot =
        QSharedPointer<QDBusInterface>::create("com.deepin.copilot",
                                               "/com/deepin/copilot",
                                               "com.deepin.copilot");
    ASSERT_FALSE(copilot.isNull());

    IflytekAiAssistant::CallStatus ret =
        IflytekAiAssistant::launchCopilotChat(copilot);
    // The return value is either Disable (error path) or Enable (no error msg).
    EXPECT_TRUE(ret == IflytekAiAssistant::Disable ||
                ret == IflytekAiAssistant::Enable);
}

// Cover IflytekAiAssistant::stopTtsDirectlyInternal() const (private method).
// When status() != Enable it returns the current status immediately.
TEST(UT_IflytekAiAssistant, stopTtsDirectlyInternal_notEnabled)
{
    IflytekAiAssistant *ins = IflytekAiAssistant::instance();
    IflytekAiAssistant::CallStatus ret = ins->stopTtsDirectlyInternal();
    EXPECT_EQ(ret, ins->status());
}

// Cover IflytekAiAssistant::stopTtsDirectlyInternal() const on the success path.
// Force m_status to Enable so the DBus call branch is exercised.
TEST(UT_IflytekAiAssistant, stopTtsDirectlyInternal_enabled)
{
    IflytekAiAssistant *ins = IflytekAiAssistant::instance();
    IflytekAiAssistant::CallStatus oldStatus = ins->m_status;
    ins->m_status = IflytekAiAssistant::Enable;

    IflytekAiAssistant::CallStatus ret = ins->stopTtsDirectlyInternal();
    EXPECT_EQ(ret, IflytekAiAssistant::Success);

    ins->m_status = oldStatus;
}

// Cover IflytekAiAssistant::~IflytekAiAssistant() (private, defaulted destructor)
// by constructing and destructing a fresh instance.
TEST(UT_IflytekAiAssistant, Destructor)
{
    IflytekAiAssistant *p = new IflytekAiAssistant();
    ASSERT_NE(p, nullptr);
    delete p;
    SUCCEED();
}
