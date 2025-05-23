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

#include <DSysInfo>

// progress
static const QString kUosAiBin = "uos-ai-assistant";
static const QString kCopilotService = "com.deepin.copilot";
static const QString kCopilotPath = "/com/deepin/copilot";
static const QString kCopilotInterface = "com.deepin.copilot";
static const QString kFlytekService = "com.iflytek.aiassistant";

IflytekAiAssistant::IflytekAiAssistant(QObject *parent)
    : QObject(parent)
    , m_copilot{new QDBusInterface(kCopilotService, kCopilotPath, kCopilotInterface)}
{
    qDebug() << "IflytekAiAssistant created";
    m_copilot->setTimeout(2000);
}

IflytekAiAssistant *IflytekAiAssistant::instance()
{
    static IflytekAiAssistant ins;
    return &ins;
}

/**
 * @brief Check if uos-ai is installed and user experience is valid.
 *        If not valid, launch uos-ai chat page.
 */
IflytekAiAssistant::CallStatus IflytekAiAssistant::checkValid()
{
    qDebug() << "checkValid status:" << m_status;
    switch (m_status) {
        case NotInstalled:
            m_status = copilotInstalled(m_copilot);
            if (Enable != m_status) {
                qDebug() << "status is not enable!";;
                break;
            }
            // If detect installed first, detect the user argreement is valid.
            Q_FALLTHROUGH();
        case NoUserAgreement: {
            // Detect current state every call
            m_status = isCopilotEnabled(m_copilot);
            if (NoUserAgreement == m_status) {
                qDebug() << "user agreement not agreed!";
                launchCopilotChat(m_copilot);
            }
        } break;
        default:
            break;
    }

    qDebug() << "checkValid status, return:" << m_status;
    return m_status;
}

IflytekAiAssistant::CallStatus IflytekAiAssistant::isTtsInWorking() const
{
    if (Enable != status()) {
        qDebug() << "status is not enable!";
        return status();
    }

    QDBusInterface interface(copilotService(), "/aiassistant/tts", "com.iflytek.aiassistant.tts");
    interface.setTimeout(100);

    QDBusReply<bool> ret = interface.call("isTTSInWorking");
    if (ret.isValid()) {
        qDebug() << "isTtsInWorking ret:" << ret.value();
        return ret.value() ? Enable : Disable;
    }
    return Invalid;
}

IflytekAiAssistant::CallStatus IflytekAiAssistant::isTtsEnable() const
{
    if (Enable != status()) {
        qDebug() << "status is not enable!";
        return status();
    }

    QDBusInterface interface(copilotService(), "/aiassistant/tts", "com.iflytek.aiassistant.tts");
    interface.setTimeout(100);

    QDBusReply<bool> ret = interface.call("getTTSEnable");
    if (ret.isValid()) {
        qDebug() << "getTTSEnable ret:" << ret.value();
        return ret.value() ? Enable : Disable;
    }
    return Invalid;
}

IflytekAiAssistant::CallStatus IflytekAiAssistant::textToSpeech()
{
    if (Enable != checkValid()) {
        qDebug() << "checkValid failed!";
        return status();
    }

    CallStatus ret = isTtsEnable();
    if (Enable != ret) {
        qDebug() << "no output device!";
        return NoOutputDevice;
    }

    // playing, stop first
    if (isTtsInWorking()) {
        if (Success != stopTtsDirectlyInternal()) {
            qWarning() << "stopTtsDirectlyInternal failed!";
            return Failed;
        }
    }

    QDBusInterface interface(copilotService(), "/aiassistant/deepinmain", "com.iflytek.aiassistant.mainWindow");
    interface.asyncCall("TextToSpeech");

    qDebug() << "textToSpeech success!";
    return Success;
}

IflytekAiAssistant::CallStatus IflytekAiAssistant::stopTtsDirectly() const
{
    // BUG-301561 : disable stop tts when close tab or window
#ifdef ENABLE_STOP_TTS
    qDebug() << "ENABLE_STOP_TTS is defined!";
    return stopTtsDirectlyInternal()
#else
    qDebug() << "ENABLE_STOP_TTS is not defined!, return Enable";
    return Enable;
#endif
}

IflytekAiAssistant::CallStatus IflytekAiAssistant::stopTtsDirectlyInternal() const
{
    if (Enable != status()) {
        qDebug() << "status is not enable!";
        return status();
    }

    QDBusInterface interface(copilotService(), "/aiassistant/tts", "com.iflytek.aiassistant.tts");
    interface.asyncCall("stopTTSDirectly");
    qDebug() << "stopTtsDirectly success!";
    return Success;
}

IflytekAiAssistant::CallStatus IflytekAiAssistant::getIatEnable() const
{
    if (Enable != status()) {
        qDebug() << "status is not enable!";
        return status();
    }

    QDBusInterface interface(copilotService(), "/aiassistant/iat", "com.iflytek.aiassistant.iat");
    interface.setTimeout(100);

    QDBusReply<bool> ret = interface.call("getIatEnable");
    if (ret.isValid()) {
        qDebug() << "getIatEnable ret:" << ret.value();
        return ret.value() ? Enable : Disable;
    }
    return Invalid;
}

