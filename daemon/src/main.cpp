#include <QtDBus/QDBusConnection>
#include <QCoreApplication>
#include "dbus_adaptor.h"
#include "dbus.h"

int main(int argc, char **argv)
{
    QCoreApplication app(argc, argv);

    dbus *dbus1 = new dbus;
    new DbusAdaptor(dbus1);

    QDBusConnection connection = QDBusConnection::systemBus();
    if (connection.isConnected()) {
        qDebug() << "Build deepin-editor daemon success.";
    }
    
    if(!connection.registerService("com.deepin.editor.daemon") || !connection.registerObject("/", dbus1)){
        qDebug() << connection.lastError();
        app.exit(1);
        
        return 1;
    }

    return app.exec();
}
