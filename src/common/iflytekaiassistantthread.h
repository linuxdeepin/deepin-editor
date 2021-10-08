#ifndef IFLYTEKAIASSISTANTTHREAD_H
#define IFLYTEKAIASSISTANTTHREAD_H

#include <QThread>
#include <QDBusConnection>
#include <QDBusConnectionInterface>
#include <QDebug>

class IflytekAiassistantThread : public QThread
{
    Q_OBJECT
public:
    IflytekAiassistantThread(QObject *object = nullptr);
    ~IflytekAiassistantThread();

private:
    void run();

signals:
    void sigIsRegisteredIflytekAiassistant(const bool bIsRegistIflytekAiassistant);
};

#endif // IFLYTEKAIASSISTANTTHREAD_H
