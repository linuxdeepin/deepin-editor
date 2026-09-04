// SPDX-FileCopyrightText: 2026 UnionTech Software Technology Co., Ltd.
// SPDX-License-Identifier: GPL-3.0-or-later

// ============================================================================
// PathSettingWgt 单元测试（B11 / src/widgets/pathsettintwgt.cpp）
//
// 策略：真实 offscreen 构造 + 真实 Settings 单例（XDG 重定向临时目录，
// qrc settings.json 编入目标）。模态 QFileDialog::exec / selectedFiles 拦截。
//
// 分支清单（来源：pathsettintwgt.cpp）与用例映射：
// - ctor → init() → connections() → onSaveIdChanged(当前配置)
//     → Constructor_BuildsAllChildWidgets
// - onSaveIdChanged：CurFileBox / LastOptBox / CustomBox / default
//     → OnSaveIdChanged_Param（TEST_P 参数化 4 组）
// - setEditText：短文本原样 / 长文本 ElideMiddle
//     → SetEditText_ShortText_Unchanged / SetEditText_LongText_ElidedMiddle
// - onBoxClicked（经 QButtonGroup 信号）：三种选项写回 Settings + 按钮态
//     → OnBoxClicked_CustomBox_UpdatesSettingsAndEnablesButton
//       OnBoxClicked_CurFileBox_UpdatesSettingsAndDisablesButton
// - onBtnClicked（经按钮 clicked）：
//     · 对话框拒绝 → 早退 → OnBtnClicked_DialogRejected_NoChange
//     · 自定义路径不存在 → 默认 Documents 目录分支 + 接受 → 保存路径回写
//       → OnBtnClicked_DefaultPathAndAccepted_UpdatesCustomPath
// - updateSizeMode（ctor 已执行；直接调用复核尺寸）
//     → UpdateSizeMode_ButtonSizeMatchesCurrentMode
// - dtor → Destructor_DelelesSafely
// ============================================================================

#include <gtest/gtest.h>
#include "stubext.h"

#include <QApplication>
#include <QTemporaryDir>
#include <QDir>
#include <QCheckBox>
#include <QLineEdit>
#include <QPushButton>
#include <QButtonGroup>
#include <QFontMetrics>
#include <QFileDialog>
#include <DGuiApplicationHelper>

#include "widgets/pathsettintwgt.h"
#include "common/settings.h"

class PathSettingWgtTest : public ::testing::Test {
protected:
    static void SetUpTestSuite()
    {
        s_configHome = new QTemporaryDir();
        s_dataHome = new QTemporaryDir();
        const QString xdgConfig = s_configHome->filePath("xdg-config");
        const QString xdgData = s_dataHome->filePath("xdg-data");
        QDir().mkpath(xdgConfig);
        QDir().mkpath(xdgData);
        qputenv("XDG_CONFIG_HOME", xdgConfig.toUtf8());
        qputenv("XDG_DATA_HOME", xdgData.toUtf8());

        int argc = 1;
        char *argv[] = {s_argv, nullptr};
        s_app = new QApplication(argc, argv);
        QApplication::setOrganizationName(QStringLiteral("deepin"));
        QApplication::setApplicationName(QStringLiteral("deepin-editor"));

        // 真实 Settings 单例（资源 + 临时 config 落盘）
        Settings::instance();
    }

    static void TearDownTestSuite()
    {
        qunsetenv("XDG_CONFIG_HOME");
        qunsetenv("XDG_DATA_HOME");
    }

    void SetUp() override
    {
        m_tempDir = new QTemporaryDir();
        m_wgt = new PathSettingWgt();
    }

    void TearDown() override
    {
        delete m_wgt;
        m_wgt = nullptr;
        stub.clear();
        delete m_tempDir;
    }

    stub_ext::StubExt stub;
    PathSettingWgt *m_wgt = nullptr;
    QTemporaryDir *m_tempDir = nullptr;

    int dialogExecResult = QDialog::Rejected;
    int dialogExecCalls = 0;
    QStringList selectedFiles;

