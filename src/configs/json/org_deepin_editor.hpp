// SPDX-FileCopyrightText: 2024 - 2025 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

#ifndef ORG_DEEPIN_EDITOR_H
#define ORG_DEEPIN_EDITOR_H

#include <QThread>
#include <QVariant>
#include <QDebug>
#include <QAtomicPointer>
#include <QAtomicInteger>
#include <DConfig>

class org_deepin_editor : public QObject {
    Q_OBJECT

    Q_PROPERTY(QString defaultEncoding READ defaultEncoding WRITE setDefaultEncoding NOTIFY defaultEncodingChanged)
    Q_PROPERTY(bool disableImproveGB18030 READ disableImproveGB18030 WRITE setDisableImproveGB18030 NOTIFY disableImproveGB18030Changed)
    Q_PROPERTY(bool enablePatchedIconv READ enablePatchedIconv WRITE setEnablePatchedIconv NOTIFY enablePatchedIconvChanged)
public:
    explicit org_deepin_editor(QThread *thread, const QString &appId, const QString &name, const QString &subpath, QObject *parent = nullptr)
        : QObject(parent) {

        if (!thread->isRunning()) {
            qWarning() << QStringLiteral("Warning: The provided thread is not running.");
        }
        Q_ASSERT(QThread::currentThread() != thread);
        auto worker = new QObject();
        worker->moveToThread(thread);
        QMetaObject::invokeMethod(worker, [=]() {
            auto config = DTK_CORE_NAMESPACE::DConfig::create(appId, name, subpath, nullptr);
            if (!config) {
                qWarning() << QStringLiteral("Failed to create DConfig instance.");
                worker->deleteLater();
                return;
            }
            config->moveToThread(QThread::currentThread());
            initialize(config);
            worker->deleteLater();
        });
    }
    explicit org_deepin_editor(QThread *thread, DTK_CORE_NAMESPACE::DConfigBackend *backend, const QString &appId, const QString &name, const QString &subpath, QObject *parent = nullptr)
        : QObject(parent) {

        if (!thread->isRunning()) {
            qWarning() << QStringLiteral("Warning: The provided thread is not running.");
        }
        Q_ASSERT(QThread::currentThread() != thread);
        auto worker = new QObject();
        worker->moveToThread(thread);
        QMetaObject::invokeMethod(worker, [=]() {
            auto config = DTK_CORE_NAMESPACE::DConfig::create(backend, appId, name, subpath, nullptr);
            if (!config) {
                qWarning() << QStringLiteral("Failed to create DConfig instance.");
                worker->deleteLater();
                return;
            }
            config->moveToThread(QThread::currentThread());
            initialize(config);
            worker->deleteLater();
        });
    }
    explicit org_deepin_editor(QThread *thread, const QString &name, const QString &subpath, QObject *parent = nullptr)
        : QObject(parent) {

        if (!thread->isRunning()) {
            qWarning() << QStringLiteral("Warning: The provided thread is not running.");
        }
        Q_ASSERT(QThread::currentThread() != thread);
        auto worker = new QObject();
        worker->moveToThread(thread);
        QMetaObject::invokeMethod(worker, [=]() {
            auto config = DTK_CORE_NAMESPACE::DConfig::create(name, subpath, nullptr);
            if (!config) {
                qWarning() << QStringLiteral("Failed to create DConfig instance.");
                worker->deleteLater();
                return;
            }
            config->moveToThread(QThread::currentThread());
            initialize(config);
            worker->deleteLater();
        });
    }
    explicit org_deepin_editor(QThread *thread, DTK_CORE_NAMESPACE::DConfigBackend *backend, const QString &name, const QString &subpath, QObject *parent = nullptr)
        : QObject(parent) {

        if (!thread->isRunning()) {
            qWarning() << QStringLiteral("Warning: The provided thread is not running.");
        }
        Q_ASSERT(QThread::currentThread() != thread);
        auto worker = new QObject();
        worker->moveToThread(thread);
        QMetaObject::invokeMethod(worker, [=]() {
            auto config = DTK_CORE_NAMESPACE::DConfig::create(backend, name, subpath, nullptr);
            if (!config) {
                qWarning() << QStringLiteral("Failed to create DConfig instance.");
                worker->deleteLater();
                return;
            }
            config->moveToThread(QThread::currentThread());
            initialize(config);
            worker->deleteLater();
        });
    }
    ~org_deepin_editor() {
        if (m_config.loadRelaxed()) {
            m_config.loadRelaxed()->deleteLater();
        }
    }

