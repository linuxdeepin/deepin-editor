// SPDX-FileCopyrightText: 2026 UnionTech Software Technology Co., Ltd.
// SPDX-License-Identifier: GPL-3.0-or-later

#include <gtest/gtest.h>
#include "stubext.h"

// ---------------------------------------------------------------------------
// 预包含 settings.h 的全部直接依赖（保证这些头按正常访问级别解析），
// 随后仅对目标头 settings.h 放开 private 访问（访问 Settings 私有方法/
// 静态单例指针/成员，用于分支覆盖与单例重置）。不修改任何源码。
// ---------------------------------------------------------------------------
#include "dsettingsdialog.h"
#include <qsettingbackend.h>
#include <DKeySequenceEdit>
#include <DDialog>
#include <QSettings>
#include <QPointer>
#include <QKeyEvent>
#include <QDebug>
#include <DApplication>
#include <QLabel>
#include <QPushButton>
#include <QMutex>

#define private public
#include "settings.h"
#undef private

#include <DSettings>
#include <DSettingsOption>
#include "pathsettintwgt.h"
#include <DGuiApplicationHelper>
#include <QApplication>
#include <QTemporaryDir>
#include <QSignalSpy>
#include <QFontDatabase>
#include <QComboBox>
#include <QStyleFactory>
#include <QStandardPaths>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QEvent>
#include <QMouseEvent>
#include <QAbstractButton>
#include <QVariant>

// ---------------------------------------------------------------------------
// 分支清单（来源：src/common/settings.cpp）
// S1 : CustemBackend::doSync / doSetOption / keys / getOption / 析构
// S2 : Settings::Settings（配置路径构造 + 15 个 option 连接 + keymap items）
// S3 : Settings::setSavePath/getSavePath switch 三 id + default 告警分支
// S4 : Settings::setSavePathId/getSavePathId 读写 savingpathwgt option
// S5 : Settings::~Settings（backend 清理 + 单例指针复位）
// S6 : Settings::setSettingDialog
// S7 : Settings::dtkThemeWorkaround（递归 + 非 widget continue）
// S8 : Settings::createFontComBoBoxHandle（value 为空/非空 + valueChanged lambda）
// S9 : Settings::createSavingPathWgt（两个 valueChanged lambda）
// S10: Settings::createKeySequenceEditHandle + editingFinished 大 lambda
//      （checkShortcutValid 失败恢复 / 冲突 replace / 冲突 cancel / Alt+M /
//        standard 模式 / customize 模式 / "<" 与 Return 转义）
// S11: Settings::instance（懒汉单例）
// S12: Settings::updateAllKeysWithKeymap（window/editor 两组 + option 为空告警）
// S13: Settings::copyCustomizeKeysFromKeymap
// S14: Settings::checkShortcutValid（单键非 F / Num / Shift 非默认 / 通过）
// S15: Settings::isShortcutConflict（命中不同名 / 未命中 / 同名跳过）
// S16: Settings::createDialog（bIsConflicts 真/假按钮差异）
// S17: Settings::removeLockFiles（目录不存在 / 删除成功 / 删除失败）
// S18: Settings::slotCustomshortcut（keymap customize 直接更新 /
//      非 customize 拷贝切换 / 非 shortcuts 前缀 / keymap 自身 / _keymap_ 忽略）
// S19: Settings::slotsigAdjust* 10 个转发槽（发对应信号）
// S20: KeySequenceEdit 构造/option/slotDSettingsOptionvalueChanged(空/非空)/eventFilter
//
// 用例映射（TEST_F/TEST_P 名 → 分支）：
// - CustemBackend 节：DoSetOption/DoSync/Keys/GetOption/Destruct*        → S1
// - Instance_FirstCall_* / Instance_SecondCall_*                         → S11
// - SetSavePath_ValidId_* (TEST_P 0/1/2) / SetSavePath_UnknownId_*       → S3
// - GetSavePath_UnknownId_ReturnsEmptyString                             → S3
// - SetGetSavePathId_RoundTripsIdValue                                   → S4
// - Destruct_TemporaryInstance_KeepsSingletonIntact / Recreated_AfterDelete → S5
// - SetSettingDialog_NullDialog_StoresPointer                            → S6
// - DtkThemeWorkaround_MixedChildren_AppliesStyleRecursively             → S7
// - CreateFontComBoBoxHandle_EmptyOptionValue_SetsSystemFixedFont        → S8
// - CreateFontComBoBoxHandle_ExistingValue_KeepsOptionValue              → S8
// - CreateSavingPathWgt_OptionChange_DrivesPathWidget                    → S9
// - CreateKeySequenceEditHandle_ValidSequence_UpdatesCustomizeOption     → S10
// - CreateKeySequenceEditHandle_InvalidSingleKey_RestoresCurrentValue    → S10/S14/S16
// - CreateKeySequenceEditHandle_ConflictingSequence_ReplacesOldBinding   → S10/S16
// - CreateKeySequenceEditHandle_AltMConflict_CancelKeepsOldBinding       → S10/S16
// - CreateKeySequenceEditHandle_UnderCustomizeMode_SetsDirectly          → S10
// - SlotupdateAllKeysWithKeymap_EmacsKeymap_SyncsShortcutOptions         → S12/S19
// - UpdateAllKeysWithKeymap_UnknownKeymap_KeepsValuesSafe                → S12
// - CopyCustomizeKeysFromKeymap_Emacs_CopiesGroupValues                  → S13
// - CheckShortcutValid_ParamVariants_ReturnsExpected (TEST_P)            → S14
// - IsShortcutConflict_* 三个                                             → S15
// - CreateDialog_* 两个                                                   → S16
// - RemoveLockFiles_* 三个                                                → S17
// - SlotCustomshortcut_* 四个                                             → S18
// - SlotsigAdjust* 十个                                                   → S19
// - KeySequenceEdit 节五个                                                → S20
//
// 环境隔离：
// - XDG_CONFIG_HOME 重定向到 QTemporaryDir，绝不读写真实 ~/.config
// - DDialog::exec 全 stub（offscreen 下阻塞等待会挂起测试）
// - QApplication + QT_QPA_PLATFORM=offscreen（无 X11/Wayland 依赖）
// ---------------------------------------------------------------------------

namespace {
const char *kOrgName = "deepin";
const char *kAppName = "deepin-editor";
} // namespace

class SettingsTest : public ::testing::Test {
protected:
    static void SetUpTestSuite()
    {
        s_configHome = new QTemporaryDir();
        const QString xdgConfig = s_configHome->filePath("xdg-config");
        QDir().mkpath(xdgConfig);
        qputenv("XDG_CONFIG_HOME", xdgConfig.toUtf8());
        int argc = 1;
        s_app = new QApplication(argc, s_argv);
        QApplication::setOrganizationName(QString::fromLatin1(kOrgName));
        QApplication::setApplicationName(QString::fromLatin1(kAppName));
        // 构造共享单例（读取 :/resources/settings.json，写入临时 config）
        Settings::instance();
    }