    static QApplication *s_app;
    static QTemporaryDir *s_configHome;
    static QTemporaryDir *s_dataHome;
    static char s_argv[];
};

QApplication *PathSettingWgtTest::s_app = nullptr;
QTemporaryDir *PathSettingWgtTest::s_configHome = nullptr;
QTemporaryDir *PathSettingWgtTest::s_dataHome = nullptr;
char PathSettingWgtTest::s_argv[] = "test_pathsettintwgt";

// ------------------------------------------------------------
// 构造
// ------------------------------------------------------------

TEST_F(PathSettingWgtTest, Constructor_BuildsAllChildWidgets)
{
    // Assert: 全部子控件按 objectName 就位 + 按钮组三选一
    EXPECT_NE(m_wgt->findChild<QCheckBox *>("CurFileBox"), nullptr);
    EXPECT_NE(m_wgt->findChild<QCheckBox *>("LastOptBox"), nullptr);
    EXPECT_NE(m_wgt->findChild<QCheckBox *>("CustomBox"), nullptr);
    EXPECT_NE(m_wgt->findChild<DLineEdit *>("CustomEdit"), nullptr);
    EXPECT_NE(m_wgt->findChild<QPushButton *>("CustomBtn"), nullptr);
    QButtonGroup *group = m_wgt->findChild<QButtonGroup *>("Group");
    ASSERT_NE(group, nullptr);
    EXPECT_EQ(group->buttons().count(), 3);
    EXPECT_EQ(group->id(group->buttons().at(0)), PathSettingWgt::LastOptBox);
}

// ------------------------------------------------------------
// onSaveIdChanged 参数化
// ------------------------------------------------------------

namespace {
struct SaveIdCase {
    int id;
    const char *expectedChecked; // objectName，nullptr = 无勾选
    bool expectBtnEnabled;
};
} // namespace

class PathSettingSaveIdTest : public PathSettingWgtTest,
                              public ::testing::WithParamInterface<SaveIdCase> {
};

TEST_P(PathSettingSaveIdTest, OnSaveIdChanged_EachId_ChecksMatchingBox)
{
    // Arrange: 先固定到 LastOptBox，隔离持久化配置的影响
    const SaveIdCase c = GetParam();
    m_wgt->onSaveIdChanged(PathSettingWgt::LastOptBox);

    // Act
    m_wgt->onSaveIdChanged(c.id);

    // Assert: 对应选项勾选、其它不勾选，按钮可用性正确
    QCheckBox *cur = m_wgt->findChild<QCheckBox *>("CurFileBox");
    QCheckBox *last = m_wgt->findChild<QCheckBox *>("LastOptBox");
    QCheckBox *custom = m_wgt->findChild<QCheckBox *>("CustomBox");
    QPushButton *btn = m_wgt->findChild<QPushButton *>("CustomBtn");
    const std::string checkedName = c.expectedChecked ? std::string(c.expectedChecked) : std::string();
    EXPECT_EQ(cur->isChecked(), checkedName == std::string("CurFileBox"));
    EXPECT_EQ(last->isChecked(), checkedName == std::string("LastOptBox"));
    EXPECT_EQ(custom->isChecked(), checkedName == std::string("CustomBox"));
    EXPECT_EQ(btn->isEnabled(), c.expectBtnEnabled);

    // CustomBox 分支额外回填编辑框文本（来自 Settings 的自定义路径，经 elide）
    if (c.id == PathSettingWgt::CustomBox) {
        DLineEdit *edit = m_wgt->findChild<DLineEdit *>("CustomEdit");
        const QString expectBase = Settings::instance()->getSavePath(PathSettingWgt::CustomBox);
        const QString expect = QFontMetrics(edit->font())
                                   .elidedText(expectBase, Qt::ElideMiddle, 175);
        EXPECT_EQ(edit->text(), expect);
    }
}

