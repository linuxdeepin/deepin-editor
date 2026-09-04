// SPDX-FileCopyrightText: 2026 UnionTech Software Technology Co., Ltd.
// SPDX-License-Identifier: GPL-3.0-or-later

// ComDeepinDdeDaemonDockEntryInterface（src/dbus/com_deepin_dde_daemon_dock_entry.*）单元测试
//
// stub 方案与 test_comdeepindededaemondockinterface.cpp 完全一致（见该文件头注释）：
//   S1 QDBusConnection::call → 拦截属性 Get/Set；S2 QDBusMessage::signature → 本地伪造
//   reply 补 "v" 签名；S3 asyncCallWithArgumentList → 拦截方法调用。零真实 D-Bus。
//
// 方法与分支清单（生成代码为参数直传型，分支即“属性/方法”枚举本身）：
//   构造/析构、staticInterfaceName；
//   读属性（9）：CurrentWindow(uint)、DesktopFile/Icon/Id/Menu/Name(QString)、
//     IsActive/IsDocked(bool)、WindowInfos(WindowInfoMap)；
//   D-Bus 方法（10）：Activate/Check/ForceQuit/GetAllowedCloseWindows/HandleDragDrop/
//     HandleMenuItem/NewInstance/PresentWindows/RequestDock/RequestUndock。
//
// 用例映射：
// - StaticInterfaceName_MatchesEntrySpec / Constructor_WithFakeConnection_ExposesAddresses
// - UintPropertyGetter_ReturnsBackendValue /* TEST_P */      → CurrentWindow
// - StringPropertyGetters_ReturnBackendValue /* TEST_P */    → 5 个 QString 属性（含空串边界）
// - BoolPropertyGetters_ReturnBackendValue /* TEST_P */      → IsActive/IsDocked
// - WindowInfosPropertyGetter_ReturnsBackendWindowInfoMap    → WindowInfos（元类型登记依赖）
// - Methods_CarryNameAndArguments /* TEST_P */               → 10 个 D-Bus 方法

#include <gtest/gtest.h>

#include <QCoreApplication>
#include <QDBusPendingCallWatcher>
#include <QStringList>
#include <QVariant>

#include "com_deepin_dde_daemon_dock_entry.h"
#include "stubext.h"
#include "types/windowinfomap.h"

using EntryIface = ComDeepinDdeDaemonDockEntryInterface;

namespace {

class ComDeepinDdeDaemonDockEntryInterfaceTest : public ::testing::Test {
protected:
    static void SetUpTestSuite()
    {
        registerWindowInfoMapMetaType();  // WindowInfos 读路径要求 Info/Map 元类型已登记
    }

    void SetUp() override
    {
        stub.clear();
        canned = QVariant();
        getProps.clear();
        syncCalls = 0;
        asyncCalls = 0;
        asyncMethod.clear();
        asyncArgs.clear();

        stub.set_lamda(
            static_cast<QDBusMessage (QDBusConnection::*)(const QDBusMessage &, QDBus::CallMode, int) const>(
                &QDBusConnection::call),
            [this](QDBusConnection *, const QDBusMessage &msg, QDBus::CallMode, int) -> QDBusMessage {
                __DBG_STUB_INVOKE__
                ++syncCalls;
                const QList<QVariant> args = msg.arguments();
                if (msg.member() == "Get") {
                    getProps << args.at(1).toString();
                    return msg.createReply(
                        QVariantList{QVariant::fromValue(QDBusVariant(canned))});
                }
                return msg.createReply(QVariantList{});
            });
        stub.set_lamda(&QDBusMessage::signature,
                       [](const QDBusMessage *self) -> QString {
                           __DBG_STUB_INVOKE__
                           const QList<QVariant> args = self->arguments();
                           if (self->type() == QDBusMessage::ReplyMessage && args.size() == 1
                               && args.at(0).metaType() == QMetaType::fromType<QDBusVariant>())
                               return QStringLiteral("v");
                           return QString();
                       });
        stub.set_lamda(&QDBusAbstractInterface::asyncCallWithArgumentList,
                       [this](QDBusAbstractInterface *, const QString &method,
                              const QList<QVariant> &args) -> QDBusPendingCall {
                           __DBG_STUB_INVOKE__
                           ++asyncCalls;
                           asyncMethod = method;
                           asyncArgs = args;
                           QDBusMessage call = QDBusMessage::createMethodCall(
                               "ut", "/ut", "ut.iface", method);
                           return QDBusPendingCall::fromCompletedCall(
                               call.createReply(QVariantList{}));
                       });

        obj = new EntryIface("com.deepin.dde.daemon.Dock", "/com/deepin/dde/daemon/Dock/entry/ut",
                             QDBusConnection("ut-noop-bus"), nullptr);
    }