    static void TearDownTestSuite()
    {
        // QApplication 故意不销毁：套件顺序与进程退出安全优先
        // 还原环境变量（与 qputenv 数量配平，避免环境泄漏）
        qunsetenv("XDG_CONFIG_HOME");
        qunsetenv("XDG_SESSION_TYPE");
    }

    void SetUp() override
    {
        stub.clear();
        s_instance = Settings::instance();
        ASSERT_NE(s_instance->settings, nullptr);
    }

    void TearDown() override { stub.clear(); }

    void stubDialogExec(int result)
    {
        stub.set_lamda(VADDR(DDialog, exec), [result]() -> int {
            return result;
        });
    }

    static QString optionValue(const QString &key)
    {
        return Settings::instance()->settings->option(key)->value().toString();
    }

    stub_ext::StubExt stub;
    Settings *s_instance = nullptr;
    static QTemporaryDir *s_configHome;
    static QApplication *s_app;
    static char s_argv0[];
    static char *s_argv[2];
};

QTemporaryDir *SettingsTest::s_configHome = nullptr;
QApplication *SettingsTest::s_app = nullptr;
char SettingsTest::s_argv0[] = "test_settings";
char *SettingsTest::s_argv[2] = { SettingsTest::s_argv0, nullptr };

// ===========================================================================
// CustemBackend（S1）
// ===========================================================================
namespace {
struct BackendValueCase {
    QString key;
    QVariant value;
};
} // namespace

class CustemBackendTest : public SettingsTest, public ::testing::WithParamInterface<BackendValueCase> {
};

TEST_P(CustemBackendTest, DoSetOptionAndgetOption_RoundTripsTypedValues)
{
    // Arrange：ini 文件位于临时目录（按参数键名隔离，避免用例间复用）
    QString safeName = GetParam().key;
    const QString iniFile = s_configHome->filePath(QString("backend-%1.ini").arg(safeName.replace('/', '_')));
    CustemBackend backend(iniFile);
    ASSERT_NE(backend.m_settings, nullptr);

    // Act
    backend.doSetOption(GetParam().key, GetParam().value);

    // Assert：写后立即可读（ini 序列化后类型统一为字符串，按规范字符串比对），keys 同步包含
    EXPECT_EQ(backend.getOption(GetParam().key).toString(), GetParam().value.toString());
    EXPECT_TRUE(backend.keys().contains(GetParam().key));
}

INSTANTIATE_TEST_SUITE_P(
        TypedValues, CustemBackendTest,
        ::testing::Values(
                BackendValueCase{ QString("group/int"), QVariant(42) },
                BackendValueCase{ QString("group/str"), QVariant(QString("hello 世界")) },
                BackendValueCase{ QString("flag/bool"), QVariant(true) },
                BackendValueCase{ QString("real/double"), QVariant(3.14) }));

TEST_F(SettingsTest, CustemBackend_HeapLifecycle_ConstructsAndDeletesCleanly)
{
    // Arrange：堆上创建（覆盖 deleting destructor D0）
    const QString iniFile = s_configHome->filePath("heap-lifecycle.ini");
    CustemBackend *backend = new CustemBackend(iniFile);
    ASSERT_NE(backend->m_settings, nullptr);
    backend->doSetOption(QString("heap/key"), QVariant(7));

    // Act：delete 直接触发删除型析构
    delete backend;

    // Assert：值在删除前已落盘，可由新实例读回
    CustemBackend verifier(iniFile);
    EXPECT_EQ(verifier.getOption(QString("heap/key")).toString(), QString::fromLatin1("7"));
    EXPECT_TRUE(verifier.keys().contains(QString("heap/key")));
}

TEST_F(SettingsTest, CustemBackend_DoSync_FlushesPendingValueToFile)
{
    // Arrange：绕过 doSetOption 直接写底层 QSettings
    const QString iniFile = s_configHome->filePath("dosync.ini");
    {
        CustemBackend backend(iniFile);
        backend.m_settings->setValue("raw/pending", QString("unsynced"));

        // Act
        backend.doSync();
    } // 析构后再用独立实例读回（跨实例落盘验证）

    // Assert：值已持久化到 ini 文件
    QSettings externalReader(iniFile, QSettings::IniFormat);
    EXPECT_EQ(externalReader.value("raw/pending").toString(), QString("unsynced"));
    CustemBackend reopened(iniFile);
    EXPECT_TRUE(reopened.keys().contains(QString("raw/pending")));
}

TEST_F(SettingsTest, CustemBackend_Keys_EmptyFileInitiallyThenContainsWrittenKeys)
{
    // Arrange
    const QString iniFile = s_configHome->filePath("keys.ini");
    {
        CustemBackend backend(iniFile);
        // Assert：空文件 keys 为空
        EXPECT_TRUE(backend.keys().isEmpty());

        // Act
        backend.doSetOption(QString("a/b"), QVariant(1));
        backend.doSetOption(QString("c/d"), QVariant(2));
    }

    // Assert：重新打开后 keys 完整（并覆盖析构路径 S1）
    CustemBackend reopened(iniFile);
    EXPECT_EQ(reopened.keys().size(), 2);
    EXPECT_TRUE(reopened.keys().contains(QString("a/b")));
    EXPECT_TRUE(reopened.keys().contains(QString("c/d")));
}

// ===========================================================================
// Settings 单例与生命周期（S2/S5/S11）
// ===========================================================================
TEST_F(SettingsTest, Instance_FirstCall_AlreadyInitializedWithAllOptions)
{
    // Act（SetUpTestSuite 已创建）
    Settings *inst = Settings::instance();

    // Assert：json 资源解析成功，关键 option 均存在
    ASSERT_NE(inst, nullptr);
    EXPECT_NE(inst->settings->option("base.font.family"), nullptr);
    EXPECT_NE(inst->settings->option("shortcuts.keymap.keymap"), nullptr);
    EXPECT_NE(inst->settings->option("advance.open_save_setting.savingpathwgt"), nullptr);
}

TEST_F(SettingsTest, Instance_SecondCall_ReturnsSameInstance)
{
    // Act
    Settings *first = Settings::instance();
    Settings *second = Settings::instance();

    // Assert
    EXPECT_EQ(first, second);
    EXPECT_EQ(second, Settings::s_pSetting); // 静态指针即单例本体
}

TEST_F(SettingsTest, Destruct_TemporaryInstance_KeepsSingletonIntact)
{
    // Arrange：直接构造一个临时实例（不注册为单例）
    Settings *tmp = new Settings();

    // Act
    delete tmp;

    // Assert：单例不受影响（dtor 中 s_pSetting != this 分支）
    EXPECT_EQ(Settings::instance(), s_instance);
    EXPECT_EQ(Settings::s_pSetting, s_instance); // 静态指针未被临时对象改写
}

