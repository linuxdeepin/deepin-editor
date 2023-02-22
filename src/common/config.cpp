// SPDX-FileCopyrightText: 2023 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include "config.h"

#include <QDebug>

#ifdef DTKCORE_CLASS_DConfigFile
#include <QStandardPaths>

const QString g_keyDisableImproveGB18030 = "disableImproveGB18030";
#endif

/**
   @class Config
   @brief 获取DConfig配置
   @details 取得DConfig配置，目前主要用于配置是否提高GB18030识别率。
    配置文件路径为 /usr/share/dsg/configs/org.deepin.editor/org.deepin.editor.json
 */

Config::Config(QObject *parent)
    : QObject(parent)
{
#ifdef DTKCORE_CLASS_DConfigFile
    dconfig = DConfig::create("org.deepin.editor", "org.deepin.editor");
    if (dconfig->isValid()) {
        // 默认提高GB18030识别率
        improveGB18030 = !dconfig->value(g_keyDisableImproveGB18030).toBool();
        qInfo() << "DConfig::ImproveGB18030:" << improveGB18030;

        connect(dconfig, &DConfig::valueChanged, this, [this](const QString &key) {
            if (key == g_keyDisableImproveGB18030) {
                this->improveGB18030 = !this->dconfig->value(g_keyDisableImproveGB18030).toBool();
                qInfo() << "DConfig::ImproveGB18030 Changed:" << improveGB18030;
            }
        });
    } else {
        qWarning() << "org.deepin.editor DConfig not valid!";
    }
#else
    qWarning() << "DConfig is not supported by DTK!";
#endif
}

Config::~Config()
{
#ifdef DTKCORE_CLASS_DConfigFile
    if (dconfig) {
        delete dconfig;
    }
#endif
}

Config *Config::instance()
{
    static Config config;
    return &config;
}

bool Config::enableImproveGB18030() const
{
    return improveGB18030;
}
