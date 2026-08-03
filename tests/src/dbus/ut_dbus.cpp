// SPDX-FileCopyrightText: 2026 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include <gtest/gtest.h>

#include <QDBusConnection>
#include <QDBusArgument>
#include <QDebug>
#include <QRect>
#include <QDBusObjectPath>
#include <QStringList>

#include "../../../src/dbus/com_deepin_dde_daemon_dock.h"
#include "../../../src/dbus/com_deepin_dde_daemon_dock_entry.h"
#include "../../../src/dbus/types/windowinfomap.h"
#include "../../../src/dbus/types/windowlist.h"

// registerDockRectMetaType() is defined in the .cpp but not declared in the header.
extern void registerDockRectMetaType();

namespace {
ComDeepinDdeDaemonDockInterface *createDockInterface()
{
    return new ComDeepinDdeDaemonDockInterface(
        QStringLiteral("com.deepin.dde.daemon.Dock"),
        QStringLiteral("/com/deepin/dde/daemon/Dock"),
        QDBusConnection::sessionBus());
}

ComDeepinDdeDaemonDockEntryInterface *createEntryInterface()
{
    return new ComDeepinDdeDaemonDockEntryInterface(
        QStringLiteral("com.deepin.dde.daemon.Dock"),
        QStringLiteral("/com/deepin/dde/daemon/Dock/entry"),
        QDBusConnection::sessionBus());
}
}

// ============================ DockRect ============================

// registerDockRectMetaType() + QMetaTypeId<DockRect>::qt_metatype_id()
TEST(UT_DockRect, registerDockRectMetaType)
{
    registerDockRectMetaType();
    SUCCEED();
}

// DockRect::DockRect()
TEST(UT_DockRect, defaultConstructor)
{
    DockRect rect;
    Q_UNUSED(rect)
    SUCCEED();
}

// DockRect::operator QRect() const
TEST(UT_DockRect, toQRect)
{
    DockRect rect;
    QRect r = rect.operator QRect();
    EXPECT_EQ(r, QRect(0, 0, 0, 0));
}

// operator<<(QDebug, DockRect const&)
TEST(UT_DockRect, qDebugOperator)
{
    DockRect rect;
    qDebug() << rect;
    SUCCEED();
}

// operator<<(QDBusArgument&, DockRect const&)
TEST(UT_DockRect, qdbusArgumentWrite)
{
    DockRect rect;
    QDBusArgument arg;
    arg << rect;
    SUCCEED();
}

// operator>>(QDBusArgument const&, DockRect&)
TEST(UT_DockRect, qdbusArgumentRead)
{
    DockRect rect;
    QDBusArgument arg;
    const QDBusArgument &ref = arg;
    ref >> rect;
    SUCCEED();
}

// ===================== ComDeepinDdeDaemonDockInterface =====================

// ctor + dtor + staticInterfaceName()
TEST(UT_ComDeepinDdeDaemonDockInterface, constructorAndDestructor)
{
    ComDeepinDdeDaemonDockInterface *iface = createDockInterface();
    EXPECT_NE(iface, nullptr);
    EXPECT_STREQ(ComDeepinDdeDaemonDockInterface::staticInterfaceName(),
                 "com.deepin.dde.daemon.Dock");
    EXPECT_STREQ(iface->staticInterfaceName(), "com.deepin.dde.daemon.Dock");
    delete iface;
}

// property getters
TEST(UT_ComDeepinDdeDaemonDockInterface, propertyGetters)
{
    registerDockRectMetaType();
    ComDeepinDdeDaemonDockInterface *iface = createDockInterface();
    iface->displayMode();
    iface->dockedApps();
    iface->entries();
    iface->frontendWindowRect();
    iface->hideMode();
    iface->hideState();
    iface->hideTimeout();
    iface->iconSize();
    iface->opacity();
    iface->position();
    iface->showTimeout();
    iface->windowSize();
    iface->windowSizeEfficient();
    iface->windowSizeFashion();
    delete iface;
    SUCCEED();
}

// property setters
TEST(UT_ComDeepinDdeDaemonDockInterface, propertySetters)
{
    ComDeepinDdeDaemonDockInterface *iface = createDockInterface();
    iface->setDisplayMode(1);
    iface->setHideMode(1);
    iface->setHideTimeout(1u);
    iface->setIconSize(1u);
    iface->setOpacity(1.0);
    iface->setPosition(1);
    iface->setShowTimeout(1u);
    iface->setWindowSize(1u);
    iface->setWindowSizeEfficient(1u);
    iface->setWindowSizeFashion(1u);
    delete iface;
    SUCCEED();
}

