#include "dbus.h"
#include "utils.h"
#include "PolicyKitHelper.h"
#include <QDebug>
#include <QFileInfo>
#include <QDir>
#include <QtCore/QFile>

dbus::dbus(QObject *parent) :
    QObject(parent){
}

bool dbus::saveFile(QString filepath, QString text) {
    if(PolicyKitHelper::instance()->checkAuthorization("com.deepin.editor.saveFile", getpid())){
        // Create file if filepath is not exists.
        if (!Utils::fileExists(filepath)) {
            QString directory = QFileInfo(filepath).dir().absolutePath();

            QDir().mkpath(directory);
            if (QFile(filepath).open(QIODevice::ReadWrite)) {
                qDebug() << QString("File %1 not exists, create one.").arg(filepath);
            }
        }
        
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
