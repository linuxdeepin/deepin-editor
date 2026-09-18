// SPDX-FileCopyrightText: 2026 UnionTech Software Technology Co., Ltd.
// SPDX-License-Identifier: GPL-3.0-or-later

// ComDeepinDdeDaemonDockInterface 与 DockRect（src/dbus/com_deepin_dde_daemon_dock.*）单元测试
//
// 生成接口的调用汇聚路径（Qt 6.8 QDBusAbstractInterface 源码结论，Wave1 经验扩展）：
//   属性读：getter → property() → qt_metacall(ReadProperty)（QDBusAbstractInterfaceBase
//           拦截）→ QDBusAbstractInterfacePrivate::property → QDBusConnection::call(
//           org.freedesktop.DBus.Properties.Get, [iface, prop])，回包须为签名 "v" 的
//           QDBusVariant；
//   属性写：setter → setProperty() → qt_metacall(WriteProperty) → ...Private::setProperty
//           → QDBusConnection::call(Properties.Set, [iface, prop, QDBusVariant(value)])；
//   方法：  asyncCallWithArgumentList(method, args) 直达。
// 本地伪造的 QDBusMessage::signature() 恒为空（仅接收方向报文才有签名，
// qdbusmessage.cpp 由 d_ptr->signature 返回），无法通过 Get 回包的 "v" 校验；且空
// service 会令 canMakeCalls 解引用空 connectionPrivate() 崩溃。故 stub 方案（SetUp 设置
// / TearDown clear，地址级替换）：
//   S1: QDBusConnection::call(3 参重载) → 拦截全部属性 Get/Set：记录属性名/写入值，
//       Get 回灌夹具预置 canned 值（包 QDBusVariant，经 createReply 保证 ReplyMessage 类型）；
//   S2: QDBusMessage::signature → 仅对「ReplyMessage + 单一 QDBusArgument 参数」返回 "v"
//       （与真实总线回包语义一致），其余返回 QString()（与本地消息未 stub 时行为一致）；
//   S3: QDBusAbstractInterface::asyncCallWithArgumentList → 拦截全部方法调用：记录方法名
//       与参数表，返回已完成的 QDBusPendingCall。
//   service 名使用非空合法总线名（绕开 canMakeCalls 空值短路路径），连接对象为断连的
//   QDBusConnection("ut-noop-bus")，全程零真实 D-Bus。
//
// 方法与分支清单（生成代码为参数直传型，分支即“属性/方法/信号”枚举本身）：
//   构造/析构、staticInterfaceName；
//   读属性（10）：DisplayMode/HideMode/HideState/Position(int)、HideTimeout/IconSize/
//     ShowTimeout/WindowSize/WindowSizeEfficient/WindowSizeFashion(uint)、Opacity(double)、
//     DockedApps(QStringList)、Entries(QList<QDBusObjectPath>)、FrontendWindowRect(DockRect)
//     ——共 14 个读属性；写属性（10）：DisplayMode/HideMode/Position、HideTimeout/IconSize/
//     ShowTimeout/WindowSize/WindowSizeEfficient/WindowSizeFashion、Opacity；
//   D-Bus 方法（21）：ActivateWindow … SetPluginSettings（见 Methods_CarryNameAndArguments）；
//   信号（5）：DockAppSettingsSynced/EntryAdded/EntryRemoved/PluginSettingsSynced/ServiceRestarted。
//   DockRect：默认构造、operator QRect()、operator<<(QDebug)、operator<<(QDBusArgument)、
//     operator>>(QDBusArgument)（Demarshal 经 Qt 原语抽取点 stub 回灌，被测函数体真实执行）、
//     registerDockRectMetaType。
//
// 用例映射：
// - StaticInterfaceName_MatchesDockSpec / Constructor_WithFakeConnection_ExposesAddresses → 构造/静态
// - IntPropertyGetters_ReturnBackendValue /* TEST_P */        → 4 个 int 读属性
// - UintPropertyGetters_ReturnBackendValue /* TEST_P */       → 6 个 uint 读属性
// - OpacityPropertyGetter_ReturnsBackendValue                 → Opacity
// - DockedAppsPropertyGetter_ReturnsBackendValue              → DockedApps（含空表边界）
// - EntriesPropertyGetter_ReturnsBackendValue                 → Entries
// - FrontendWindowRectPropertyGetter_ReturnsBackendDockRect   → FrontendWindowRect（依赖元类型登记）
// - IntPropertySetters_SendValueToBackend /* TEST_P */         → 3 个 int 写属性
// - UintPropertySetters_SendValueToBackend /* TEST_P */       → 6 个 uint 写属性
// - OpacityPropertySetter_SendsValueToBackend                 → Opacity 写
// - Methods_CarryNameAndArguments /* TEST_P */                → 21 个 D-Bus 方法
// - Signals_EmittedViaMetaSystem_CarryExactArguments          → 5 个信号
// - DockRect_*（6 例，见各用例注释）                          → DockRect 全部函数
// - RegisterDockRectMetaType_RegistersQtAndDBusTypes          → registerDockRectMetaType
//
// 编译说明：-fno-access-control 用于读写 DockRect 私有字段（x/y/w/h）做精确断言。

