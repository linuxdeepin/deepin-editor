// SPDX-FileCopyrightText: 2026 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include <gtest/gtest.h>
#include "../../src/thememodule/themelistmodel.h"
#include "src/stub.h"
#include <QDir>
#include <QFile>
#include <QFileInfoList>
#include <QModelIndex>

// 临时主题数据，用于触发 initThemes 加载与排序 lambda
static const char *g_lightThemeJson =
    "{\"metadata\":{\"name\":\"Light\"},\"editor-colors\":{\"background-color\":\"#ffffff\"}}";
static const char *g_darkThemeJson =
    "{\"metadata\":{\"name\":\"Dark\"},\"editor-colors\":{\"background-color\":\"#000000\"}}";
static QFileInfoList g_testThemesList;

QFileInfoList entryInfoListThemesStub()
{
    return g_testThemesList;
}

// 创建临时主题文件，返回路径列表
static QStringList prepareTempThemes(const QString &subDir)
{
    QString tmpDir = QDir::tempPath() + "/" + subDir;
    QDir().mkpath(tmpDir);

    QString lightPath = tmpDir + "/light.theme";
    QString darkPath = tmpDir + "/dark.theme";

    QFile f1(lightPath);
    f1.open(QIODevice::WriteOnly);
    f1.write(g_lightThemeJson);
    f1.close();

    QFile f2(darkPath);
    f2.open(QIODevice::WriteOnly);
    f2.write(g_darkThemeJson);
    f2.close();

    return {lightPath, darkPath};
}

// setFrameColor
TEST(UT_ThemeListModel, setFrameColor)
{
    ThemeListModel model;
    model.setFrameColor("#aabbcc", "#112233");
    EXPECT_EQ(model.m_frameSelectedColor, QString("#aabbcc"));
    EXPECT_EQ(model.m_frameNormalColor, QString("#112233"));
}

// ~ThemeListModel
TEST(UT_ThemeListModel, Destructor)
{
    ThemeListModel *model = new ThemeListModel;
    delete model;
    SUCCEED();
}

// data() 各 role 与 initThemes 排序 lambda（需加载主题）
TEST(UT_ThemeListModel, data_and_initThemes)
{
    QStringList paths = prepareTempThemes("de_themes_ut_model");
    g_testThemesList.clear();
    g_testThemesList << QFileInfo(paths[0]) << QFileInfo(paths[1]);

    Stub s;
    s.set((QFileInfoList(QDir::*)(QDir::Filters, QDir::SortFlags) const) ADDR(QDir, entryInfoList),
          entryInfoListThemesStub);

    ThemeListModel *model = new ThemeListModel;
    // 加载到 >=2 个主题，确保 initThemes 中的排序 lambda 被执行
    EXPECT_GE(model->m_themes.size(), 2);

    QModelIndex idx0 = model->index(0, 0);
    EXPECT_FALSE(model->data(idx0, ThemeListModel::ThemeName).toString().isEmpty());
    EXPECT_FALSE(model->data(idx0, ThemeListModel::ThemePath).toString().isEmpty());
    // ThemeMap 在 data() 中无对应 case，走 default 返回空 QVariant
    EXPECT_EQ(model->data(idx0, ThemeListModel::ThemeMap), QVariant());

    model->setFrameColor("#aabbcc", "#112233");
    EXPECT_EQ(model->data(idx0, ThemeListModel::FrameNormalColor).toString(), QString("#112233"));
    EXPECT_EQ(model->data(idx0, ThemeListModel::FrameSelectedColor).toString(), QString("#aabbcc"));

    // 未知 role，走 default
    EXPECT_EQ(model->data(idx0, 9999), QVariant());

    EXPECT_GE(model->rowCount(QModelIndex()), 2);

    delete model;
}
