#ifndef DEEPIN_EDITOR_DAEMON_POLICYKITHELPER_H
#define DEEPIN_EDITOR_DAEMON_POLICYKITHELPER_H

#include <polkit-qt5-1/PolkitQt1/Authority>

using namespace PolkitQt1;

class PolicyKitHelper {
public:
    inline static PolicyKitHelper* instance(){
        static PolicyKitHelper instance;
        return &instance;
    }

    bool checkAuthorization(const QString& actionId, qint64 applicationPid);

private:
    PolicyKitHelper();
    ~PolicyKitHelper();

    Q_DISABLE_COPY(PolicyKitHelper)

};


#endif //DEEPIN_EDITOR_DAEMON_POLICYKITHELPER_H