#include <gtest/gtest.h>

#include <QCoreApplication>
#include <QDBusPendingCallWatcher>
#include <QSignalSpy>
#include <QStringList>
#include <QVariant>

#include "com_deepin_dde_daemon_dock.h"
#include "stubext.h"

// 该函数在 com_deepin_dde_daemon_dock.cpp 定义但头文件未声明（生成代码手工追加），此处补声明
extern void registerDockRectMetaType();

using Iface = ComDeepinDdeDaemonDockInterface;

namespace {

class ComDeepinDdeDaemonDockInterfaceTest : public ::testing::Test {
protected:
    static void SetUpTestSuite()
    {
        registerDockRectMetaType();  // FrontendWindowRect 读路径要求 D-Bus 元类型已登记
    }

    void SetUp() override
    {
        stub.clear();
        canned = QVariant();
        getProps.clear();
        setProps.clear();
        setValues.clear();
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
                setProps << args.at(1).toString();
                setValues << qvariant_cast<QDBusVariant>(args.at(2)).variant();
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

        obj = new Iface("com.deepin.dde.daemon.Dock", "/com/deepin/dde/daemon/Dock",
                        QDBusConnection("ut-noop-bus"), nullptr);
    }

    void TearDown() override
    {
        delete obj;
        stub.clear();
    }

