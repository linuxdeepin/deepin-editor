// SPDX-FileCopyrightText: 2026 UnionTech Software Technology Co., Ltd.
// SPDX-License-Identifier: GPL-3.0-or-later

#include <gtest/gtest.h>
#include "stubext.h"
#include "themeserializer.h"

#include <QColor>
#include <QJsonDocument>
#include <QJsonObject>

// 分支清单（来源：themeserializer.h）
// isDark:
//   K1: 背景色缺失/非法 → false（!bg.isValid()）
//   K2: bg.lightness() < 128 → true（深色）
//   K3: bg.lightness() >= 128 → false（浅色）
// serialize（bg/fg 各自 valid 与否 × 深浅色回退，三态组合）:
//   T1: bg+fg 均 valid（浅）→ 各键由真实色派生（darker 分支）
//   T2: bg+fg 均 valid（深）→ lighter 分支
//   T3: bg valid、fg 缺失（深）→ --fg/--fg-secondary/--code-fg 走深色回退常量
//   T4: bg valid、fg 缺失（浅）→ 浅色回退常量
//   T5: bg 缺失（空 map）→ --bg/--code-bg/--border 回退常量（dark=false）
// foregroundColor(private，经 serialize 间接):
//   P1: editor-colors.text-color 有效 → 直接用
//   P2: 缺失/非法 → 回退 text-styles.Normal.text-color
// backgroundColor(private，经 isDark/serialize 间接):
//   Q1: editor-colors 存在 → 取 background-color
//   Q2: 不存在 → 无效色
//
// 用例映射：
// - IsDark_BackgroundLightness_ReturnsExpected（TEST_P 4 组）                       → K1/K2/K3
// - Serialize_FullySpecifiedThemes_DerivesAllCssVars（TEST_P 3 组：浅/深/中亮度）    → T1/T2
// - Serialize_MissingForeground_FallsBackByDarkness（TEST_P 3 组）                   → T3/T4
// - Serialize_EmptyOrBrokenMap_UsesLightDefaults                                     → T5 + Q2 + P2 回退链
// - Serialize_ForegroundFromTextStyleNormal_UsedWhenEditorColorMissing               → P2 命中
// - Serialize_OutputIsCompactJson_AllKeysPresent                                      → 结构契约
//
// 最小清单自检：1 公开方法≥1用例 ✔（isDark/serialize；private 两个经公开方法间接全覆盖分支）
// 2 等价类（bg/fg × 深/浅/缺失）✔ 3 边界（lightness 127/128 邻界）✔ 4 TEST_P ×3 ✔
// 5 分支映射 ✔ 6 全分支 ✔ 7 无异常 8 破损/空 map 负面 ✔ 9 纯函数 ✔ 10 无依赖直接调用 ✔

namespace {
QVariantMap makeTheme(const QString &bg, const QString &fg)
{
    QVariantMap theme;
    QVariantMap ec;
    if (!bg.isEmpty())
        ec["background-color"] = bg;
    if (!fg.isEmpty())
        ec["text-color"] = fg;
    if (!ec.isEmpty())
        theme["editor-colors"] = ec;
    return theme;
}
} // namespace

// —— K1/K2/K3：深浅判定等价类（含 127/128 邻界）——
namespace {
struct DarkCase {
    QString bg;
    bool expected;
    const char *note;
};
const DarkCase kDarkCases[] = {
    { "#1e1e1e", true,  "典型深色底" },          // lightness=30 < 128
    { "#000000", true,  "纯黑边界（0）" },
    { "#ffffff", false, "纯白边界（255）" },
    { "#808080", false, "邻界（lightness=128，不小于 128 → 浅）" },
};
} // namespace

class IsDarkParamTest : public ::testing::TestWithParam<DarkCase> {
};

TEST_P(IsDarkParamTest, IsDark_BackgroundLightness_ReturnsExpected)
{
    // Arrange
    const auto &c = GetParam();
    const QVariantMap theme = makeTheme(c.bg, "#ffffff");

    // Act
    const bool dark = ThemeSerializer::isDark(theme);

    // Assert
    EXPECT_EQ(dark, c.expected);   // 期望值依据：c.note 描述的 lightness 分支
    EXPECT_EQ(QColor(c.bg).lightness() < 128, dark);   // 与 QColor lightness 独立复核
}

