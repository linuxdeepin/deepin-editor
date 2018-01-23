#ifndef DEEPIN_EDITOR_DAEMON_DBUS_H
#define DEEPIN_EDITOR_DAEMON_DBUS_H

#include <QtCore/QObject>

class dbus : public QObject{
    Q_OBJECT
    Q_CLASSINFO("D-Bus Interface","com.deepin.editor.daemon")

    public:
    dbus(QObject* parent=0);


public Q_SLOTS:
    bool saveFile(QString filepath, QString text);
};


#endif //DEEPIN_EDITOR_DAEMON_DBUS_H