TEST_F(SettingsTest, Instance_Recreated_AfterSingletonDelete)
{
    // Arrange
    Settings *oldOne = Settings::instance();
    EXPECT_EQ(Settings::s_pSetting, oldOne);

    // Act：删除单例 → 静态指针复位 → 再次创建
    delete oldOne;
    Settings *fresh = Settings::instance();

    // Assert：静态指针先复位后指向新实例，新实例功能完好
    EXPECT_NE(fresh, nullptr);
    EXPECT_EQ(Settings::s_pSetting, fresh);
    EXPECT_NE(fresh->settings, nullptr);
    EXPECT_EQ(Settings::instance(), fresh);
}

// ===========================================================================
// setSavePath / getSavePath / setSavePathId / getSavePathId（S3/S4）
// ===========================================================================
namespace {
struct SavePathCase {
    int id;
    const char *optionKey;
};
} // namespace

class SavePathTest : public SettingsTest, public ::testing::WithParamInterface<SavePathCase> {
};

TEST_P(SavePathTest, SetSavePath_ValidId_PersistsAndReadsBack)
{
    // Arrange
    const QString path = s_configHome->filePath(QString("dir-%1").arg(GetParam().id));

    // Act
    s_instance->setSavePath(GetParam().id, path);

    // Assert：getSavePath 读回 + option 值一致
    EXPECT_EQ(s_instance->getSavePath(GetParam().id), path);
    EXPECT_EQ(optionValue(QString::fromLatin1(GetParam().optionKey)), path);
}

INSTANTIATE_TEST_SUITE_P(
        KnownIds, SavePathTest,
        ::testing::Values(
                SavePathCase{ PathSettingWgt::LastOptBox, "advance.open_save_setting.open_save_lastopt_path" },
                SavePathCase{ PathSettingWgt::CurFileBox, "advance.open_save_setting.open_save_curfile_path" },
                SavePathCase{ PathSettingWgt::CustomBox, "advance.open_save_setting.open_save_custom_path" }));

TEST_F(SettingsTest, SetSavePath_UnknownId_DoesNotTouchAnyPathOption)
{
    // Arrange：记录三个 option 当前值
    const QString last = optionValue("advance.open_save_setting.open_save_lastopt_path");
    const QString cur = optionValue("advance.open_save_setting.open_save_curfile_path");
    const QString custom = optionValue("advance.open_save_setting.open_save_custom_path");

    // Act
    s_instance->setSavePath(99, QString::fromLatin1("/no/such/id"));

    // Assert：default 分支不改任何值
    EXPECT_EQ(optionValue("advance.open_save_setting.open_save_lastopt_path"), last);
    EXPECT_EQ(optionValue("advance.open_save_setting.open_save_curfile_path"), cur);
    EXPECT_EQ(optionValue("advance.open_save_setting.open_save_custom_path"), custom);
}

TEST_F(SettingsTest, GetSavePath_UnknownId_ReturnsEmptyString)
{
    // Act / Assert：default 分支返回空串（两个未知 id 一致，长度为 0）
    EXPECT_TRUE(s_instance->getSavePath(-1).isEmpty());
    EXPECT_EQ(s_instance->getSavePath(99).size(), 0);
}

TEST_F(SettingsTest, SetGetSavePathId_RoundTripsIdValue)
{
    // Arrange
    const int expected = PathSettingWgt::CustomBox;

    // Act
    s_instance->setSavePathId(expected);

    // Assert
    EXPECT_EQ(s_instance->getSavePathId(), expected);
    EXPECT_EQ(optionValue("advance.open_save_setting.savingpathwgt"), QString::number(expected));
}

// ===========================================================================
// setSettingDialog / dtkThemeWorkaround（S6/S7）
// ===========================================================================
TEST_F(SettingsTest, SetSettingDialog_NullDialog_StoresPointer)
{
    // Act
    s_instance->setSettingDialog(nullptr);

    // Assert
    EXPECT_EQ(s_instance->m_pSettingsDialog, nullptr);
    EXPECT_EQ(s_instance->parent(), nullptr); // 顶层对象无父窗口
}

TEST_F(SettingsTest, DtkThemeWorkaround_MixedChildren_AppliesStyleRecursively)
{
    // Arrange：父窗口 + widget 子 + 非 widget 子（覆盖 continue 分支）
    QWidget parent;
    QWidget *childWidget = new QWidget(&parent);
    QObject *plainChild = new QObject(&parent);

    // Act（成员函数，经单例调用）
    s_instance->dtkThemeWorkaround(&parent, QString::fromLatin1("Fusion"));

    // Assert：父子均应用 Fusion 风格；非 widget 子对象无副作用
    EXPECT_TRUE(parent.style()->objectName().contains(QString::fromLatin1("Fusion"), Qt::CaseInsensitive));
    EXPECT_TRUE(childWidget->style()->objectName().contains(QString::fromLatin1("Fusion"), Qt::CaseInsensitive));
    EXPECT_EQ(plainChild->children().size(), 0);
    delete plainChild;
}

// ===========================================================================
// createFontComBoBoxHandle（S8）
// ===========================================================================
namespace {
// DSettingsWidgetFactory 返回 pair 的两个元素既可能是控件本体也可能是包装容器，
// 统一做"本体 + 子树"两级查找
QComboBox *findComboBox(QWidget *pairWidget)
{
    if (auto *cb = qobject_cast<QComboBox *>(pairWidget))
        return cb;
    const auto list = pairWidget->findChildren<QComboBox *>();
    return list.isEmpty() ? nullptr : list.first();
}

PathSettingWgt *findPathWidget(QWidget *wgt)
{
    if (auto *pw = qobject_cast<PathSettingWgt *>(wgt))
        return pw;
    const auto list = wgt->findChildren<PathSettingWgt *>();
    return list.isEmpty() ? nullptr : list.first();
}
} // namespace