INSTANTIATE_TEST_SUITE_P(SaveIdVariants, PathSettingSaveIdTest,
                         ::testing::Values(
                             SaveIdCase{PathSettingWgt::CurFileBox, "CurFileBox", false},
                             SaveIdCase{PathSettingWgt::LastOptBox, "LastOptBox", false},
                             SaveIdCase{PathSettingWgt::CustomBox, "CustomBox", true},
                             // default 分支语义为"不改动"，Arrange 先置 LastOptBox 再喂 99
                             SaveIdCase{99, "LastOptBox", false}));

// ------------------------------------------------------------
// setEditText
// ------------------------------------------------------------

TEST_F(PathSettingWgtTest, SetEditText_ShortText_Unchanged)
{
    // Arrange
    DLineEdit *edit = m_wgt->findChild<DLineEdit *>("CustomEdit");

    // Act
    m_wgt->setEditText(QStringLiteral("/short/path"));

    // Assert: 短文本不折叠且不含省略号
    EXPECT_EQ(edit->text(), QString("/short/path"));
    EXPECT_FALSE(edit->text().contains(QChar(0x2026)));
}

TEST_F(PathSettingWgtTest, SetEditText_LongText_ElidedMiddle)
{
    // Arrange
    DLineEdit *edit = m_wgt->findChild<DLineEdit *>("CustomEdit");
    QString longPath = QStringLiteral("/very/");
    for (int i = 0; i < 30; ++i)
        longPath += QStringLiteral("long_segment_");
    longPath += QStringLiteral("end.txt");
    const QString expect = QFontMetrics(edit->font()).elidedText(longPath, Qt::ElideMiddle, 175);

    // Act
    m_wgt->setEditText(longPath);

    // Assert: 中部折叠且与直接计算一致
    EXPECT_EQ(edit->text(), expect);
    EXPECT_NE(edit->text(), longPath);
    EXPECT_TRUE(edit->text().contains(QStringLiteral("…")));
}

// ------------------------------------------------------------
// onBoxClicked（经 QButtonGroup 真实信号链）
// ------------------------------------------------------------

TEST_F(PathSettingWgtTest, OnBoxClicked_CustomBox_UpdatesSettingsAndEnablesButton)
{
    // Arrange
    QCheckBox *custom = m_wgt->findChild<QCheckBox *>("CustomBox");
    QPushButton *btn = m_wgt->findChild<QPushButton *>("CustomBtn");

    // Act: 点击自定义路径选项（buttonClicked → lambda → onBoxClicked(CustomBox)）
    custom->click();

    // Assert: Settings 保存路径模式已切换 + 按钮使能
    EXPECT_EQ(Settings::instance()->getSavePathId(), PathSettingWgt::CustomBox);
    EXPECT_TRUE(btn->isEnabled());
}

TEST_F(PathSettingWgtTest, OnBoxClicked_CurFileBox_UpdatesSettingsAndDisablesButton)
{
    // Arrange
    QCheckBox *cur = m_wgt->findChild<QCheckBox *>("CurFileBox");
    QPushButton *btn = m_wgt->findChild<QPushButton *>("CustomBtn");
    m_wgt->onSaveIdChanged(PathSettingWgt::CustomBox);
    ASSERT_TRUE(btn->isEnabled());

    // Act
    cur->click();

    // Assert
    EXPECT_EQ(Settings::instance()->getSavePathId(), PathSettingWgt::CurFileBox);
    EXPECT_FALSE(btn->isEnabled());
}

// ------------------------------------------------------------
// onBtnClicked（经按钮 clicked，QFileDialog 拦截）
// ------------------------------------------------------------

