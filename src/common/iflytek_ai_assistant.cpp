// SPDX-FileCopyrightText: 2022 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include "iflytek_ai_assistant.h"

#include <mutex>

#include <QtConcurrent/QtConcurrentRun>
#include <QDBusConnection>
#include <QDBusConnectionInterface>
#include <QDBusInterface>
#include <QDebug>
#include <QProcess>
#include <QStandardPaths>

// progress
static const QString kUosAiBin = "uos-ai-assistant";

IflytekAiAssistant::IflytekAiAssistant(QObject *parent)
    : QObject(parent)
{
}

IflytekAiAssistant *IflytekAiAssistant::instance()
{
    static IflytekAiAssistant ins;
    return &ins;
}

IflytekAiAssistant::CallStatus IflytekAiAssistant::isTtsInWorking() const
{
    if (Enable != status()) {
        return status();
    }

    QDBusInterface interface("com.iflytek.aiassistant", "/aiassistant/tts", "com.iflytek.aiassistant.tts");
    interface.setTimeout(100);

    QDBusReply<bool> ret = interface.call("isTTSInWorking");
    if (ret.isValid()) {
        return ret.value() ? Enable : Disable;
    }
    return Invalid;
}

IflytekAiAssistant::CallStatus IflytekAiAssistant::isTtsEnable() const
{
    if (Enable != status()) {
        return status();
    }

    QDBusInterface interface("com.iflytek.aiassistant", "/aiassistant/tts", "com.iflytek.aiassistant.tts");
    interface.setTimeout(100);

    QDBusReply<bool> ret = interface.call("getTTSEnable");
    if (ret.isValid()) {
        return ret.value() ? Enable : Disable;
    }
    return Invalid;
}

IflytekAiAssistant::CallStatus IflytekAiAssistant::textToSpeech() const
{
    if (Enable != status()) {
        return status();
    }

    CallStatus ret = isTtsEnable();
    if (Enable != ret) {
        return NoOutputDevice;
    }

    // playing, stop first
    if (isTtsInWorking()) {
        if (Success != stopTtsDirectly()) {
            return Failed;
        }
    }

    QDBusInterface interface("com.iflytek.aiassistant", "/aiassistant/deepinmain", "com.iflytek.aiassistant.mainWindow");
    interface.asyncCall("TextToSpeech");

    return Success;
}

IflytekAiAssistant::CallStatus IflytekAiAssistant::stopTtsDirectly() const
{
    if (Enable != status()) {
        return status();
    }

    QDBusInterface interface("com.iflytek.aiassistant", "/aiassistant/tts", "com.iflytek.aiassistant.tts");
    interface.asyncCall("stopTTSDirectly");
    return Success;
}

IflytekAiAssistant::CallStatus IflytekAiAssistant::getIatEnable() const
{
    if (Enable != status()) {
        return status();
    }

    QDBusInterface interface("com.iflytek.aiassistant", "/aiassistant/iat", "com.iflytek.aiassistant.iat");
    interface.setTimeout(100);

    QDBusReply<bool> ret = interface.call("getIatEnable");
    if (ret.isValid()) {
        return ret.value() ? Enable : Disable;
    }
    return Invalid;
}

IflytekAiAssistant::CallStatus IflytekAiAssistant::speechToText() const
{
    if (Enable != status()) {
        return status();
    }

    CallStatus ret = getIatEnable();
    if (Enable != ret) {
        return NoInputDevice;
    }

    QDBusInterface interface("com.iflytek.aiassistant", "/aiassistant/deepinmain", "com.iflytek.aiassistant.mainWindow");
    interface.asyncCall("SpeechToText");

    return Success;
}

IflytekAiAssistant::CallStatus IflytekAiAssistant::getTransEnable() const
{
    if (Enable != status()) {
        return status();
    }

    QDBusInterface interface("com.iflytek.aiassistant", "/aiassistant/trans", "com.iflytek.aiassistant.trans");
    interface.setTimeout(100);

    QDBusReply<bool> ret = interface.call("getTransEnable");
    if (ret.isValid()) {
        return ret.value() ? Enable : Disable;
    }
    return Invalid;
}

IflytekAiAssistant::CallStatus IflytekAiAssistant::textToTranslate() const
{
    if (Enable != status()) {
        return status();
    }

    CallStatus ret = getTransEnable();
    if (Enable != ret) {
        return Invalid;
    }

    QDBusInterface interface("com.iflytek.aiassistant", "/aiassistant/deepinmain", "com.iflytek.aiassistant.mainWindow");
    interface.asyncCall("TextToTranslate");

    return Success;
}

QString IflytekAiAssistant::errorString(CallStatus ret) const
{
    switch (ret) {
        case NotInstalled:
            return QObject::tr("Please install 'UOS AI' from the App Store before using");
        case NoInputDevice:
            return QObject::tr("No audio input device detected. Please check and try again");
        case NoOutputDevice:
            return QObject::tr("No audio output device detected. Please check and try again");
        default:
            return "Unknown error";
    }
}

void IflytekAiAssistant::checkAiExists()
{
    static std::once_flag kAiFlag;
    std::call_once(kAiFlag, [this]() {
        QtConcurrent::run([this]() {
            QDBusConnection connection = QDBusConnection::sessionBus();
            QDBusConnectionInterface *bus = connection.interface();
            CallStatus status = Invalid;

            if (bus->isServiceRegistered("com.iflytek.aiassistant")) {
                status = Enable;
            }

            if (Enable != status) {
                // Check if install uos-ai
                if (QStandardPaths::findExecutable(kUosAiBin).isEmpty()) {
                    status = NotInstalled;
                }

                // Try to start uos-ai
                if (NotInstalled != status) {
                    QProcess process;
                    process.setProgram(kUosAiBin);
                    qint64 pid = 0;
                    if (process.startDetached(&pid)) {
                        status = Enable;
                    }

                    qInfo() << QString("Ai service not register, try to start %1, ret: %2, pid: %3")
                                   .arg(kUosAiBin)
                                   .arg(Enable == status)
                                   .arg(pid);
                } else {
                    qInfo() << QString("Ai service not register, not found %1").arg(kUosAiBin);
                }
            }

            // call on non-gui thread, so queued connection.
            QMetaObject::invokeMethod(
                this,
                [status, this]() {
                    this->m_inited = true;
                    this->m_status = status;

                    Q_EMIT initFinished();
                },
                Qt::QueuedConnection);
        });
    });
}