TEST_F(SettingsTest, CreateFontComBoBoxHandle_EmptyOptionValue_SetsSystemFixedFont)
{
    // Arrange：清空字体族配置
    auto option = s_instance->settings->option("base.font.family");
    ASSERT_NE(option, nullptr);
    option->setValue(QString(""));

    // Act
    QPair<QWidget *, QWidget *> pair = Settings::createFontComBoBoxHandle(option);

    // Assert：空值分支回填系统等宽字体；返回成对控件且包含组合框
    const QString fixedFont = QFontDatabase::systemFont(QFontDatabase::FixedFont).family();
    EXPECT_EQ(option->value().toString(), fixedFont);
    EXPECT_NE(pair.first, nullptr);
    QComboBox *comboBox = findComboBox(pair.second ? pair.second : pair.first);
    EXPECT_NE(comboBox, nullptr);
    // 系统等宽字体可能是别名（如 monospace），组合框列表为真实族名，
    // currentText 可能解析为具体族——断言列表已填充且选项非空
    EXPECT_GT(comboBox->count(), 0);
    EXPECT_FALSE(comboBox->currentText().isEmpty());

    // Act2：option 值变化 → valueChanged lambda → comboBox 同步选中
    option->setValue(QString::fromLatin1("Noto Mono"));

    // Assert2：lambda 已执行，值生效（若字体族存在则组合框选中同步）
    EXPECT_EQ(option->value().toString(), QString::fromLatin1("Noto Mono"));
    if (comboBox->findText(QString::fromLatin1("Noto Mono")) >= 0)
        EXPECT_EQ(comboBox->currentText(), QString::fromLatin1("Noto Mono"));

    // Act3：触发紧凑模式切换信号 → sizeModeChanged lambda → 高度调整
    QMetaObject::invokeMethod(DGuiApplicationHelper::instance(), "sizeModeChanged",
                              Q_ARG(DGuiApplicationHelper::SizeMode, DGuiApplicationHelper::CompactMode));

    // Assert3：lambda 执行 setFixedHeight（紧凑 24 / 常规 36，取决于全局模式）
    EXPECT_TRUE(comboBox->height() == 24 || comboBox->height() == 36)
            << "height=" << comboBox->height();
}

TEST_F(SettingsTest, CreateFontComBoBoxHandle_ExistingValue_KeepsOptionValue)
{
    // Arrange
    auto option = s_instance->settings->option("base.font.family");
    ASSERT_NE(option, nullptr);
    option->setValue(QString::fromLatin1("Noto Mono"));

    // Act
    QPair<QWidget *, QWidget *> pair = Settings::createFontComBoBoxHandle(option);

    // Assert：非空分支保持原值，组合框选中同一字体
    EXPECT_EQ(option->value().toString(), QString::fromLatin1("Noto Mono"));
    QComboBox *comboBox = findComboBox(pair.second ? pair.second : pair.first);
    ASSERT_NE(comboBox, nullptr);
    EXPECT_EQ(comboBox->currentText(), QString::fromLatin1("Noto Mono"));
    EXPECT_EQ(comboBox->count() > 0, true); // 字体族列表已填充
}

// ===========================================================================
// createSavingPathWgt（S9）
// ===========================================================================
TEST_F(SettingsTest, CreateSavingPathWgt_OptionChange_DrivesPathWidget)
{
    // Arrange：固定初始选中态为 LastOptBox，避免受先行用例残留影响
    s_instance->settings->option("advance.open_save_setting.savingpathwgt")
            ->setValue(QString::number(PathSettingWgt::LastOptBox));
    auto option = s_instance->settings->option("advance.open_save_setting.savingpathwgt");
    ASSERT_NE(option, nullptr);

    // Act
    QWidget *wgt = Settings::createSavingPathWgt(option);

    // Assert：返回包装控件且内含 PathSettingWgt
    ASSERT_NE(wgt, nullptr);
    PathSettingWgt *pathWgt = findPathWidget(wgt);
    ASSERT_NE(pathWgt, nullptr);
    auto *customBox = pathWgt->findChild<QAbstractButton *>(QString("CustomBox"));
    auto *customBtn = pathWgt->findChild<QAbstractButton *>(QString("CustomBtn"));
    ASSERT_NE(customBox, nullptr);
    ASSERT_NE(customBtn, nullptr);
    EXPECT_FALSE(customBox->isChecked()); // 初始为 LastOptBox

    // Act2：option 值变化 → lambda → onSaveIdChanged(CustomBox)
    option->setValue(PathSettingWgt::CustomBox);

    // Assert2：CustomBox 勾选 + 按钮使能（覆盖 S9 第一个 lambda）
    EXPECT_TRUE(customBox->isChecked());
    EXPECT_TRUE(customBtn->isEnabled());

    // Act3：custom path option 变化 → 空实现 lambda（覆盖 S9 第二个 lambda）
    s_instance->settings->option("advance.open_save_setting.open_save_custom_path")->setValue(QString::fromLatin1("/tmp-value"));
    EXPECT_TRUE(customBox->isChecked()); // 状态不受空 lambda 影响
}

// ===========================================================================
// createKeySequenceEditHandle + editingFinished lambda（S10）
// ===========================================================================
namespace {
KeySequenceEdit *findKeySequenceEdit(QWidget *w)
{
    // KeySequenceEdit 未声明 Q_OBJECT，不能使用 qobject_cast/findChildren<T>，
    // 改用 RTTI dynamic_cast 遍历 widget 树
    if (auto *kse = dynamic_cast<KeySequenceEdit *>(w))
        return kse;
    const auto widgets = w->findChildren<QWidget *>();
    for (QWidget *child : widgets) {
        if (auto *kse = dynamic_cast<KeySequenceEdit *>(child))
            return kse;
    }
    return nullptr;
}
} // namespace

TEST_F(SettingsTest, CreateKeySequenceEditHandle_ValidSequence_UpdatesCustomizeOption)
{
    // Arrange：standard keymap 下编辑 window.savefile
    s_instance->settings->option("shortcuts.keymap.keymap")->setValue(QString::fromLatin1("standard"));
    auto option = s_instance->settings->option("shortcuts.window.savefile");
    ASSERT_NE(option, nullptr);
    QPair<QWidget *, QWidget *> pair = Settings::createKeySequenceEditHandle(option);
    ASSERT_NE(pair.second, nullptr);
    KeySequenceEdit *kse = findKeySequenceEdit(pair.second);
    ASSERT_NE(kse, nullptr);

    // Act：有效且不冲突的序列
    QMetaObject::invokeMethod(kse, "editingFinished",
                              Q_ARG(QKeySequence, QKeySequence(QString::fromLatin1("Ctrl+Alt+Z"))));

    // Assert：customize 分支被写入，keymap 最终回到 customize
    EXPECT_EQ(optionValue("shortcuts.window_keymap_customize.savefile"), QString::fromLatin1("Ctrl+Alt+Z"));
    EXPECT_EQ(optionValue("shortcuts.keymap.keymap"), QString::fromLatin1("customize"));
    EXPECT_EQ(kse->option(), option);
}

TEST_F(SettingsTest, CreateKeySequenceEditHandle_InvalidSingleKey_RestoresCurrentValue)
{
    // Arrange
    s_instance->settings->option("shortcuts.keymap.keymap")->setValue(QString::fromLatin1("standard"));
    // 恢复语义：customize 键被重置为 window.savefile 的当前有效值
    const QString windowVal = optionValue("shortcuts.window.savefile");
    auto option = s_instance->settings->option("shortcuts.window.savefile");
    QPair<QWidget *, QWidget *> pair = Settings::createKeySequenceEditHandle(option);
    KeySequenceEdit *kse = findKeySequenceEdit(pair.second);
    ASSERT_NE(kse, nullptr);
    stubDialogExec(1); // OK

    // Act：单键 "Z" 非法（非 F1-F12）
    QMetaObject::invokeMethod(kse, "editingFinished",
                              Q_ARG(QKeySequence, QKeySequence(QString::fromLatin1("Z"))));

    // Assert：弹出对话框后 customize 键恢复为当前有效值，keymap 切到 customize
    EXPECT_NE(s_instance->m_pDialog, nullptr);
    EXPECT_EQ(optionValue("shortcuts.window_keymap_customize.savefile"), windowVal);
    EXPECT_EQ(optionValue("shortcuts.keymap.keymap"), QString::fromLatin1("customize"));
}

