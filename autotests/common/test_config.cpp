// SPDX-FileCopyrightText: 2026 UnionTech Software Technology Co., Ltd.
// SPDX-License-Identifier: GPL-3.0-or-later

#include "../../src/common/config.h"

#include <gtest/gtest.h>

// Config has a private constructor; the test target is built with
// -fno-access-control, so we can construct fresh instances to cover ctor/dtor.
TEST(ConfigTest, ConstructAndDestroy_DoesNotCrash)
{
    Config *cfg = new Config();
    ASSERT_NE(cfg, nullptr);
    delete cfg;
    SUCCEED();
}

TEST(ConfigTest, Instance_ReturnsSameSingleton)
{
    Config *a = Config::instance();
    Config *b = Config::instance();
    ASSERT_NE(a, nullptr);
    EXPECT_EQ(a, b);
}

TEST(ConfigTest, DefaultEncoding_IsNonEmpty)
{
    Config *cfg = Config::instance();
    EXPECT_FALSE(cfg->defaultEncoding().isEmpty());
}

TEST(ConfigTest, EnableImproveGB18030_ReturnsBool)
{
    Config *cfg = Config::instance();
    (void)cfg->enableImproveGB18030();
    SUCCEED();
}

TEST(ConfigTest, EnablePatchedIconv_ReturnsBool)
{
    Config *cfg = Config::instance();
    (void)cfg->enablePatchedIconv();
    SUCCEED();
}
