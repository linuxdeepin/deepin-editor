// SPDX-FileCopyrightText: 2022 - 2024 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

#ifndef IFLYTEKAIASSISTANT_H
#define IFLYTEKAIASSISTANT_H

#include <QObject>

class QDBusInterface;

class IflytekAiAssistant : public QObject
{
    Q_OBJECT

public:
    static IflytekAiAssistant *instance();

    enum CallStatus {
        Invalid = -1,
        Disable = 0,
        Enable = 1,
        NotInstalled,   // not install uos-ai
        NoInputDevice,  // the dbus interface not valid or return disable
        NoOutputDevice,

        Success = Enable,
        Failed = Disable,
    };

    inline CallStatus status() const { return m_status; }
    inline bool valid() const { return Enable == status(); }
    void checkAiExists();

    // text to speech
    CallStatus isTtsInWorking() const;
    CallStatus isTtsEnable() const;
    CallStatus textToSpeech() const;
    CallStatus stopTtsDirectly() const;

    // speech to text
    CallStatus getIatEnable() const;
    CallStatus speechToText() const;

    // text translation
    CallStatus getTransEnable() const;
    CallStatus textToTranslate() const;

    QString errorString(CallStatus ret) const;

    Q_SIGNAL void initFinished();

private:
    explicit IflytekAiAssistant(QObject *object = nullptr);
    ~IflytekAiAssistant() override = default;

    bool m_inited{false};
    CallStatus m_status{Invalid};
};

#endif  // IFLYTEKAIASSISTANT_H
