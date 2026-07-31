// SPDX-FileCopyrightText: 2011 - 2026 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include "policykithelper.h"

bool PolicyKitHelper::checkAuthorization(const QString& actionId, const QString& appBusName)
{
    if (appBusName.isEmpty())
        return false;

    Authority::Result result;

    result = Authority::instance()->checkAuthorizationSync(
        actionId,
        SystemBusNameSubject(appBusName),
        Authority::AllowUserInteraction);

    return result == Authority::Yes;
}

PolicyKitHelper::PolicyKitHelper()
{

}

PolicyKitHelper::~PolicyKitHelper()
{

}
