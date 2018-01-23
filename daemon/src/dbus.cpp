#include "dbus.h"
#include "PolicyKitHelper.h"
#include <QDebug>
#include <QtCore/QFile>

dbus::dbus(QObject *parent) :
    QObject(parent){
}

bool dbus::saveFile(QString filepath, QString text) {
    if(PolicyKitHelper::instance()->checkAuthorization("com.deepin.editor.saveFile", getpid())){
        QFile file(filepath);
        if(!file.open(QIODevice::WriteOnly | QIODevice::Text)){
            qDebug() << "Can't write file: " << filepath;

            return false;
        }

        QTextStream out(&file);
        out << text;
        file.close();
        
        return true;
    } else{
        return false;
    }
}