TEST_F(PathSettingWgtTest, OnBtnClicked_DialogRejected_NoChange)
{
    // Arrange: 进入自定义模式并预置已有路径
    m_wgt->onSaveIdChanged(PathSettingWgt::CustomBox);
    const QString preset = m_tempDir->filePath(QStringLiteral("preset"));
    Settings::instance()->setSavePath(PathSettingWgt::CustomBox, preset);
    DLineEdit *edit = m_wgt->findChild<DLineEdit *>("CustomEdit");
    m_wgt->setEditText(preset);
    const QString shownBefore = edit->text();

    stub.set_lamda(VADDR(QDialog, exec), [this]() -> int {
        ++dialogExecCalls;
        return dialogExecResult; // 默认 Rejected
    });
    QPushButton *btn = m_wgt->findChild<QPushButton *>("CustomBtn");

    // Act
    btn->click();

    // Assert: 对话框弹出过一次但被取消，配置与编辑框不变（强异常安全）
    EXPECT_EQ(dialogExecCalls, 1);
    EXPECT_EQ(Settings::instance()->getSavePath(PathSettingWgt::CustomBox), preset);
    EXPECT_EQ(edit->text(), shownBefore);
}

TEST_F(PathSettingWgtTest, OnBtnClicked_DefaultPathAndAccepted_UpdatesCustomPath)
{
    // Arrange: 自定义路径为不存在路径 → 走默认 Documents 目录分支；接受并选中新目录
    m_wgt->onSaveIdChanged(PathSettingWgt::CustomBox);
    Settings::instance()->setSavePath(PathSettingWgt::CustomBox,
                                      QStringLiteral("/nonexistent/somewhere"));
    const QString chosen = m_tempDir->filePath(QStringLiteral("chosen-dir"));
    ASSERT_TRUE(QDir().mkpath(chosen));

    stub.set_lamda(VADDR(QDialog, exec), [this]() -> int {
        ++dialogExecCalls;
        return dialogExecResult;
    });
    dialogExecResult = QDialog::Accepted;
    selectedFiles = QStringList{chosen};
    stub.set_lamda(VADDR(QFileDialog, selectedFiles),
                   [this](QFileDialog *) -> QStringList { return selectedFiles; });
    QPushButton *btn = m_wgt->findChild<QPushButton *>("CustomBtn");
    DLineEdit *edit = m_wgt->findChild<DLineEdit *>("CustomEdit");
    const QString expectShown = QFontMetrics(edit->font()).elidedText(chosen, Qt::ElideMiddle, 175);

    // Act
    btn->click();

    // Assert: 自定义路径回写为选中目录，编辑框同步（按 elide 规则折叠）
    EXPECT_EQ(dialogExecCalls, 1);
    EXPECT_EQ(Settings::instance()->getSavePath(PathSettingWgt::CustomBox), chosen);
    EXPECT_EQ(edit->text(), expectShown);
}

// ------------------------------------------------------------
// updateSizeMode / 析构
// ------------------------------------------------------------

TEST_F(PathSettingWgtTest, UpdateSizeMode_ButtonSizeMatchesCurrentMode)
{
    // Arrange
    QPushButton *btn = m_wgt->findChild<QPushButton *>("CustomBtn");
    const int expectBtn = DGuiApplicationHelper::isCompactMode() ? 24 : 36;
    const int expectIcon = DGuiApplicationHelper::isCompactMode() ? 16 : 24;

    // Act（ctor 已调用过，这里直接复核当前模式分支的尺寸结果）
    m_wgt->updateSizeMode();

    // Assert: 与当前布局模式的固定尺寸一致
    EXPECT_EQ(btn->width(), expectBtn);
    EXPECT_EQ(btn->height(), expectBtn);
    EXPECT_EQ(btn->iconSize().width(), expectIcon);
    EXPECT_EQ(btn->iconSize().height(), expectIcon);
}

TEST_F(PathSettingWgtTest, Destructor_DelelesSafely)
{
    // Arrange: 堆上再建一个
    PathSettingWgt *extra = new PathSettingWgt();

    // Act
    delete extra;

    // Assert: 无崩溃（真实覆盖析构函数）；夹具内对象由 TearDown 覆盖第二次析构
    EXPECT_NE(m_wgt, nullptr);
    EXPECT_TRUE(m_wgt->findChild<QCheckBox *>("CurFileBox") != nullptr);
}