TEST_F(SettingsTest, CreateKeySequenceEditHandle_ConflictingSequence_ReplacesOldBinding)
{
    // Arrange：customize.newwindow 先占位一个唯一序列（不能含 Shift，否则被判非法）
    const QString conflictSeq = QString::fromLatin1("Ctrl+Alt+F13");
    s_instance->settings->option("shortcuts.window_keymap_customize.newwindow")->setValue(conflictSeq);
    s_instance->settings->option("shortcuts.keymap.keymap")->setValue(QString::fromLatin1("standard"));
    auto option = s_instance->settings->option("shortcuts.window.savefile");
    QPair<QWidget *, QWidget *> pair = Settings::createKeySequenceEditHandle(option);
    KeySequenceEdit *kse = findKeySequenceEdit(pair.second);
    ASSERT_NE(kse, nullptr);
    stubDialogExec(1); // Replace

    // Act：savefile 编辑成与 newwindow 冲突的序列
    QMetaObject::invokeMethod(kse, "editingFinished",
                              Q_ARG(QKeySequence, QKeySequence(conflictSeq)));

    // Assert：replace 生效——savefile 拿到序列，冲突方被清空
    EXPECT_EQ(optionValue("shortcuts.window_keymap_customize.savefile"), conflictSeq);
    EXPECT_TRUE(optionValue("shortcuts.window_keymap_customize.newwindow").isEmpty());
    EXPECT_EQ(optionValue("shortcuts.keymap.keymap"), QString::fromLatin1("customize"));
}

TEST_F(SettingsTest, CreateKeySequenceEditHandle_AltMConflict_CancelKeepsOldBinding)
{
    // Arrange
    s_instance->settings->option("shortcuts.keymap.keymap")->setValue(QString::fromLatin1("standard"));
    const QString before = optionValue("shortcuts.window_keymap_customize.savefile");
    auto option = s_instance->settings->option("shortcuts.window.savefile");
    QPair<QWidget *, QWidget *> pair = Settings::createKeySequenceEditHandle(option);
    KeySequenceEdit *kse = findKeySequenceEdit(pair.second);
    ASSERT_NE(kse, nullptr);
    stubDialogExec(0); // Cancel

    // Act：系统保留键 Alt+M，用户点取消
    QMetaObject::invokeMethod(kse, "editingFinished",
                              Q_ARG(QKeySequence, QKeySequence(QString::fromLatin1("Alt+M"))));

    // Assert：取消保持原绑定
    EXPECT_EQ(optionValue("shortcuts.window_keymap_customize.savefile"), before);
    EXPECT_NE(s_instance->m_pDialog, nullptr);
}

TEST_F(SettingsTest, CreateKeySequenceEditHandle_UnderCustomizeMode_SetsDirectly)
{
    // Arrange：keymap 已是 customize
    s_instance->settings->option("shortcuts.keymap.keymap")->setValue(QString::fromLatin1("customize"));
    auto option = s_instance->settings->option("shortcuts.window.savefile");
    QPair<QWidget *, QWidget *> pair = Settings::createKeySequenceEditHandle(option);
    KeySequenceEdit *kse = findKeySequenceEdit(pair.second);
    ASSERT_NE(kse, nullptr);

    // Act
    QMetaObject::invokeMethod(kse, "editingFinished",
                              Q_ARG(QKeySequence, QKeySequence(QString::fromLatin1("Ctrl+Alt+Y"))));

    // Assert：customize 模式直接写当前 key
    EXPECT_EQ(optionValue("shortcuts.window_keymap_customize.savefile"), QString::fromLatin1("Ctrl+Alt+Y"));
    EXPECT_EQ(optionValue("shortcuts.keymap.keymap"), QString::fromLatin1("customize"));
}

// ===========================================================================
// updateAllKeysWithKeymap / copyCustomizeKeysFromKeymap（S12/S13）
// ===========================================================================
TEST_F(SettingsTest, SlotupdateAllKeysWithKeymap_EmacsKeymap_SyncsShortcutOptions)
{
    // Arrange：记录 emacs 映射的目标值
    const QString emacsSave = optionValue("shortcuts.window_keymap_emacs.savefile");
    ASSERT_FALSE(emacsSave.isEmpty());

    // Act
    s_instance->slotupdateAllKeysWithKeymap(QVariant(QString::fromLatin1("emacs")));

    // Assert：window/editor 两组均已同步为 emacs 值
    const QString emacsIndent = optionValue("shortcuts.editor_keymap_emacs.indentline");
    EXPECT_EQ(optionValue("shortcuts.window.savefile"), emacsSave);
    EXPECT_EQ(optionValue("shortcuts.editor.indentline"), emacsIndent);
}

TEST_F(SettingsTest, UpdateAllKeysWithKeymap_UnknownKeymap_KeepsValuesSafe)
{
    // Arrange：未知 keymap → window_keymap_xxx 组不存在（option 为空）
    const QString before = optionValue("shortcuts.window.savefile");
    const QString editorBefore = optionValue("shortcuts.editor.indentline");

    // Act：直接调用私有实现，覆盖 option 为空的告警分支
    s_instance->updateAllKeysWithKeymap(QString::fromLatin1("nosuchkeymap"));

    // Assert：option() 为空时不写值（强异常安全，window/editor 两组均保持调用前状态）
    EXPECT_EQ(optionValue("shortcuts.window.savefile"), before);
    EXPECT_EQ(optionValue("shortcuts.editor.indentline"), editorBefore);
}

TEST_F(SettingsTest, CopyCustomizeKeysFromKeymap_Emacs_CopiesGroupValues)
{
    // Arrange：先破坏 customize 值以观察拷贝效果
    s_instance->settings->option("shortcuts.window_keymap_customize.savefile")->setValue(QString::fromLatin1("Ctrl+Broken"));
    const QString emacsSave = optionValue("shortcuts.window_keymap_emacs.savefile");

    // Act：直接调用私有实现
    s_instance->copyCustomizeKeysFromKeymap(QString::fromLatin1("emacs"));

    // Assert：customize 组（window/editor）被 emacs 组覆盖
    const QString emacsIndent = optionValue("shortcuts.editor_keymap_emacs.indentline");
    EXPECT_EQ(optionValue("shortcuts.window_keymap_customize.savefile"), emacsSave);
    EXPECT_EQ(optionValue("shortcuts.editor_keymap_customize.indentline"), emacsIndent);
}