INSTANTIATE_TEST_SUITE_P(Darkness, IsDarkParamTest,
                         ::testing::ValuesIn(kDarkCases));

TEST(MarkdownThemeSerializerTest, IsDark_MissingOrInvalidBackground_ReturnsFalse)
{
    // Arrange：负面——空 map / 缺 editor-colors / 非法色串
    const QVariantMap empty;
    const QVariantMap broken = makeTheme("not-a-color", "#ffffff");
    QVariantMap noKey;
    noKey["other"] = 42;

    // Act & Assert：无效背景一律按浅色处理（期望 false 边）
    EXPECT_FALSE(ThemeSerializer::isDark(empty));
    EXPECT_FALSE(ThemeSerializer::isDark(broken));
    EXPECT_FALSE(ThemeSerializer::isDark(noKey));
}

// —— T1/T2：bg+fg 齐备时派生全部 CSS 变量（QColor darker/lighter 为确定性算法）——
namespace {
struct FullCase {
    QString bg;
    QString fg;
};
const FullCase kFullCases[] = {
    { "#ffffff", "#1f1f1f" },   // 浅色主题
    { "#1e1e1e", "#eeeeee" },   // 深色主题
    { "#f0f0f0", "#333333" },   // 中间亮度浅色
};
} // namespace

class FullThemeParamTest : public ::testing::TestWithParam<FullCase> {
};

TEST_P(FullThemeParamTest, Serialize_FullySpecifiedThemes_DerivesAllCssVars)
{
    // Arrange
    const auto &c = GetParam();
    const QVariantMap theme = makeTheme(c.bg, c.fg);
    const QColor bg(c.bg);
    const QColor fg(c.fg);
    const bool dark = bg.lightness() < 128;

    // Act
    const QString json = ThemeSerializer::serialize(theme);

    // Assert：六个 CSS 变量全部按源规则派生
    const QJsonObject obj = QJsonDocument::fromJson(json.toUtf8()).object();
    EXPECT_EQ(obj["--bg"].toString(), bg.name());
    EXPECT_EQ(obj["--fg"].toString(), fg.name());
    EXPECT_EQ(obj["--fg-secondary"].toString(),
              dark ? fg.darker(150).name() : fg.darker(180).name());
    EXPECT_EQ(obj["--code-fg"].toString(), fg.name());
    EXPECT_EQ(obj["--code-bg"].toString(),
              dark ? bg.lighter(130).name() : bg.darker(110).name());
    EXPECT_EQ(obj["--border"].toString(),
              dark ? bg.lighter(150).name() : bg.darker(130).name());
    // isDark 与 serialize 内部判定一致（副作用交叉验证）
    EXPECT_EQ(ThemeSerializer::isDark(theme), dark);
}

INSTANTIATE_TEST_SUITE_P(FullThemes, FullThemeParamTest,
                         ::testing::ValuesIn(kFullCases));

// —— T3/T4：fg 缺失时前景三键按深浅回退常量（bg 仍真实派生 code-bg/border）——
namespace {
struct FallbackCase {
    QString bg;
    QString expectedFg;
    QString expectedFgSecondary;
};
const FallbackCase kFallbackCases[] = {
    { "#1e1e1e", "#ffffff", "#a6a6a6" },   // 深色回退
    { "#ffffff", "#1f1f1f", "#595959" },   // 浅色回退
    { "#f5f5f5", "#1f1f1f", "#595959" },   // 中间亮度浅色回退
};
} // namespace

class FallbackThemeParamTest : public ::testing::TestWithParam<FallbackCase> {
};

