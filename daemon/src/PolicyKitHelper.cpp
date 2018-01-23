#include "PolicyKitHelper.h"

bool PolicyKitHelper::checkAuthorization(const QString& actionId, qint64 applicationPid)
{
    Authority::Result result;

    result = Authority::instance()->checkAuthorizationSync(actionId, UnixProcessSubject(applicationPid),
                                                           Authority::AllowUserInteraction);
    if (result == Authority::Yes) {
        return true;
    }else {
        return false;
    }
}

PolicyKitHelper::PolicyKitHelper()
{

}

PolicyKitHelper::~PolicyKitHelper()
{

}