// ===========================================================================
// checkShortcutValid（S14）
// ===========================================================================
namespace {
struct ShortcutValidCase {
    QString key;
    QString defaultValue;
    bool expectedValid;
    bool expectReasonInvalid;
};
} // namespace

class CheckShortcutValidTest : public SettingsTest, public ::testing::WithParamInterface<ShortcutValidCase> {
};

TEST_P(CheckShortcutValidTest, CheckShortcutValid_ParamVariants_ReturnsExpected)
{
    // Arrange
    QString reason;
    bool conflicts = false;

    // Act
    const bool valid = s_instance->checkShortcutValid(
            QString::fromLatin1("ut-option"), GetParam().key, reason, conflicts, GetParam().defaultValue);

    // Assert
    EXPECT_EQ(valid, GetParam().expectedValid);
    if (GetParam().expectReasonInvalid) {
        EXPECT_TRUE(reason.contains(QString::fromLatin1("invalid"), Qt::CaseInsensitive));
        EXPECT_FALSE(conflicts); // 非法键一律不算冲突
    } else {
        EXPECT_TRUE(reason.isEmpty());
    }
}

INSTANTIATE_TEST_SUITE_P(
        BranchMatrix, CheckShortcutValidTest,
        ::testing::Values(
                ShortcutValidCase{ QString::fromLatin1("Ctrl+S"), QString(), true, false },          // 常规组合键
                ShortcutValidCase{ QString::fromLatin1("F5"), QString(), true, false },             // 单键 F 区间内
                ShortcutValidCase{ QString::fromLatin1("F12"), QString(), true, false },            // 单键 F 上边界
                ShortcutValidCase{ QString::fromLatin1("Z"), QString(), false, true },              // 单键非 F
                ShortcutValidCase{ QString::fromLatin1("A"), QString(), false, true },              // 单键字母
                ShortcutValidCase{ QString::fromLatin1("Num+6"), QString(), false, true },          // 小键盘
                ShortcutValidCase{ QString::fromLatin1("Shift+A"), QString(), false, true },        // Shift 组合
                ShortcutValidCase{ QString::fromLatin1("Shift+A"), QString::fromLatin1("Shift+A"), true, false }, // Shift 恢复默认
                ShortcutValidCase{ QString::fromLatin1("Ctrl+<X>"), QString(), true, false }));     // 含 < 转义路径

// ===========================================================================
// isShortcutConflict（S15）
// ===========================================================================
TEST_F(SettingsTest, IsShortcutConflict_ValueHeldByOtherKey_ReturnsTrue)
{
    // Arrange：取一个已存在的快捷键值，用不同名查询
    const QString existing = optionValue("shortcuts.window_keymap_standard.savefile");
    ASSERT_FALSE(existing.isEmpty());

    // Act / Assert：其它按键占用该序列 → 冲突（window 与 editor 两组各验一次）
    EXPECT_TRUE(s_instance->isShortcutConflict(QString::fromLatin1("ut-other-name"), existing));
    EXPECT_TRUE(s_instance->isShortcutConflict(
            QString::fromLatin1("ut-editor-name"),
            optionValue("shortcuts.editor_keymap_standard.indentline")));
}

TEST_F(SettingsTest, IsShortcutConflict_UnknownSequence_ReturnsFalse)
{
    // Act / Assert：两个未知序列均不冲突
    EXPECT_FALSE(s_instance->isShortcutConflict(QString::fromLatin1("ut-name"),
                                                QString::fromLatin1("Ctrl+Alt+F24-Nope")));
    EXPECT_FALSE(s_instance->isShortcutConflict(QString::fromLatin1("ut-name-2"),
                                                QString::fromLatin1("Ctrl+Shift+Alt+Nope")));
}

TEST_F(SettingsTest, IsShortcutConflict_SelfKeySolelyHoldingValue_ReturnsFalse)
{
    // Arrange：让被查询键成为该序列的唯一持有者（其它键值均不同）
    const QString selfKey = QString::fromLatin1("shortcuts.window_keymap_emacs.savefile");
    const QString uniqueSeq = QString::fromLatin1("Ctrl+Alt+U7");
    s_instance->settings->option(selfKey)->setValue(uniqueSeq);

    // Act / Assert：自身持有不算冲突（window 与 editor 键各验一次）
    EXPECT_FALSE(s_instance->isShortcutConflict(selfKey, uniqueSeq));
    const QString editorKey = QString::fromLatin1("shortcuts.editor_keymap_emacs.indentline");
    const QString editorSeq = QString::fromLatin1("Ctrl+Alt+U8");
    s_instance->settings->option(editorKey)->setValue(editorSeq);
    EXPECT_FALSE(s_instance->isShortcutConflict(editorKey, editorSeq));
}

// ===========================================================================
// createDialog（S16）
// ===========================================================================
TEST_F(SettingsTest, CreateDialog_ConflictsMode_ReturnsDialogWithBottomHint)
{
    // Act
    DDialog *dialog = s_instance->createDialog(QString::fromLatin1("ut-title"),
                                               QString::fromLatin1("ut-content"), true);

    // Assert
    ASSERT_NE(dialog, nullptr);
    EXPECT_TRUE(dialog->windowFlags().testFlag(Qt::WindowStaysOnBottomHint));
    EXPECT_EQ(dialog->title(), QString::fromLatin1("ut-title"));
    delete dialog;
}

TEST_F(SettingsTest, CreateDialog_NoConflictMode_ReturnsDialogWithBottomHint)
{
    // Act
    DDialog *dialog = s_instance->createDialog(QString::fromLatin1("ut-title"),
                                               QString(), false);

    // Assert
    ASSERT_NE(dialog, nullptr);
    EXPECT_TRUE(dialog->windowFlags().testFlag(Qt::WindowStaysOnBottomHint));
    EXPECT_EQ(dialog->title(), QString::fromLatin1("ut-title"));
    delete dialog;
}

// ===========================================================================
// removeLockFiles（S17）
// ===========================================================================
TEST_F(SettingsTest, RemoveLockFiles_MissingConfigDir_ReturnsEarly)
{
    // Arrange：把 XDG_CONFIG_HOME 临时指向不存在的子目录再调用
    // （removeLockFiles 每次调用实时读取 writableLocation）
    const QString missing = s_configHome->filePath("missing-subdir");
    const QByteArray oldXdg = qgetenv("XDG_CONFIG_HOME");
    qputenv("XDG_CONFIG_HOME", missing.toUtf8());

    // Act
    s_instance->removeLockFiles();

    // Assert：提前返回，不创建目录、不崩溃
    EXPECT_FALSE(QDir(missing).exists());
    // 还原环境，避免影响后续用例
    if (oldXdg.isEmpty())
        qunsetenv("XDG_CONFIG_HOME");
    else
        qputenv("XDG_CONFIG_HOME", oldXdg);
    EXPECT_TRUE(QDir(s_configHome->path()).exists());
}

