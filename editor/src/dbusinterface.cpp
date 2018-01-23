#include "dbusinterface.h"

SaveFileInterface::SaveFileInterface(const QString &service, const QString &path, const QDBusConnection &connection, QObject *parent)
    : QDBusAbstractInterface(service, path, staticInterfaceName(), connection, parent)
{
}

SaveFileInterface::~SaveFileInterface()
{
}

