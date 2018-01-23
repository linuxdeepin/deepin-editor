#ifndef DBUS_ADAPTOR_H
#define DBUS_ADAPTOR_H

#include <QtCore/QObject>
#include <QtDBus/QtDBus>
QT_BEGIN_NAMESPACE
class QByteArray;
template<class T> class QList;
template<class Key, class Value> class QMap;
class QString;
class QStringList;
class QVariant;
QT_END_NAMESPACE

class DbusAdaptor: public QDBusAbstractAdaptor
{
    Q_OBJECT
    Q_CLASSINFO("D-Bus Interface", "com.deepin.editor.daemon")
    Q_CLASSINFO("D-Bus Introspection", ""
                "  <interface name=\"com.deepin.editor.daemon\">\n"
                "    <method name=\"saveFile\">\n"
                "      <arg direction=\"out\" type=\"b\"/>\n"
                "      <arg direction=\"in\" type=\"ss\" name=\"text\"/>\n"
                "    </method>\n"
                "  </interface>\n"
                "")
public:
    DbusAdaptor(QObject *parent);
    virtual ~DbusAdaptor();

public: // PROPERTIES
public Q_SLOTS: // METHODS
    bool saveFile(const QString &filepath, const QString &text);
Q_SIGNALS: // SIGNALS
};

#endif