// DBus methods
TEST(UT_ComDeepinDdeDaemonDockInterface, methods)
{
    ComDeepinDdeDaemonDockInterface *iface = createDockInterface();
    iface->MoveWindow(1u);
    iface->CloseWindow(1u);
    iface->GetEntryIDs();
    iface->RequestDock(QStringLiteral("app.desktop"), 1);
    iface->PreviewWindow(1u);
    iface->RequestUndock(QStringLiteral("app.desktop"));
    iface->ActivateWindow(1u);
    iface->MaximizeWindow(1u);
    iface->MinimizeWindow(1u);
    iface->MakeWindowAbove(1u);
    iface->GetPluginSettings();
    iface->SetPluginSettings(QStringLiteral("{}"));
    iface->CancelPreviewWindow();
    iface->MergePluginSettings(QStringLiteral("{}"));
    iface->RemovePluginSettings(QStringLiteral("plugin"), QStringList() << "key");
    iface->SetFrontendWindowRect(1, 2, 3u, 4u);
    iface->GetDockedAppsDesktopFiles();
    iface->QueryWindowIdentifyMethod(1u);
    iface->IsDocked(QStringLiteral("app.desktop"));
    iface->IsOnDock(QStringLiteral("app.desktop"));
    iface->MoveEntry(1, 2);
    delete iface;
    SUCCEED();
}

// =================== ComDeepinDdeDaemonDockEntryInterface ===================

// ctor + dtor + staticInterfaceName()
TEST(UT_ComDeepinDdeDaemonDockEntryInterface, constructorAndDestructor)
{
    ComDeepinDdeDaemonDockEntryInterface *iface = createEntryInterface();
    EXPECT_NE(iface, nullptr);
    EXPECT_STREQ(ComDeepinDdeDaemonDockEntryInterface::staticInterfaceName(),
                 "com.deepin.dde.daemon.Dock.Entry");
    EXPECT_STREQ(iface->staticInterfaceName(), "com.deepin.dde.daemon.Dock.Entry");
    delete iface;
}

// property getters (windowInfos triggers QMetaTypeId<QMap<uint,WindowInfo>>)
TEST(UT_ComDeepinDdeDaemonDockEntryInterface, propertyGetters)
{
    registerWindowInfoMapMetaType();
    registerWindowListMetaType();
    ComDeepinDdeDaemonDockEntryInterface *iface = createEntryInterface();
    iface->currentWindow();
    iface->desktopFile();
    iface->icon();
    iface->id();
    iface->isActive();
    iface->isDocked();
    iface->menu();
    iface->name();
    iface->windowInfos();
    delete iface;
    SUCCEED();
}

// DBus methods (GetAllowedCloseWindows triggers QMetaTypeId<QList<uint>>)
TEST(UT_ComDeepinDdeDaemonDockEntryInterface, methods)
{
    registerWindowListMetaType();
    ComDeepinDdeDaemonDockEntryInterface *iface = createEntryInterface();
    iface->NewInstance(1u);
    iface->RequestDock();
    iface->RequestUndock();
    iface->HandleDragDrop(1u, QStringList() << "file");
    iface->HandleMenuItem(1u, QStringLiteral("item"));
    iface->PresentWindows();
    iface->GetAllowedCloseWindows();
    iface->Check();
    iface->Activate(1u);
    iface->ForceQuit();
    delete iface;
    SUCCEED();
}

// ============================ WindowInfo ============================

// registerWindowInfoMetaType() + QMetaTypeId<WindowInfo>::qt_metatype_id()
TEST(UT_WindowInfo, registerWindowInfoMetaType)
{
    registerWindowInfoMetaType();
    SUCCEED();
}

// registerWindowInfoMapMetaType() + QMetaTypeId<QMap<uint,WindowInfo>>::qt_metatype_id()
TEST(UT_WindowInfo, registerWindowInfoMapMetaType)
{
    registerWindowInfoMapMetaType();
    SUCCEED();
}

// WindowInfo::operator==(WindowInfo const&) const
TEST(UT_WindowInfo, equality)
{
    WindowInfo a;
    a.attention = true;
    a.title = "title";

    WindowInfo b;
    b.attention = true;
    b.title = "title";

    WindowInfo c;
    c.attention = false;
    c.title = "other";

    EXPECT_TRUE(a == b);
    EXPECT_FALSE(a == c);
}

// operator<<(QDebug, WindowInfo const&)
TEST(UT_WindowInfo, qDebugOperator)
{
    WindowInfo info;
    info.attention = true;
    info.title = "hello";
    qDebug() << info;
    SUCCEED();
}

// operator<<(QDBusArgument&, WindowInfo const&)
TEST(UT_WindowInfo, qdbusArgumentWrite)
{
    WindowInfo info;
    info.attention = false;
    info.title = "title";
    QDBusArgument arg;
    arg << info;
    SUCCEED();
}

// operator>>(QDBusArgument const&, WindowInfo&)
TEST(UT_WindowInfo, qdbusArgumentRead)
{
    WindowInfo info;
    QDBusArgument arg;
    const QDBusArgument &ref = arg;
    ref >> info;
    SUCCEED();
}

// ============================ WindowList ============================

// registerWindowListMetaType() + QMetaTypeId<QList<uint>>::qt_metatype_id()
TEST(UT_WindowList, registerWindowListMetaType)
{
    registerWindowListMetaType();
    SUCCEED();
}