    stub_ext::StubExt stub;
    Iface *obj = nullptr;
    QVariant canned;                 // Get 回灌值
    QStringList getProps;            // Properties.Get 捕获的属性名序列
    QStringList setProps;            // Properties.Set 捕获的属性名序列
    QList<QVariant> setValues;       // Properties.Set 捕获的写入值序列
    int syncCalls = 0;               // QDBusConnection::call 拦截计数
    int asyncCalls = 0;              // asyncCallWithArgumentList 拦截计数
    QString asyncMethod;             // 最近一次方法名
    QVariantList asyncArgs;          // 最近一次方法参数表
};

TEST_F(ComDeepinDdeDaemonDockInterfaceTest, StaticInterfaceName_MatchesDockSpec)
{
    EXPECT_EQ(QString(Iface::staticInterfaceName()), QString("com.deepin.dde.daemon.Dock"));
    EXPECT_NE(Iface::staticInterfaceName(), nullptr);  // 返回静态存储字符串
}

TEST_F(ComDeepinDdeDaemonDockInterfaceTest, Constructor_WithFakeConnection_ExposesAddresses)
{
    EXPECT_EQ(obj->service(), QString("com.deepin.dde.daemon.Dock"));
    EXPECT_EQ(obj->path(), QString("/com/deepin/dde/daemon/Dock"));
    EXPECT_EQ(obj->interface(), QString("com.deepin.dde.daemon.Dock"));
    EXPECT_FALSE(obj->isValid());  // 断连伪连接 → 有效性为 false
}

struct IntPropertyCase {
    const char *prop;
    int value;
};

class DockIntPropertyGetterTest : public ComDeepinDdeDaemonDockInterfaceTest,
                                  public ::testing::WithParamInterface<IntPropertyCase> {
};

TEST_P(DockIntPropertyGetterTest, IntPropertyGetters_ReturnBackendValue)
{
    const IntPropertyCase &c = GetParam();
    canned = QVariant(c.value);

    // Act
    int got = 0;
    const QString prop = QString::fromLatin1(c.prop);
    if (prop == "DisplayMode")
        got = obj->displayMode();
    else if (prop == "HideMode")
        got = obj->hideMode();
    else if (prop == "HideState")
        got = obj->hideState();
    else if (prop == "Position")
        got = obj->position();
    else
        ADD_FAILURE() << "unhandled int property: " << c.prop;

    // Assert
    EXPECT_EQ(got, c.value) << "prop: " << c.prop;
    EXPECT_EQ(getProps, QStringList{prop}) << "getter 必须按名读取属性";
    EXPECT_EQ(syncCalls, 1);
}

INSTANTIATE_TEST_SUITE_P(IntProperties, DockIntPropertyGetterTest,
    ::testing::Values(
        IntPropertyCase{"DisplayMode", 0},
        IntPropertyCase{"DisplayMode", 5},
        IntPropertyCase{"HideMode", 2},
        IntPropertyCase{"HideState", 1},
        IntPropertyCase{"Position", 3},
        IntPropertyCase{"Position", 0}));

struct UintPropertyCase {
    const char *prop;
    uint value;
};

class DockUintPropertyGetterTest : public ComDeepinDdeDaemonDockInterfaceTest,
                                   public ::testing::WithParamInterface<UintPropertyCase> {
};

TEST_P(DockUintPropertyGetterTest, UintPropertyGetters_ReturnBackendValue)
{
    const UintPropertyCase &c = GetParam();
    canned = QVariant::fromValue(c.value);

    // Act
    uint got = 0;
    const QString prop = QString::fromLatin1(c.prop);
    if (prop == "HideTimeout")
        got = obj->hideTimeout();
    else if (prop == "IconSize")
        got = obj->iconSize();
    else if (prop == "ShowTimeout")
        got = obj->showTimeout();
    else if (prop == "WindowSize")
        got = obj->windowSize();
    else if (prop == "WindowSizeEfficient")
        got = obj->windowSizeEfficient();
    else if (prop == "WindowSizeFashion")
        got = obj->windowSizeFashion();
    else
        ADD_FAILURE() << "unhandled uint property: " << c.prop;

    // Assert
    EXPECT_EQ(got, c.value) << "prop: " << c.prop;
    EXPECT_EQ(getProps, QStringList{prop});
    EXPECT_EQ(syncCalls, 1);
}

INSTANTIATE_TEST_SUITE_P(UintProperties, DockUintPropertyGetterTest,
    ::testing::Values(
        UintPropertyCase{"HideTimeout", 0u},
        UintPropertyCase{"HideTimeout", 100u},
        UintPropertyCase{"IconSize", 48u},
        UintPropertyCase{"ShowTimeout", 250u},
        UintPropertyCase{"WindowSize", 400u},
        UintPropertyCase{"WindowSizeEfficient", 300u},
        UintPropertyCase{"WindowSizeFashion", UINT_MAX}));

TEST_F(ComDeepinDdeDaemonDockInterfaceTest, OpacityPropertyGetter_ReturnsBackendValue)
{
    canned = QVariant::fromValue(0.75);

    // Act
    double got = obj->opacity();

    // Assert
    EXPECT_DOUBLE_EQ(got, 0.75);
    EXPECT_EQ(getProps, QStringList{"Opacity"});
    EXPECT_EQ(syncCalls, 1);
}

TEST_F(ComDeepinDdeDaemonDockInterfaceTest, DockedAppsPropertyGetter_ReturnsBackendValue)
{
    const QStringList backend{"org.deepin.editor.desktop", "ut.second.desktop"};
    canned = QVariant::fromValue(backend);

    // Act
    QStringList got = obj->dockedApps();

    // Assert
    EXPECT_EQ(got, backend);
    EXPECT_EQ(getProps, QStringList{"DockedApps"});

    // 空表边界（等价类第二组）
    canned = QVariant::fromValue(QStringList{});
    QStringList empty = obj->dockedApps();
    EXPECT_TRUE(empty.isEmpty());
    EXPECT_EQ(getProps, (QStringList{"DockedApps", "DockedApps"}));
}

TEST_F(ComDeepinDdeDaemonDockInterfaceTest, EntriesPropertyGetter_ReturnsBackendValue)
{
    const QList<QDBusObjectPath> backend{QDBusObjectPath("/org/entry/1"),
                                         QDBusObjectPath("/org/entry/2")};
    canned = QVariant::fromValue(backend);

    // Act
    QList<QDBusObjectPath> got = obj->entries();

    // Assert
    EXPECT_EQ(got.size(), 2);
    EXPECT_EQ(got, backend);
    EXPECT_EQ(getProps, QStringList{"Entries"});
}

TEST_F(ComDeepinDdeDaemonDockInterfaceTest, FrontendWindowRectPropertyGetter_ReturnsBackendDockRect)
{
    DockRect backend;
    backend.x = 11;
    backend.y = 22;
    backend.w = 333;
    backend.h = 4444;
    canned = QVariant::fromValue(backend);

    // Act
    DockRect got = obj->frontendWindowRect();

    // Assert
    EXPECT_EQ(got.operator QRect(), QRect(11, 22, 333, 4444));  // 值经 D-Bus 类型系统完整往返
    EXPECT_EQ(getProps, QStringList{"FrontendWindowRect"});
}

struct IntSetterCase {
    const char *prop;
    int value;
};

class DockIntPropertySetterTest : public ComDeepinDdeDaemonDockInterfaceTest,
                                  public ::testing::WithParamInterface<IntSetterCase> {
};

TEST_P(DockIntPropertySetterTest, IntPropertySetters_SendValueToBackend)
{
    const IntSetterCase &c = GetParam();
    const QString prop = QString::fromLatin1(c.prop);

    // Act
    if (prop == "DisplayMode")
        obj->setDisplayMode(c.value);
    else if (prop == "HideMode")
        obj->setHideMode(c.value);
    else if (prop == "Position")
        obj->setPosition(c.value);
    else
        ADD_FAILURE() << "unhandled int setter: " << c.prop;

    // Assert
    ASSERT_EQ(setProps.size(), 1);
    EXPECT_EQ(setProps.at(0), prop) << "setter 必须按名写属性";
    EXPECT_EQ(setValues.at(0), QVariant(c.value)) << "写入值精确一致";
    EXPECT_EQ(syncCalls, 1);
}

INSTANTIATE_TEST_SUITE_P(IntSetters, DockIntPropertySetterTest,
    ::testing::Values(
        IntSetterCase{"DisplayMode", 1},
        IntSetterCase{"HideMode", 0},
        IntSetterCase{"HideMode", 2},
        IntSetterCase{"Position", 3},
        IntSetterCase{"Position", -1}));

struct UintSetterCase {
    const char *prop;
    uint value;
};

class DockUintPropertySetterTest : public ComDeepinDdeDaemonDockInterfaceTest,
                                   public ::testing::WithParamInterface<UintSetterCase> {
};

TEST_P(DockUintPropertySetterTest, UintPropertySetters_SendValueToBackend)
{
    const UintSetterCase &c = GetParam();
    const QString prop = QString::fromLatin1(c.prop);

    // Act
    if (prop == "HideTimeout")
        obj->setHideTimeout(c.value);
    else if (prop == "IconSize")
        obj->setIconSize(c.value);
    else if (prop == "ShowTimeout")
        obj->setShowTimeout(c.value);
    else if (prop == "WindowSize")
        obj->setWindowSize(c.value);
    else if (prop == "WindowSizeEfficient")
        obj->setWindowSizeEfficient(c.value);
    else if (prop == "WindowSizeFashion")
        obj->setWindowSizeFashion(c.value);
    else
        ADD_FAILURE() << "unhandled uint setter: " << c.prop;

    // Assert
    ASSERT_EQ(setProps.size(), 1);
    EXPECT_EQ(setProps.at(0), prop);
    EXPECT_EQ(setValues.at(0).metaType(), QMetaType::fromType<uint>());  // 类型精确为 uint
    EXPECT_EQ(setValues.at(0), QVariant::fromValue(c.value));
    EXPECT_EQ(syncCalls, 1);
}

INSTANTIATE_TEST_SUITE_P(UintSetters, DockUintPropertySetterTest,
    ::testing::Values(
        UintSetterCase{"HideTimeout", 0u},
        UintSetterCase{"IconSize", 48u},
        UintSetterCase{"ShowTimeout", 200u},
        UintSetterCase{"WindowSize", 400u},
        UintSetterCase{"WindowSizeEfficient", 300u},
        UintSetterCase{"WindowSizeFashion", 500u}));

TEST_F(ComDeepinDdeDaemonDockInterfaceTest, OpacityPropertySetter_SendsValueToBackend)
{
    // Act
    obj->setOpacity(0.5);

    // Assert
    ASSERT_EQ(setProps.size(), 1);
    EXPECT_EQ(setProps.at(0), QString("Opacity"));
    EXPECT_EQ(setValues.at(0), QVariant::fromValue(0.5));
    EXPECT_EQ(syncCalls, 1);
}

struct MethodCase {
    QString name;
    QVariantList args;
    std::function<void(Iface *)> invoke;
};

class DockMethodsTest : public ComDeepinDdeDaemonDockInterfaceTest,
                        public ::testing::WithParamInterface<MethodCase> {
};

TEST_P(DockMethodsTest, Methods_CarryNameAndArguments)
{
    const MethodCase &c = GetParam();

    // Act
    c.invoke(obj);

    // Assert
    EXPECT_EQ(asyncCalls, 1) << "method: " << c.name.toStdString();
    EXPECT_EQ(asyncMethod, c.name) << "方法名精确匹配";
    EXPECT_EQ(asyncArgs, c.args) << "参数表（顺序/类型/值）精确匹配: " << c.name.toStdString();
}

INSTANTIATE_TEST_SUITE_P(AllMethods, DockMethodsTest,
    ::testing::Values(
        MethodCase{"ActivateWindow", {QVariant::fromValue(1u)},
                   [](Iface *i) { i->ActivateWindow(1u); }},
        MethodCase{"CancelPreviewWindow", {},
                   [](Iface *i) { i->CancelPreviewWindow(); }},
        MethodCase{"CloseWindow", {QVariant::fromValue(2u)},
                   [](Iface *i) { i->CloseWindow(2u); }},
        MethodCase{"GetDockedAppsDesktopFiles", {},
                   [](Iface *i) { i->GetDockedAppsDesktopFiles(); }},
        MethodCase{"GetEntryIDs", {}, [](Iface *i) { i->GetEntryIDs(); }},
        MethodCase{"GetPluginSettings", {}, [](Iface *i) { i->GetPluginSettings(); }},
        MethodCase{"IsDocked", {QVariant(QString("dock-id"))},
                   [](Iface *i) { i->IsDocked("dock-id"); }},
        MethodCase{"IsOnDock", {QVariant(QString("on-dock-id"))},
                   [](Iface *i) { i->IsOnDock("on-dock-id"); }},
        MethodCase{"MakeWindowAbove", {QVariant::fromValue(3u)},
                   [](Iface *i) { i->MakeWindowAbove(3u); }},
        MethodCase{"MaximizeWindow", {QVariant::fromValue(4u)},
                   [](Iface *i) { i->MaximizeWindow(4u); }},
        MethodCase{"MergePluginSettings", {QVariant(QString("merge"))},
                   [](Iface *i) { i->MergePluginSettings("merge"); }},
        MethodCase{"MinimizeWindow", {QVariant::fromValue(5u)},
                   [](Iface *i) { i->MinimizeWindow(5u); }},
        MethodCase{"MoveEntry", {QVariant::fromValue(-1), QVariant::fromValue(5)},
                   [](Iface *i) { i->MoveEntry(-1, 5); }},
        MethodCase{"MoveWindow", {QVariant::fromValue(6u)},
                   [](Iface *i) { i->MoveWindow(6u); }},
        MethodCase{"PreviewWindow", {QVariant::fromValue(7u)},
                   [](Iface *i) { i->PreviewWindow(7u); }},
        MethodCase{"QueryWindowIdentifyMethod", {QVariant::fromValue(8u)},
                   [](Iface *i) { i->QueryWindowIdentifyMethod(8u); }},
        MethodCase{"RemovePluginSettings",
                   {QVariant(QString("key")), QVariant::fromValue(QStringList{"a", "b"})},
                   [](Iface *i) { i->RemovePluginSettings("key", {"a", "b"}); }},
        MethodCase{"RequestDock", {QVariant(QString("desktop-file")), QVariant::fromValue(1)},
                   [](Iface *i) { i->RequestDock("desktop-file", 1); }},
        MethodCase{"RequestUndock", {QVariant(QString("undock-id"))},
                   [](Iface *i) { i->RequestUndock("undock-id"); }},
        MethodCase{"SetFrontendWindowRect",
                   {QVariant::fromValue(1), QVariant::fromValue(2), QVariant::fromValue(3u),
                    QVariant::fromValue(4u)},
                   [](Iface *i) { i->SetFrontendWindowRect(1, 2, 3u, 4u); }},
        MethodCase{"SetPluginSettings", {QVariant(QString("settings"))},
                   [](Iface *i) { i->SetPluginSettings("settings"); }}));

// 5 个信号：经元对象系统发射（moc 真实路径），QSignalSpy 验证参数
TEST_F(ComDeepinDdeDaemonDockInterfaceTest, Signals_EmittedViaMetaSystem_CarryExactArguments)
{
    QSignalSpy syncedSpy(obj, &Iface::DockAppSettingsSynced);
    QSignalSpy addedSpy(obj, &Iface::EntryAdded);
    QSignalSpy removedSpy(obj, &Iface::EntryRemoved);
    QSignalSpy pluginSpy(obj, &Iface::PluginSettingsSynced);
    QSignalSpy restartedSpy(obj, &Iface::ServiceRestarted);
    ASSERT_TRUE(syncedSpy.isValid());
    ASSERT_TRUE(addedSpy.isValid());

    // Act
    QMetaObject::invokeMethod(obj, "DockAppSettingsSynced");
    QMetaObject::invokeMethod(obj, "EntryAdded", Q_ARG(QDBusObjectPath, QDBusObjectPath("/e/1")),
                              Q_ARG(int, 7));
    QMetaObject::invokeMethod(obj, "EntryRemoved", Q_ARG(QString, "gone-entry"));
    QMetaObject::invokeMethod(obj, "PluginSettingsSynced");
    QMetaObject::invokeMethod(obj, "ServiceRestarted");

    // Assert
    EXPECT_EQ(syncedSpy.count(), 1);
    EXPECT_EQ(addedSpy.count(), 1);
    EXPECT_EQ(addedSpy.at(0).at(0).value<QDBusObjectPath>().path(), QString("/e/1"));
    EXPECT_EQ(addedSpy.at(0).at(1).toInt(), 7);
    EXPECT_EQ(removedSpy.count(), 1);
    EXPECT_EQ(removedSpy.at(0).at(0).toString(), QString("gone-entry"));
    EXPECT_EQ(pluginSpy.count(), 1);
    EXPECT_EQ(restartedSpy.count(), 1);
}

// ---------------- DockRect ----------------

TEST(DockRectTest, DefaultConstructor_ConvertsToNullQRect)
{
    DockRect r;

    EXPECT_TRUE(r.operator QRect().isNull());   // 默认 0,0,0x0
    EXPECT_EQ(r.operator QRect(), QRect(0, 0, 0, 0));
}

TEST(DockRectTest, QRectConversion_MapsAllFourFields)
{
    DockRect r;
    r.x = 1;
    r.y = 2;
    r.w = 3;
    r.h = 4;

    EXPECT_EQ(r.operator QRect(), QRect(1, 2, 3, 4));
    EXPECT_EQ(r.operator QRect().width(), 3);   // 宽度字段独立可见
}

TEST(DockRectTest, DebugOperator_FormatsAllFourFields)
{
    DockRect r;
    r.x = 9;
    r.y = 8;
    r.w = 7;
    r.h = 6;

    QString captured;
    {
        QDebug dbg(&captured);
        dbg << r;
    }

    EXPECT_TRUE(captured.contains("DockRect(9, 8, 7, 6)"));  // 四字段完整格式化
    EXPECT_FALSE(captured.isEmpty());
}

TEST(DockRectTest, Marshal_ProducesStructSignature)
{
    DockRect r;
    r.x = 1;
    r.y = 2;
    r.w = 3;
    r.h = 4;

    QDBusArgument arg;
    arg << r;

    EXPECT_EQ(arg.currentSignature(), QString("(iiuu)"));  // int,int,uint,uint 结构体
}

// Demarshal：Qt 原语抽取点 stub 回灌（见文件头说明），被测 operator>> 真实执行
class DockRectDemarshalTest : public ::testing::Test {
protected:
    void SetUp() override
    {
        stub.clear();
        fedInts.clear();
        fedUints.clear();

        stub.set_lamda(
            static_cast<void (QDBusArgument::*)() const>(&QDBusArgument::beginStructure),
            [](const QDBusArgument *) { __DBG_STUB_INVOKE__ });
        stub.set_lamda(
            static_cast<void (QDBusArgument::*)() const>(&QDBusArgument::endStructure),
            [](const QDBusArgument *) { __DBG_STUB_INVOKE__ });
        stub.set_lamda(
            static_cast<const QDBusArgument &(QDBusArgument::*)(int &) const>(&QDBusArgument::operator>>),
            [this](const QDBusArgument *self, int &out) -> const QDBusArgument & {
                __DBG_STUB_INVOKE__
                out = fedInts.isEmpty() ? 0 : fedInts.takeFirst();
                return *self;
            });
        stub.set_lamda(
            static_cast<const QDBusArgument &(QDBusArgument::*)(uint &) const>(&QDBusArgument::operator>>),
            [this](const QDBusArgument *self, uint &out) -> const QDBusArgument & {
                __DBG_STUB_INVOKE__
                out = fedUints.isEmpty() ? 0u : fedUints.takeFirst();
                return *self;
            });
    }

