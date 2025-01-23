// SPDX-FileCopyrightText: 2022 - 2024 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

#ifndef IFLYTEKAIASSISTANT_H
#define IFLYTEKAIASSISTANT_H

#include <QObject>
#include <QSharedPointer>

class QDBusInterface;

class IflytekAiAssistant : public QObject
{
    Q_OBJECT

public:
    static IflytekAiAssistant *instance();

    enum CallStatus {
        Invalid = -1,  // interface no result or error
        Disable = 0,
        Enable = 1,
        NotInstalled,     // not install uos-ai
        NoUserAgreement,  // the user agreement not agreed
        NoInputDevice,    // the dbus interface not valid or return disable
        NoOutputDevice,

        Success = Enable,
        Failed = Disable,
    };

    inline CallStatus status() const { return m_status; }
    inline bool valid() const { return Enable == status(); }
    void checkAiExists();
    CallStatus checkValid();

    // text to speech
    CallStatus isTtsInWorking() const;
    CallStatus isTtsEnable() const;
    CallStatus textToSpeech();
    CallStatus stopTtsDirectly() const;

    // speech to text
    CallStatus getIatEnable() const;
    CallStatus speechToText();

    // text translation
    CallStatus getTransEnable() const;
    CallStatus textToTranslate();

    QString errorString(CallStatus ret) const;

    Q_SIGNAL void initFinished();

private:
    explicit IflytekAiAssistant(QObject *object = nullptr);
    ~IflytekAiAssistant() override = default;

    CallStatus stopTtsDirectlyInternal() const;

    static CallStatus copilotInstalled(const QSharedPointer<QDBusInterface> &copilot);
    static CallStatus isCopilotEnabled(const QSharedPointer<QDBusInterface> &copilot);
    static CallStatus launchCopilotChat(const QSharedPointer<QDBusInterface> &copilot);

    static QString copilotService();

    bool m_inited{false};
    CallStatus m_status{Invalid};
    QSharedPointer<QDBusInterface> m_copilot;
};

#endif  // IFLYTEKAIASSISTANT_H
