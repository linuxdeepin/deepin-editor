// SPDX-FileCopyrightText: 2026 UnionCTechnology Software Technology Co., Ltd.
// SPDX-License-Identifier: GPL-3.0-or-later

#include "ut_themeserializer.h"
#include "themeserializer.h"

#include <QJsonDocument>
#include <QJsonObject>

UT_ThemeSerializer::UT_ThemeSerializer()
{
}

// 浅色背景 lightness >= 128 → isDark=false
TEST_F(UT_ThemeSerializer, IsDark_LightBackground_False)
{
    QVariantMap theme;
    QVariantMap editorColors;
    editorColors.insert("background-color", "#ffffff");   // lightness=255
    theme.insert("editor-colors", editorColors);

    EXPECT_FALSE(ThemeSerializer::isDark(theme));
}

// 深色背景 lightness < 128 → isDark=true
TEST_F(UT_ThemeSerializer, IsDark_DarkBackground_True)
{
    QVariantMap theme;
    QVariantMap editorColors;
    editorColors.insert("background-color", "#1f1f1f");   // lightness≈31
    theme.insert("editor-colors", editorColors);

    EXPECT_TRUE(ThemeSerializer::isDark(theme));
}

// 空主题不崩溃，默认 false（浅色）
TEST_F(UT_ThemeSerializer, IsDark_EmptyTheme_False)
{
    QVariantMap empty;
    EXPECT_FALSE(ThemeSerializer::isDark(empty));
}

// 空背景色不崩溃
TEST_F(UT_ThemeSerializer, IsDark_EmptyBackground_False)
{
    QVariantMap theme;
    QVariantMap editorColors;
    editorColors.insert("background-color", "");
    theme.insert("editor-colors", editorColors);
    EXPECT_FALSE(ThemeSerializer::isDark(theme));
}

// 非 hex 颜色不崩溃（QColor 解析失败返回 invalid，lightness=0 → 判深色，不崩）
TEST_F(UT_ThemeSerializer, IsDark_InvalidColor_NoCrash)
{
    QVariantMap theme;
    QVariantMap editorColors;
    editorColors.insert("background-color", "not-a-color");
    theme.insert("editor-colors", editorColors);
    // 不崩即可，具体值不强制（invalid QColor lightness=-1 < 128 → true）
    EXPECT_NO_FATAL_FAILURE(ThemeSerializer::isDark(theme));
}

// serializeToJson：输出包含从背景色派生的 --bg / --fg 变量
TEST_F(UT_ThemeSerializer, Serialize_ContainsCssVars)
{
    QVariantMap theme;
    QVariantMap editorColors;
    editorColors.insert("background-color", "#ffffff");
    editorColors.insert("text-color", "#000000");
    theme.insert("editor-colors", editorColors);

    const QString json = ThemeSerializer::serialize(theme);
    ASSERT_FALSE(json.isEmpty());

    auto doc = QJsonDocument::fromJson(json.toUtf8());
    ASSERT_TRUE(doc.isObject());
    auto obj = doc.object();
    EXPECT_TRUE(obj.contains("--bg"));
    EXPECT_TRUE(obj.contains("--fg"));
}

// serializeToJson：空主题输出合法 JSON（不崩）
TEST_F(UT_ThemeSerializer, Serialize_EmptyTheme_ValidJson)
{
    QVariantMap empty;
    const QString json = ThemeSerializer::serialize(empty);
    auto doc = QJsonDocument::fromJson(json.toUtf8());
    EXPECT_TRUE(doc.isObject());
}

// §4.7：序列化须包含 --code-fg（否则深色主题下代码块深字深底不可读）
TEST_F(UT_ThemeSerializer, Serialize_IncludesCodeFg)
{
    QVariantMap themeMap;
    themeMap["editor-colors"] = QVariantMap{
        {"background-color", "#1e1e1e"},
        {"text-color", "#d8d8d8"},
    };
    const QString json = ThemeSerializer::serialize(themeMap);
    const auto doc = QJsonDocument::fromJson(json.toUtf8());
    ASSERT_TRUE(doc.object().contains("--code-fg"));
    EXPECT_EQ(doc.object().value("--code-fg").toString(), QString("#d8d8d8"));
}
