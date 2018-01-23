#include "dbus_adaptor.h"
#include <QtCore/QMetaObject>
#include <QtCore/QByteArray>
#include <QtCore/QList>
#include <QtCore/QMap>
#include <QtCore/QString>
#include <QtCore/QStringList>
#include <QtCore/QVariant>

DbusAdaptor::DbusAdaptor(QObject *parent)
    : QDBusAbstractAdaptor(parent)
{
    // constructor
    setAutoRelaySignals(true);
}

DbusAdaptor::~DbusAdaptor()
{
    // destructor
}

bool DbusAdaptor::saveFile(const QString &filepath, const QString &text)
{
    // handle method call com.deepin.editor.daemon.saveFile
    bool out0;
    QMetaObject::invokeMethod(parent(), "saveFile", Q_RETURN_ARG(bool, out0), Q_ARG(QString, filepath), Q_ARG(QString, text));
    
    return out0;
}

