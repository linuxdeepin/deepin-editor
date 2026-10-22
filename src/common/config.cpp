// SPDX-FileCopyrightText: 2023-2026 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include "ddlog.h"
#include "config.h"

#include <iconv.h>

#include <QDebug>

#ifdef DTKCORE_CLASS_DConfigFile
#include <QStandardPaths>

const QString g_keyDisableImproveGB18030 = "disableImproveGB18030";
const QString g_keyDefaultEncoding = "defaultEncoding";
const QString g_keyEnablePatchedIconv = "enablePatchedIconv";
#endif

/**
   @brief 检测当前iconv使用的GB18030编码是否为2005标准，2005标准强制使用上层补丁版本
        通过检测2005和2022编码转的差异，以附录D中的编码为例验证
        2005标准 0xFE51 --> \u20087
        2022标准 0xFE51 --> \uE816
   @return iconv使用GB18030编码是否为2005标准，默认返回true
 */
bool detectIconvUse2005Standard()
{
    qCDebug(appLog) << "Detecting iconv use GB18030-2005 standard";
    iconv_t handle = iconv_open("UTF-8", "GB18030");
    if (handle == reinterpret_cast<iconv_t>(-1)) {
        qCWarning(appLog) << "Failed to open iconv handle for GB18030";
        return true;
    }

    QByteArray input("\xFE\x51");
    QByteArray output(input.size() * 2, 0);
    char *inputData = input.data();
    char *outputData = output.data();
    size_t inputLen = static_cast<size_t>(input.count());
    size_t outputLen = static_cast<size_t>(output.count());

    const size_t ret = iconv(handle, &inputData, &inputLen, &outputData, &outputLen);
    iconv_close(handle);

    if (ret == static_cast<size_t>(-1)) {
        qCWarning(appLog) << "Failed to convert GB18030 to UTF-8";
        return true;
    }

    if (!output.contains("\uE816")) {
        qCInfo(appLog) << "Current iconv uses GB18030-2005 standard";
        return true;
    }

    qCInfo(appLog) << "Current iconv uses GB18030-2022 standard";
    return false;
}

/**
   @class Config
   @brief 获取DConfig配置
   @details 取得DConfig配置，目前主要用于配置是否提高GB18030识别率。
    配置文件路径为 /usr/share/dsg/configs/org.deepin.editor/org.deepin.editor.json
 */

Config::Config(QObject *parent)
    : QObject(parent)
    , encoding("UTF-8")
{
    qCDebug(appLog) << "Initializing Config instance";
#ifdef DTKCORE_CLASS_DConfigFile
    dconfig = DConfig::create("org.deepin.editor", "org.deepin.editor");
    if (dconfig->isValid()) {
        qCDebug(appLog) << "DConfig is valid";
        // 默认提高GB18030识别率
        improveGB18030 = !dconfig->value(g_keyDisableImproveGB18030).toBool();
        qCInfo(appLog) << qPrintable("DConfig::ImproveGB18030:") << improveGB18030;

        patchedIconv = dconfig->value(g_keyEnablePatchedIconv).toBool();
        qCInfo(appLog) << qPrintable("DConfig::enablePatchedIconv:") << patchedIconv;

        encoding = dconfig->value(g_keyDefaultEncoding).toByteArray().toUpper();
        qCInfo(appLog) << qPrintable("DConfig::defaultEncoding") << encoding;

        connect(dconfig, &DConfig::valueChanged, this, [this](const QString &key) {
            qCDebug(appLog) << "Config value changed for key:" << key;
            if (key == g_keyDisableImproveGB18030) {
                this->improveGB18030 = !this->dconfig->value(g_keyDisableImproveGB18030).toBool();
                qCInfo(appLog) << qPrintable("DConfig::ImproveGB18030 changed:") << improveGB18030;
                qCDebug(appLog) << "improveGB18030 changed to" << this->improveGB18030;
            } else if (key == g_keyEnablePatchedIconv) {
                this->patchedIconv = dconfig->value(g_keyEnablePatchedIconv).toBool();
                qCInfo(appLog) << qPrintable("DConfig::enablePatchedIconv changed:") << patchedIconv;
                qCDebug(appLog) << "patchedIconv changed to" << this->patchedIconv;
            } else if (key == g_keyDefaultEncoding) {
                this->encoding = dconfig->value(g_keyDefaultEncoding).toByteArray().toUpper();
                qCInfo(appLog) << qPrintable("DConfig::defaultEncoding changed:") << encoding;
                qCDebug(appLog) << "encoding changed to" << this->encoding;
            }
        });

    } else {
        qCWarning(appLog) << qPrintable("org.deepin.editor DConfig not valid!");
    }
#else
    qCWarning(appLog) << "DConfig is not supported by DTK!";
#endif

    // 检测当前iconv使用的GB18030编码是否为2005标准(2005标准强制使用上层补丁版本)
    iocnvUse2005Standard = detectIconvUse2005Standard();
    qCDebug(appLog) << "Leaving Config::Config";
}

Config::~Config()
{
    qCDebug(appLog) << "Destroying Config instance";
#ifdef DTKCORE_CLASS_DConfigFile
    if (dconfig) {
        qCDebug(appLog) << "Deleting dconfig";
        delete dconfig;
    }
#endif
    qCDebug(appLog) << "Leaving Config::~Config";
}

Config *Config::instance()
{
    qCDebug(appLog) << "Getting Config singleton instance";
    static Config config;
    qCDebug(appLog) << "Leaving Config::instance";
    return &config;
}

/**
   @return 返回是否提高GB18030编码识别率
 */
bool Config::enableImproveGB18030() const
{
    qCDebug(appLog) << "Entering Config::enableImproveGB18030, returning" << improveGB18030;
    return improveGB18030;
}

/**
   @return 返回是否使用上层修改的适配2022标准的iconv设置，
    2005标准强制使用上层补丁版本，以适配2022要求
 */
bool Config::enablePatchedIconv() const
{
    qCDebug(appLog) << "Entering Config::enablePatchedIconv, returning" << (patchedIconv || iocnvUse2005Standard);
    return patchedIconv || iocnvUse2005Standard;
}

/**
   @return 返回配置的默认编码，默认UTF-8
 */
QByteArray Config::defaultEncoding() const
{
    qCDebug(appLog) << "Entering Config::defaultEncoding";
    if (encoding.isEmpty()) {
        qCDebug(appLog) << "Encoding is empty, returning UTF-8";
        return QByteArray("UTF-8");
    }
    qCDebug(appLog) << "Returning configured encoding:" << encoding;
    return encoding;
}