IflytekAiAssistant::CallStatus IflytekAiAssistant::speechToText()
{
    if (Enable != checkValid()) {
        qDebug() << "checkValid failed!";
        return status();
    }

    CallStatus ret = getIatEnable();
    if (Enable != ret) {
        qDebug() << "no input device!";
        return NoInputDevice;
    }

    QDBusInterface interface(copilotService(), "/aiassistant/deepinmain", "com.iflytek.aiassistant.mainWindow");
    interface.asyncCall("SpeechToText");

    qDebug() << "speechToText success!";
    return Success;
}

IflytekAiAssistant::CallStatus IflytekAiAssistant::getTransEnable() const
{
    if (Enable != status()) {
        qDebug() << "status is not enable!";
        return status();
    }

    QDBusInterface interface(copilotService(), "/aiassistant/trans", "com.iflytek.aiassistant.trans");
    interface.setTimeout(100);

    QDBusReply<bool> ret = interface.call("getTransEnable");
    if (ret.isValid()) {
        qDebug() << "getTransEnable ret:" << ret.value();
        return ret.value() ? Enable : Disable;
    }
    qDebug() << "getTransEnable failed! return Invalid";
    return Invalid;
}

IflytekAiAssistant::CallStatus IflytekAiAssistant::textToTranslate()
{
    if (Enable != checkValid()) {
        qDebug() << "checkValid failed!";
        return status();
    }

    CallStatus ret = getTransEnable();
    if (Enable != ret) {
        qWarning() << "getTransEnable is not enable!";
        return Invalid;
    }

    QDBusInterface interface(copilotService(), "/aiassistant/deepinmain", "com.iflytek.aiassistant.mainWindow");
    interface.asyncCall("TextToTranslate");

    qDebug() << "textToTranslate success!";
    return Success;
}

QString IflytekAiAssistant::errorString(CallStatus ret) const
{
    qDebug() << "errorString ret:" << ret;
    switch (ret) {
        case NotInstalled:
            return QObject::tr("Please install 'UOS AI' from the App Store before using");
        case NoInputDevice:
            return QObject::tr("No audio input device detected. Please check and try again");
        case NoOutputDevice:
            return QObject::tr("No audio output device detected. Please check and try again");
        default:
            // Unknown error
            qDebug() << "unknown error ret:" << ret;
            return {};
    }
}

IflytekAiAssistant::CallStatus IflytekAiAssistant::copilotInstalled(const QSharedPointer<QDBusInterface> &copilot)
{
    QDBusReply<QString> version = copilot->call("version");
    if (version.isValid()) {
        qInfo() << "current uos-ai version:" << version.value();
        return Enable;
    }

    qWarning() << "Query uos-ai installed faild! Maybe need install";
    return NotInstalled;
}

/**
 * @brief Query uos-ai user agreement state.
 *        If no respond, return NotInstalled.
 */
IflytekAiAssistant::CallStatus IflytekAiAssistant::isCopilotEnabled(const QSharedPointer<QDBusInterface> &copilot)
{
    QDBusReply<bool> state = copilot->call("isCopilotEnabled");
    if (state.isValid()) {
        qDebug() << "current uos-ai user exp state:" << state.value();
        return state.value() ? Enable : NoUserAgreement;
    }

    // NOTE: Adapt old version, if dbus interface not valid, assume the user agreement agreed.
    qWarning() << "Query uos-ai user exp state failed!" << state.error().message();
    return Enable;
}

/**
 * @brief Launch uos-ai chat page, show window.
 */
IflytekAiAssistant::CallStatus IflytekAiAssistant::launchCopilotChat(const QSharedPointer<QDBusInterface> &copilot)
{
    QDBusMessage message = copilot->call("launchChatPage");
    if (!message.errorMessage().isEmpty()) {
        qWarning() << "Launch uos-ai chat page failed!" << message.errorMessage();
        return Disable;
    }

    return Enable;
}

void IflytekAiAssistant::checkAiExists()
{
    qDebug() << "checkAiExists status:" << m_status;
    static std::once_flag kAiFlag;
    std::call_once(kAiFlag, [this]() {
        QtConcurrent::run([this]() {
            // If call dbus interface success, the uos-ai backend process started.
            auto copilot = QSharedPointer<QDBusInterface>::create(kCopilotService, kCopilotPath, kCopilotInterface);
            CallStatus status = IflytekAiAssistant::copilotInstalled(copilot);
            if (Enable == status) {
                status = IflytekAiAssistant::isCopilotEnabled(copilot);
            }

            qInfo() << QString("backend uos-ai status: %1(%2)").arg(Enable == status).arg(status);

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

QString IflytekAiAssistant::copilotService()
{
    return kCopilotService;
}