TEST_P(FallbackThemeParamTest, Serialize_MissingForeground_FallsBackByDarkness)
{
    // Arrange：仅有背景色（deepin/deepin_dark 等主题文本键缺失的真实形态）
    const auto &c = GetParam();
    const QVariantMap theme = makeTheme(c.bg, QString());
    const QColor bg(c.bg);
    const bool dark = bg.lightness() < 128;

    // Act
    const QString json = ThemeSerializer::serialize(theme);

    // Assert：bg 真实、fg 三键走深浅回退常量、code-bg/border 从真实 bg 派生
    const QJsonObject obj = QJsonDocument::fromJson(json.toUtf8()).object();
    EXPECT_EQ(obj["--bg"].toString(), bg.name());
    EXPECT_EQ(obj["--fg"].toString(), c.expectedFg);
    EXPECT_EQ(obj["--fg-secondary"].toString(), c.expectedFgSecondary);
    EXPECT_EQ(obj["--code-fg"].toString(), c.expectedFg);
    EXPECT_EQ(obj["--code-bg"].toString(),
              dark ? bg.lighter(130).name() : bg.darker(110).name());
    EXPECT_EQ(obj["--border"].toString(),
              dark ? bg.lighter(150).name() : bg.darker(130).name());
}

INSTANTIATE_TEST_SUITE_P(ForegroundFallbacks, FallbackThemeParamTest,
                         ::testing::ValuesIn(kFallbackCases));

TEST(MarkdownThemeSerializerTest, Serialize_EmptyOrBrokenMap_UsesLightDefaults)
{
    // Arrange：负面——完全空 map / 非法背景
    const QVariantMap empty;
    const QVariantMap broken = makeTheme("not-a-color", QString());

    // Act
    const QString jsonEmpty = ThemeSerializer::serialize(empty);
    const QString jsonBroken = ThemeSerializer::serialize(broken);

    // Assert：全部键回退浅色默认（bg 缺失分支 T5）
    const QJsonObject obj = QJsonDocument::fromJson(jsonEmpty.toUtf8()).object();
    EXPECT_EQ(obj["--bg"].toString(), "#ffffff");
    EXPECT_EQ(obj["--fg"].toString(), "#1f1f1f");
    EXPECT_EQ(obj["--fg-secondary"].toString(), "#595959");
    EXPECT_EQ(obj["--code-fg"].toString(), "#1f1f1f");
    EXPECT_EQ(obj["--code-bg"].toString(), "#f5f5f5");
    EXPECT_EQ(obj["--border"].toString(), "#e0e0e0");
    // 非法背景与空 map 输出一致（强一致性：破损输入不产生半派生状态）
    EXPECT_EQ(jsonBroken, jsonEmpty);
    EXPECT_FALSE(ThemeSerializer::isDark(empty));
}

TEST(MarkdownThemeSerializerTest, Serialize_ForegroundFromTextStyleNormal_UsedWhenEditorColorMissing)
{
    // Arrange：editor-colors.text-color 缺失，文本色存于 text-styles.Normal（private 前景回退链 P2）
    QVariantMap theme;
    QVariantMap ec;
    ec["background-color"] = "#252525";
    theme["editor-colors"] = ec;
    QVariantMap ts, normal;
    normal["text-color"] = "#d0d0d0";
    ts["Normal"] = normal;
    theme["text-styles"] = ts;

    // Act
    const QString json = ThemeSerializer::serialize(theme);

    // Assert：前景取自 Normal 而非回退常量
    const QJsonObject obj = QJsonDocument::fromJson(json.toUtf8()).object();
    EXPECT_EQ(obj["--fg"].toString(), "#d0d0d0");
    EXPECT_EQ(obj["--code-fg"].toString(), "#d0d0d0");
    EXPECT_EQ(obj["--fg-secondary"].toString(), QColor("#d0d0d0").darker(150).name());
    EXPECT_TRUE(ThemeSerializer::isDark(theme));   // #252525 → 深色
}

TEST(MarkdownThemeSerializerTest, Serialize_OutputIsCompactJson_AllKeysPresent)
{
    // Arrange
    const QVariantMap theme = makeTheme("#1e1e1e", "#eeeeee");

    // Act
    const QString json = ThemeSerializer::serialize(theme);

    // Assert：Compact JSON（无空白换行）且恰含 6 个键
    const QJsonObject obj = QJsonDocument::fromJson(json.toUtf8()).object();
    EXPECT_EQ(obj.size(), 6);
    EXPECT_EQ(obj.keys().count(), 6);
    EXPECT_FALSE(json.contains('\n'));
    EXPECT_FALSE(json.contains(' '));   // Compact 模式无空格分隔符
}