    void TearDown() override
    {
        delete obj;
        stub.clear();
    }

    stub_ext::StubExt stub;
    EntryIface *obj = nullptr;
    QVariant canned;
    QStringList getProps;
    int syncCalls = 0;
    int asyncCalls = 0;
    QString asyncMethod;
    QVariantList asyncArgs;
};

TEST_F(ComDeepinDdeDaemonDockEntryInterfaceTest, StaticInterfaceName_MatchesEntrySpec)
{
    EXPECT_EQ(QString(EntryIface::staticInterfaceName()),
              QString("com.deepin.dde.daemon.Dock.Entry"));
    EXPECT_NE(EntryIface::staticInterfaceName(), nullptr);
}

TEST_F(ComDeepinDdeDaemonDockEntryInterfaceTest, Constructor_WithFakeConnection_ExposesAddresses)
{
    EXPECT_EQ(obj->service(), QString("com.deepin.dde.daemon.Dock"));
    EXPECT_EQ(obj->path(), QString("/com/deepin/dde/daemon/Dock/entry/ut"));
    EXPECT_EQ(obj->interface(), QString("com.deepin.dde.daemon.Dock.Entry"));
    EXPECT_FALSE(obj->isValid());
}

struct UintGetterCase {
    uint value;
};

class EntryUintGetterTest : public ComDeepinDdeDaemonDockEntryInterfaceTest,
                            public ::testing::WithParamInterface<UintGetterCase> {
};

TEST_P(EntryUintGetterTest, UintPropertyGetter_ReturnsBackendValue)
{
    const UintGetterCase &c = GetParam();
    canned = QVariant::fromValue(c.value);

    // Act
    uint got = obj->currentWindow();

    // Assert
    EXPECT_EQ(got, c.value);
    EXPECT_EQ(getProps, QStringList{"CurrentWindow"});
    EXPECT_EQ(syncCalls, 1);
}

INSTANTIATE_TEST_SUITE_P(UintValues, EntryUintGetterTest,
    ::testing::Values(UintGetterCase{0u}, UintGetterCase{8899u}, UintGetterCase{UINT_MAX}));

struct StringGetterCase {
    const char *prop;
    QString value;
};

class EntryStringGetterTest : public ComDeepinDdeDaemonDockEntryInterfaceTest,
                              public ::testing::WithParamInterface<StringGetterCase> {
};

TEST_P(EntryStringGetterTest, StringPropertyGetters_ReturnBackendValue)
{
    const StringGetterCase &c = GetParam();
    canned = QVariant::fromValue(c.value);

    // Act
    QString got;
    const QString prop = QString::fromLatin1(c.prop);
    if (prop == "DesktopFile")
        got = obj->desktopFile();
    else if (prop == "Icon")
        got = obj->icon();
    else if (prop == "Id")
        got = obj->id();
    else if (prop == "Menu")
        got = obj->menu();
    else if (prop == "Name")
        got = obj->name();
    else
        ADD_FAILURE() << "unhandled string property: " << c.prop;

    // Assert
    EXPECT_EQ(got, c.value) << "prop: " << c.prop;
    EXPECT_EQ(getProps, QStringList{prop});
    EXPECT_EQ(syncCalls, 1);
}

INSTANTIATE_TEST_SUITE_P(StringProperties, EntryStringGetterTest,
    ::testing::Values(
        StringGetterCase{"DesktopFile", "org.deepin.editor.desktop"},
        StringGetterCase{"Icon", "deepin-editor"},
        StringGetterCase{"Id", ""},
        StringGetterCase{"Menu", QString::fromUtf8("菜单-µ")},
        StringGetterCase{"Name", QString::fromUtf8("文本编辑器")}));

struct BoolGetterCase {
    const char *prop;
    bool value;
};

class EntryBoolGetterTest : public ComDeepinDdeDaemonDockEntryInterfaceTest,
                            public ::testing::WithParamInterface<BoolGetterCase> {
};

TEST_P(EntryBoolGetterTest, BoolPropertyGetters_ReturnBackendValue)
{
    const BoolGetterCase &c = GetParam();
    canned = QVariant::fromValue(c.value);

    // Act
    bool got = false;
    const QString prop = QString::fromLatin1(c.prop);
    if (prop == "IsActive")
        got = obj->isActive();
    else if (prop == "IsDocked")
        got = obj->isDocked();
    else
        ADD_FAILURE() << "unhandled bool property: " << c.prop;

    // Assert
    EXPECT_EQ(got, c.value) << "prop: " << c.prop;
    EXPECT_EQ(getProps, QStringList{prop});
    EXPECT_EQ(syncCalls, 1);
}

INSTANTIATE_TEST_SUITE_P(BoolProperties, EntryBoolGetterTest,
    ::testing::Values(
        BoolGetterCase{"IsActive", true},
        BoolGetterCase{"IsActive", false},
        BoolGetterCase{"IsDocked", true},
        BoolGetterCase{"IsDocked", false}));

TEST_F(ComDeepinDdeDaemonDockEntryInterfaceTest,
       WindowInfosPropertyGetter_ReturnsBackendWindowInfoMap)
{
    WindowInfoMap backend;
    backend.insert(100u, WindowInfo{true, QString::fromUtf8("活动窗口")});
    backend.insert(200u, WindowInfo{false, "background"});
    canned = QVariant::fromValue(backend);

    // Act
    WindowInfoMap got = obj->windowInfos();

    // Assert
    EXPECT_EQ(got.size(), 2);
    EXPECT_TRUE(got.value(100u) == backend.value(100u));    // 复杂类型经 D-Bus 类型系统完整往返
    EXPECT_TRUE(got.value(200u) == backend.value(200u));
    EXPECT_EQ(getProps, QStringList{"WindowInfos"});
}

struct EntryMethodCase {
    QString name;
    QVariantList args;
    std::function<void(EntryIface *)> invoke;
};

class EntryMethodsTest : public ComDeepinDdeDaemonDockEntryInterfaceTest,
                         public ::testing::WithParamInterface<EntryMethodCase> {
};

TEST_P(EntryMethodsTest, Methods_CarryNameAndArguments)
{
    const EntryMethodCase &c = GetParam();

    // Act
    c.invoke(obj);

    // Assert
    EXPECT_EQ(asyncCalls, 1) << "method: " << c.name.toStdString();
    EXPECT_EQ(asyncMethod, c.name);
    EXPECT_EQ(asyncArgs, c.args) << "参数表精确匹配: " << c.name.toStdString();
}

INSTANTIATE_TEST_SUITE_P(AllEntryMethods, EntryMethodsTest,
    ::testing::Values(
        EntryMethodCase{"Activate", {QVariant::fromValue(1u)},
                        [](EntryIface *i) { i->Activate(1u); }},
        EntryMethodCase{"Check", {}, [](EntryIface *i) { i->Check(); }},
        EntryMethodCase{"ForceQuit", {}, [](EntryIface *i) { i->ForceQuit(); }},
        EntryMethodCase{"GetAllowedCloseWindows", {},
                        [](EntryIface *i) { i->GetAllowedCloseWindows(); }},
        EntryMethodCase{"HandleDragDrop",
                        {QVariant::fromValue(2u), QVariant::fromValue(QStringList{"x", "y"})},
                        [](EntryIface *i) { i->HandleDragDrop(2u, {"x", "y"}); }},
        EntryMethodCase{"HandleMenuItem",
                        {QVariant::fromValue(3u), QVariant(QString("item-9"))},
                        [](EntryIface *i) { i->HandleMenuItem(3u, "item-9"); }},
        EntryMethodCase{"NewInstance", {QVariant::fromValue(4u)},
                        [](EntryIface *i) { i->NewInstance(4u); }},
        EntryMethodCase{"PresentWindows", {}, [](EntryIface *i) { i->PresentWindows(); }},
        EntryMethodCase{"RequestDock", {}, [](EntryIface *i) { i->RequestDock(); }},
        EntryMethodCase{"RequestUndock", {}, [](EntryIface *i) { i->RequestUndock(); }}));

} // namespace

int main(int argc, char **argv)
{
    QCoreApplication app(argc, argv);
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}