    void TearDown() override { stub.clear(); }

    stub_ext::StubExt stub;
    QList<int> fedInts;
    QList<uint> fedUints;
};

TEST_F(DockRectDemarshalTest, Demarshal_FedPrimitives_ExtractsAllFields)
{
    fedInts << 5 << 6;
    fedUints << 70u << 80u;
    QDBusArgument wire;
    DockRect out;

    wire >> out;

    EXPECT_EQ(out.x, 5);   // x 依序提取
    EXPECT_EQ(out.y, 6);   // y 依序提取
    EXPECT_EQ(out.w, 70u); // w 依序提取
    EXPECT_EQ(out.h, 80u); // h 依序提取
    EXPECT_EQ(out.operator QRect(), QRect(5, 6, 70, 80));  // 提取后经换算复核
}

TEST(DockRectTest, RegisterDockRectMetaType_RegistersQtAndDBusTypes)
{
    registerDockRectMetaType();

    EXPECT_NE(QMetaType::fromName("DockRect").id(), QMetaType::UnknownType);  // Qt 侧登记
    QVariant boxed = QVariant::fromValue(DockRect{});
    EXPECT_EQ(boxed.metaType().name(), QByteArray("DockRect"));               // 可作为 D-Bus 属性载体
}

} // namespace

int main(int argc, char **argv)
{
    QCoreApplication app(argc, argv);
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}