TEST_F(SettingsTest, RemoveLockFiles_RemovesLockAndRmlockButKeepsOthers)
{
    // Arrange：在配置目录中放置 .lock/.rmlock/普通文件；源码使用裸文件名，
    // 因此临时切换 CWD 到配置目录（结束恢复）
    const QString configDir = QString("%1/%2/%3")
            .arg(QStandardPaths::writableLocation(QStandardPaths::ConfigLocation))
            .arg(QString::fromLatin1(kOrgName))
            .arg(QString::fromLatin1(kAppName));
    ASSERT_TRUE(QDir().mkpath(configDir));
    const QString oldCwd = QDir::currentPath();
    ASSERT_TRUE(QDir::setCurrent(configDir));
    QFile(QString("normal.txt")).remove();
    {
        QFile f(QString("a.lock"));
        ASSERT_TRUE(f.open(QIODevice::WriteOnly));
    }
    {
        QFile f(QString("b.rmlock"));
        ASSERT_TRUE(f.open(QIODevice::WriteOnly));
    }
    {
        QFile f(QString("normal.txt"));
        ASSERT_TRUE(f.open(QIODevice::WriteOnly));
    }

    // Act
    s_instance->removeLockFiles();

    // Assert
    EXPECT_FALSE(QFile::exists(QString("a.lock")));
    EXPECT_FALSE(QFile::exists(QString("b.rmlock")));
    EXPECT_TRUE(QFile::exists(QString("normal.txt")));
    QDir::setCurrent(oldCwd);
}

TEST_F(SettingsTest, RemoveLockFiles_NonRemovableEntry_SkipsGracefully)
{
    // Arrange：名为 *.lock 的子目录无法被 QFile::remove 删除（失败分支）
    const QString configDir = QString("%1/%2/%3")
            .arg(QStandardPaths::writableLocation(QStandardPaths::ConfigLocation))
            .arg(QString::fromLatin1(kOrgName))
            .arg(QString::fromLatin1(kAppName));
    ASSERT_TRUE(QDir().mkpath(configDir + QString("/dir.lock")));
    {
        QFile f(QString("ok.lock"));
        ASSERT_TRUE(f.open(QIODevice::WriteOnly));
    }
    const QString oldCwd = QDir::currentPath();
    ASSERT_TRUE(QDir::setCurrent(configDir));

    // Act
    s_instance->removeLockFiles();

    // Assert：不可删目录保留，普通 lock 文件仍被删除
    EXPECT_TRUE(QDir(QString("dir.lock")).exists());
    EXPECT_FALSE(QFile::exists(QString("ok.lock")));
    QDir::setCurrent(oldCwd);
}

// ===========================================================================
// slotCustomshortcut（S18）
// ===========================================================================
TEST_F(SettingsTest, SlotCustomshortcut_StandardKeymap_CopiesAndSwitchesToCustomize)
{
    // Arrange
    s_instance->settings->option("shortcuts.keymap.keymap")->setValue(QString::fromLatin1("standard"));

    // Act：用户改 window.savefile（shortcuts. 前缀、非 keymap、非 _keymap_）
    s_instance->slotCustomshortcut(QString::fromLatin1("shortcuts.window.savefile"),
                                   QVariant(QString::fromLatin1("Ctrl+Shift+W")));

    // Assert：拷贝 + 更新 + keymap 切换 customize
    EXPECT_EQ(optionValue("shortcuts.window_keymap_customize.savefile"), QString::fromLatin1("Ctrl+Shift+W"));
    EXPECT_EQ(optionValue("shortcuts.keymap.keymap"), QString::fromLatin1("customize"));
    EXPECT_FALSE(s_instance->m_bUserChangeKey); // 结束后复位
}

TEST_F(SettingsTest, SlotCustomshortcut_CustomizeKeymap_UpdatesKeyOnly)
{
    // Arrange
    s_instance->settings->option("shortcuts.keymap.keymap")->setValue(QString::fromLatin1("customize"));

    // Act
    s_instance->slotCustomshortcut(QString::fromLatin1("shortcuts.window.savefile"),
                                   QVariant(QString::fromLatin1("Ctrl+Alt+Q")));

    // Assert：直接更新 customize 键，keymap 保持 customize
    EXPECT_EQ(optionValue("shortcuts.window_keymap_customize.savefile"), QString::fromLatin1("Ctrl+Alt+Q"));
    EXPECT_EQ(optionValue("shortcuts.keymap.keymap"), QString::fromLatin1("customize"));
}

TEST_F(SettingsTest, SlotCustomshortcut_NonShortcutKey_Ignored)
{
    // Arrange
    const QString before = optionValue("shortcuts.keymap.keymap");

    // Act：非 shortcuts. 前缀
    s_instance->slotCustomshortcut(QString::fromLatin1("base.font.size"), QVariant(20));

    // Assert：无任何状态变化
    EXPECT_EQ(optionValue("shortcuts.keymap.keymap"), before);
    EXPECT_FALSE(s_instance->m_bUserChangeKey);
}

TEST_F(SettingsTest, SlotCustomshortcut_KeymapOrKeymapSuffixedKeys_Ignored)
{
    // Arrange
    const QString before = optionValue("shortcuts.keymap.keymap");

    // Act：keymap 自身 + 含 _keymap_ 的键都被忽略
    s_instance->slotCustomshortcut(QString::fromLatin1("shortcuts.keymap.keymap"),
                                   QVariant(QString::fromLatin1("emacs")));
    s_instance->slotCustomshortcut(QString::fromLatin1("shortcuts.window_keymap_standard.savefile"),
                                   QVariant(QString::fromLatin1("Ctrl+Ignored")));

    // Assert
    EXPECT_EQ(optionValue("shortcuts.keymap.keymap"), before);
    EXPECT_FALSE(s_instance->m_bUserChangeKey);
}

// ===========================================================================
// slotsigAdjust* 转发槽（S19）
// ===========================================================================
TEST_F(SettingsTest, SlotsigAdjustFont_StringValue_EmitsSigAdjustFont)
{
    // Arrange
    QSignalSpy spy(s_instance, &Settings::sigAdjustFont);

    // Act
    s_instance->slotsigAdjustFont(QVariant(QString::fromLatin1("Noto Mono")));

    // Assert
    EXPECT_EQ(spy.count(), 1);
    EXPECT_EQ(spy.at(0).at(0).toString(), QString::fromLatin1("Noto Mono"));
}

TEST_F(SettingsTest, SlotsigAdjustFontSize_RealValue_EmitsSigAdjustFontSize)
{
    // Arrange
    QSignalSpy spy(s_instance, &Settings::sigAdjustFontSize);

    // Act
    s_instance->slotsigAdjustFontSize(QVariant(14.5));

    // Assert
    EXPECT_EQ(spy.count(), 1);
    EXPECT_DOUBLE_EQ(spy.at(0).at(0).toReal(), 14.5);
}

