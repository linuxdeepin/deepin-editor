// SPDX-FileCopyrightText: 2026 UnionTech Software Technology Co., Ltd.
// SPDX-License-Identifier: GPL-3.0-or-later

// settingsdialog.cpp 单元测试（无类定义，仅自由函数 GenerateSettingTranslate）
//
// 函数特征：批量 QObject::tr() 翻译条目生成（Basic/Shortcuts/Editor/Advanced 组
// 与全部编码名）。tr() 最终汇聚到 QCoreApplication::translate(context, sourceText,
// disambiguation, n) —— 以 stub 计数 + 内容抽样断言覆盖。
//
// 用例映射：
// - GenerateSettingTranslate → Invoke_TranslatesAllEntries_WithoutCrash
// - GenerateSettingTranslate（幂等/重复调用） → Invoke_Repeatedly_IsStable
//
// 最小清单完成情况：
// | 1 | 每个函数 ≥1 用例 | 完成（唯一自由函数） |
// | 2 | 等价类（首次/重复调用） | 完成 |
// | 3 | 边界值 | N/A（无参数） |
// | 4 | TEST_P | N/A（无输入维度，仅 2 组同断言逻辑不足以强制） |
// | 5 | 分支清单 | 无分支（纯 tr 列表） |
// | 6 | 分支覆盖 | N/A |
// | 7 | 异常路径 | EXPECT_NO_THROW（无 throw 源） |
// | 8 | 负面 | 重复调用稳定性 |
// | 9 | 强异常安全 | 重复调用计数线性增长（2x） |
// | 10 | stub_ext（QCoreApplication::translate 计数） | 完成 |

#include <gtest/gtest.h>

#include <QCoreApplication>
#include <QString>
#include <QtTest/QSignalSpy>

#include "stubext.h"
#include "test_env.h"

// settingsdialog.cpp 未提供头文件，此处补声明
void GenerateSettingTranslate();

namespace {

class GenerateSettingTranslateTest : public ::testing::Test {
protected:
    static void SetUpTestSuite() { controlsut::ensureApp(); }

    void SetUp() override
    {
        stub.clear();
        translateCount = 0;
        lastSourceText.clear();
        sourceTexts.clear();
        // QObject::tr → QCoreApplication::translate(context, sourceText, disambiguation, n)
        // 拦截计数并回传源文（无翻译器环境下的真实行为等价）
        stub.set_lamda(
            static_cast<QString (*)(const char *, const char *, const char *, int)>(
                &QCoreApplication::translate),
            [this](const char *, const char *sourceText, const char *, int) -> QString {
                ++translateCount;
                lastSourceText = QString::fromUtf8(sourceText);
                sourceTexts << lastSourceText;
                return lastSourceText;
            });
    }

    void TearDown() override { stub.clear(); }

    stub_ext::StubExt stub;
    int translateCount = 0;
    QString lastSourceText;
    QStringList sourceTexts;
};

TEST_F(GenerateSettingTranslateTest, Invoke_TranslatesAllEntries_WithoutCrash)
{
    // Act
    EXPECT_NO_THROW(GenerateSettingTranslate());

    // Assert：全量条目经 translate 汇聚（settingsdialog.cpp 内 130+ 条 tr/Q_UNUSED）
    EXPECT_GT(translateCount, 100);
    // 内容抽样：四个分组名与编码名确实进入翻译管线
    EXPECT_TRUE(sourceTexts.contains(QString("Basic")));
    EXPECT_TRUE(sourceTexts.contains(QString("Shortcuts")));
    EXPECT_TRUE(sourceTexts.contains(QString("Editor")));
    EXPECT_TRUE(sourceTexts.contains(QString("Advanced")));
    EXPECT_TRUE(sourceTexts.contains(QString("ChineseSimplified")));
    // 无空源文（负面：任何条目不得以空串注册）
    EXPECT_FALSE(sourceTexts.contains(QString()));
}

TEST_F(GenerateSettingTranslateTest, Invoke_Repeatedly_IsStable)
{
    // Arrange
    GenerateSettingTranslate();
    const int firstRound = translateCount;

    // Act：重复调用（负面场景：多窗口/多次打开设置）
    GenerateSettingTranslate();

    // Assert：计数线性翻倍（每次全量重译，无缓存副作用、无崩溃）
    EXPECT_EQ(translateCount, firstRound * 2);
    EXPECT_GT(firstRound, 100);
}

}  // namespace
