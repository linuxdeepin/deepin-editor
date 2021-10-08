#include "iflytekaiassistantthread.h"

IflytekAiassistantThread::IflytekAiassistantThread(QObject *parent)
    : QThread (parent)
{}

IflytekAiassistantThread::~IflytekAiassistantThread()
{}

void IflytekAiassistantThread::run()
{
    bool bIsRegistIflytekAiassistant = false;
    QDBusConnection connection = QDBusConnection::sessionBus();
    QDBusConnectionInterface *bus = connection.interface();
    bool isVailid = bus->isServiceRegistered("com.iflytek.aiassistant");
    if (isVailid) {
        bIsRegistIflytekAiassistant = true;
    } else {
        qInfo() << "com.iflytek.aiassistant service no registered!";
    }

    emit sigIsRegisteredIflytekAiassistant(bIsRegistIflytekAiassistant);
}