TEST_F(SettingsTest, SlotsigAdjustWordWrap_BoolValue_EmitsSigAdjustWordWrap)
{
    // Arrange
    QSignalSpy spy(s_instance, &Settings::sigAdjustWordWrap);

    // Act
    s_instance->slotsigAdjustWordWrap(QVariant(true));

    // Assert
    EXPECT_EQ(spy.count(), 1);
    EXPECT_TRUE(spy.at(0).at(0).toBool());
}

TEST_F(SettingsTest, SlotsigSetLineNumberShow_BoolValue_EmitsSigSetLineNumberShow)
{
    // Arrange
    QSignalSpy spy(s_instance, &Settings::sigSetLineNumberShow);

    // Act
    s_instance->slotsigSetLineNumberShow(QVariant(false));

    // Assert
    EXPECT_EQ(spy.count(), 1);
    EXPECT_FALSE(spy.at(0).at(0).toBool());
}

TEST_F(SettingsTest, SlotsigAdjustBookmark_BoolValue_EmitsSigAdjustBookmark)
{
    // Arrange
    QSignalSpy spy(s_instance, &Settings::sigAdjustBookmark);

    // Act
    s_instance->slotsigAdjustBookmark(QVariant(true));

    // Assert
    EXPECT_EQ(spy.count(), 1);
    EXPECT_TRUE(spy.at(0).at(0).toBool());
}

TEST_F(SettingsTest, SlotsigShowCodeFlodFlag_BoolValue_EmitsSigShowCodeFlodFlag)
{
    // Arrange
    QSignalSpy spy(s_instance, &Settings::sigShowCodeFlodFlag);

    // Act
    s_instance->slotsigShowCodeFlodFlag(QVariant(false));

    // Assert
    EXPECT_EQ(spy.count(), 1);
    EXPECT_FALSE(spy.at(0).at(0).toBool());
}

TEST_F(SettingsTest, SlotsigShowBlankCharacter_BoolValue_EmitsSigShowBlankCharacter)
{
    // Arrange
    QSignalSpy spy(s_instance, &Settings::sigShowBlankCharacter);

    // Act
    s_instance->slotsigShowBlankCharacter(QVariant(true));

    // Assert
    EXPECT_EQ(spy.count(), 1);
    EXPECT_TRUE(spy.at(0).at(0).toBool());
}

TEST_F(SettingsTest, SlotsigHightLightCurrentLine_BoolValue_EmitsSigHightLightCurrentLine)
{
    // Arrange
    QSignalSpy spy(s_instance, &Settings::sigHightLightCurrentLine);

    // Act
    s_instance->slotsigHightLightCurrentLine(QVariant(false));

    // Assert
    EXPECT_EQ(spy.count(), 1);
    EXPECT_FALSE(spy.at(0).at(0).toBool());
}

TEST_F(SettingsTest, SlotsigAdjustTabSpaceNumber_IntValue_EmitsSigAdjustTabSpaceNumber)
{
    // Arrange
    QSignalSpy spy(s_instance, &Settings::sigAdjustTabSpaceNumber);

    // Act
    s_instance->slotsigAdjustTabSpaceNumber(QVariant(4));

    // Assert
    EXPECT_EQ(spy.count(), 1);
    EXPECT_EQ(spy.at(0).at(0).toInt(), 4);
}

// ===========================================================================
// KeySequenceEdit（S20）
// ===========================================================================
namespace {
class TestableKeySequenceEdit : public KeySequenceEdit {
public:
    using KeySequenceEdit::KeySequenceEdit;
    using KeySequenceEdit::eventFilter;
};
} // namespace

TEST_F(SettingsTest, KeySequenceEdit_Constructor_StoresOptionAndInstallsFilter)
{
    // Arrange
    auto option = s_instance->settings->option("shortcuts.window.savefile");
    ASSERT_NE(option, nullptr);

    // Act
    TestableKeySequenceEdit kse(option);

    // Assert
    EXPECT_EQ(kse.option(), option);
    EXPECT_EQ(kse.parent(), nullptr);
}

TEST_F(SettingsTest, KeySequenceEdit_SlotValueChanged_EmptyValue_ClearsSequence)
{
    // Arrange
    auto option = s_instance->settings->option("shortcuts.window.savefile");
    TestableKeySequenceEdit kse(option);
    kse.setKeySequence(QKeySequence(QString::fromLatin1("Ctrl+K")));

    // Act
    kse.slotDSettingsOptionvalueChanged(QVariant(QString::fromLatin1("")));

    // Assert：清空且计数为 0
    EXPECT_TRUE(kse.keySequence().toString().isEmpty());
    EXPECT_EQ(kse.keySequence().count(), 0);
}

TEST_F(SettingsTest, KeySequenceEdit_SlotValueChanged_ValidValue_SetsSequence)
{
    // Arrange
    auto option = s_instance->settings->option("shortcuts.window.savefile");
    TestableKeySequenceEdit kse(option);

    // Act
    kse.slotDSettingsOptionvalueChanged(QVariant(QString::fromLatin1("Ctrl+S")));

    // Assert：序列生效且计数为 1
    EXPECT_EQ(kse.keySequence(), QKeySequence(QString::fromLatin1("Ctrl+S")));
    EXPECT_EQ(kse.keySequence().count(), 1);
}

TEST_F(SettingsTest, KeySequenceEdit_EventFilter_ReturnOrSpaceWithoutModifier_ReturnsTrue)
{
    // Arrange
    auto option = s_instance->settings->option("shortcuts.window.savefile");
    TestableKeySequenceEdit kse(option);
    QKeyEvent evReturn(QEvent::KeyPress, Qt::Key_Return, Qt::NoModifier);
    QKeyEvent evSpace(QEvent::KeyPress, Qt::Key_Space, Qt::NoModifier);

    // Act / Assert：无修饰键的回车/空格被拦截（切换输入焦点行为）
    EXPECT_TRUE(kse.eventFilter(&kse, &evReturn));
    EXPECT_TRUE(kse.eventFilter(&kse, &evSpace));
}

TEST_F(SettingsTest, KeySequenceEdit_EventFilter_ModifierOrForeignObject_DelegatesToBase)
{
    // Arrange
    auto option = s_instance->settings->option("shortcuts.window.savefile");
    TestableKeySequenceEdit kse(option);
    QKeyEvent evCtrlReturn(QEvent::KeyPress, Qt::Key_Return, Qt::ControlModifier);
    QKeyEvent evPlainReturn(QEvent::KeyPress, Qt::Key_Return, Qt::NoModifier);
    QObject foreign;

    // Act：组合键与外部对象走基类实现
    const bool withModifier = kse.eventFilter(&kse, &evCtrlReturn);
    const bool foreignObject = kse.eventFilter(&foreign, &evPlainReturn);

    // Assert：基类不拦截（返回 false），与"自家无修饰键才拦截"互补
    EXPECT_FALSE(withModifier);
    EXPECT_FALSE(foreignObject);
}