    QString defaultEncoding() const {
        return p_defaultEncoding;
    }
    void setDefaultEncoding(const QString &value) {
        auto oldValue = p_defaultEncoding;
        p_defaultEncoding = value;
        markPropertySet(0);
        if (auto config = m_config.loadRelaxed()) {
            QMetaObject::invokeMethod(config, [this, value]() {
                m_config.loadRelaxed()->setValue(QStringLiteral("defaultEncoding"), value);
            });
        }
        if (p_defaultEncoding != oldValue) {
            Q_EMIT defaultEncodingChanged();
        }
    }
    bool disableImproveGB18030() const {
        return p_disableImproveGB18030;
    }
    void setDisableImproveGB18030(const bool &value) {
        auto oldValue = p_disableImproveGB18030;
        p_disableImproveGB18030 = value;
        markPropertySet(1);
        if (auto config = m_config.loadRelaxed()) {
            QMetaObject::invokeMethod(config, [this, value]() {
                m_config.loadRelaxed()->setValue(QStringLiteral("disableImproveGB18030"), value);
            });
        }
        if (p_disableImproveGB18030 != oldValue) {
            Q_EMIT disableImproveGB18030Changed();
        }
    }
    bool enablePatchedIconv() const {
        return p_enablePatchedIconv;
    }
    void setEnablePatchedIconv(const bool &value) {
        auto oldValue = p_enablePatchedIconv;
        p_enablePatchedIconv = value;
        markPropertySet(2);
        if (auto config = m_config.loadRelaxed()) {
            QMetaObject::invokeMethod(config, [this, value]() {
                m_config.loadRelaxed()->setValue(QStringLiteral("enablePatchedIconv"), value);
            });
        }
        if (p_enablePatchedIconv != oldValue) {
            Q_EMIT enablePatchedIconvChanged();
        }
    }
Q_SIGNALS:
    void defaultEncodingChanged();
    void disableImproveGB18030Changed();
    void enablePatchedIconvChanged();
private:
    void initialize(DTK_CORE_NAMESPACE::DConfig *config) {
        Q_ASSERT(!m_config.loadRelaxed());
        m_config.storeRelaxed(config);
        if (testPropertySet(0)) {
            config->setValue(QStringLiteral("defaultEncoding"), QVariant::fromValue(p_defaultEncoding));
        } else {
            updateValue(QStringLiteral("defaultEncoding"), QVariant::fromValue(p_defaultEncoding));
        }
        if (testPropertySet(1)) {
            config->setValue(QStringLiteral("disableImproveGB18030"), QVariant::fromValue(p_disableImproveGB18030));
        } else {
            updateValue(QStringLiteral("disableImproveGB18030"), QVariant::fromValue(p_disableImproveGB18030));
        }
        if (testPropertySet(2)) {
            config->setValue(QStringLiteral("enablePatchedIconv"), QVariant::fromValue(p_enablePatchedIconv));
        } else {
            updateValue(QStringLiteral("enablePatchedIconv"), QVariant::fromValue(p_enablePatchedIconv));
        }

        connect(config, &DTK_CORE_NAMESPACE::DConfig::valueChanged, this, [this](const QString &key) {
            updateValue(key);
        }, Qt::DirectConnection);
    }
    void updateValue(const QString &key, const QVariant &fallback = QVariant()) {
        Q_ASSERT(QThread::currentThread() == m_config.loadRelaxed()->thread());
        const QVariant &value = m_config.loadRelaxed()->value(key, fallback);
        if (key == QStringLiteral("defaultEncoding")) {
            auto newValue = qvariant_cast<QString>(value);
            QMetaObject::invokeMethod(this, [this, newValue]() {
                if (p_defaultEncoding != newValue) {
                    p_defaultEncoding = newValue;
                    Q_EMIT defaultEncodingChanged();
                }
            });
            return;
        }
        if (key == QStringLiteral("disableImproveGB18030")) {
            auto newValue = qvariant_cast<bool>(value);
            QMetaObject::invokeMethod(this, [this, newValue]() {
                if (p_disableImproveGB18030 != newValue) {
                    p_disableImproveGB18030 = newValue;
                    Q_EMIT disableImproveGB18030Changed();
                }
            });
            return;
        }
        if (key == QStringLiteral("enablePatchedIconv")) {
            auto newValue = qvariant_cast<bool>(value);
            QMetaObject::invokeMethod(this, [this, newValue]() {
                if (p_enablePatchedIconv != newValue) {
                    p_enablePatchedIconv = newValue;
                    Q_EMIT enablePatchedIconvChanged();
                }
            });
            return;
        }
    }
    inline void markPropertySet(const int index) {
        if (index < 32) {
            m_propertySetStatus0.fetchAndOrOrdered(1 << (index - 0));
            return;
        }
        Q_UNREACHABLE();
    }
    inline bool testPropertySet(const int index) const {
        if (index < 32) {
            return (m_propertySetStatus0.loadRelaxed() & (1 << (index - 0)));
        }
        Q_UNREACHABLE();
    }
    QAtomicPointer<DTK_CORE_NAMESPACE::DConfig> m_config = nullptr;
    QString p_defaultEncoding { QStringLiteral("UTF-8") };
    bool p_disableImproveGB18030 { false };
    bool p_enablePatchedIconv { false };
    QAtomicInteger<quint32> m_propertySetStatus0 = 0;
};

#endif // ORG_DEEPIN_EDITOR_H
