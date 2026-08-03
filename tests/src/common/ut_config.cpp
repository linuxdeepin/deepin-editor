// SPDX-FileCopyrightText: 2026 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include "../../src/common/config.h"

#include <gtest/gtest.h>
#include <QDBusInterface>

// Cover Config::Config(QObject*) and Config::~Config() by constructing and
// destructing a fresh instance (constructor is private, accessible via
// -fno-access-control compile flag used by the test target).
TEST(UT_Config, Destructor)
{
    Config *cfg = new Config();
    ASSERT_NE(cfg, nullptr);

    delete cfg;
    SUCCEED();
}

// Cover Config::instance() singleton access path.
TEST(UT_Config, Instance)
{
    Config *cfg = Config::instance();
    ASSERT_NE(cfg, nullptr);
    EXPECT_EQ(Config::instance(), cfg);
}

// Cover Config::enableImproveGB18030 / enablePatchedIconv / defaultEncoding getters.
TEST(UT_Config, Getters)
{
    Config *cfg = Config::instance();
    EXPECT_FALSE(cfg->defaultEncoding().isEmpty());
}

// Cover the lambda connected to DConfig::valueChanged inside the constructor.
// dconfig member is private; accessible via -fno-access-control.
#ifdef DTKCORE_CLASS_DConfigFile
TEST(UT_Config, ValueChangedLambda)
{
    Config *cfg = Config::instance();
    if (cfg->dconfig && cfg->dconfig->isValid()) {
        QMetaObject::invokeMethod(cfg->dconfig, "valueChanged",
                                  Qt::DirectConnection,
                                  Q_ARG(QString, "disableImproveGB18030"));
        QMetaObject::invokeMethod(cfg->dconfig, "valueChanged",
                                  Qt::DirectConnection,
                                  Q_ARG(QString, "enablePatchedIconv"));
        QMetaObject::invokeMethod(cfg->dconfig, "valueChanged",
                                  Qt::DirectConnection,
                                  Q_ARG(QString, "defaultEncoding"));
    }
    SUCCEED();
}
#endif
