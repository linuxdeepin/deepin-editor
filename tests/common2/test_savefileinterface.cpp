// SPDX-FileCopyrightText: 2026 UnionTech Software Technology Co., Ltd.
// SPDX-License-Identifier: GPL-3.0-or-later

/*
 * SaveFileInterface(dbusinterface) 单元测试
 *
 * 分支清单（来源：SaveFileInterface 构造/析构/staticInterfaceName）
 * B1 : staticInterfaceName()             → "com.deepin.editor.daemon"
 * B2 : 构造（service/path/connection）    → QDBusAbstractInterface 记录三元组
 * B3 : 传入伪连接（未连接）               → isValid() == false（不触碰真实 DBus）
 * B4 : 析构                              → 安全清理
 * B5 : DBusDaemon::dbus 类型别名          → 与 SaveFileInterface 同型
 *
 * 用例映射：
 * - StaticInterfaceName_ReturnsDaemonServiceName        → B1
 * - Construct_WithFakeConnection_StoresEndpointInfo     → B2+B3
 * - Construct_WithParent_KeepsQObjectParent             → B2（parent 路径）
 * - Destruct_ScopedInterface_CleansUpQuietly            → B4
 * - TypeAlias_DBusDaemonDBus_MatchesSaveFileInterface   → B5
 *
 * 隔离：使用 QDBusConnection("ut-fake-bus")（不存在的连接名 → 未连接的连接对象），
 * 全程不 connectToBus、不发起任何真实 DBus 会话/系统总线交互。
 */

#include <gtest/gtest.h>
#include "stubext.h"

#include "dbusinterface.h"

#include <QCoreApplication>
#include <QDBusConnection>
#include <QObject>
#include <type_traits>

namespace {

QCoreApplication *ensureApp()
{
    if (!QCoreApplication::instance()) {
        static int argc = 1;
        static char argv0[] = "test_common2";
        static char *argv[2] = { argv0, nullptr };
        static QCoreApplication app(argc, argv);
        return &app;
    }
    return QCoreApplication::instance();
}

// 伪连接：名称不存在 → 未连接、isValid()==false 的连接对象（无真实总线访问）
QDBusConnection fakeConnection()
{
    return QDBusConnection(QStringLiteral("ut-fake-bus"));
}

} // namespace

class SaveFileInterfaceTest : public ::testing::Test
{
protected:
    void SetUp() override { ensureApp(); }
};

// B1
TEST_F(SaveFileInterfaceTest, StaticInterfaceName_ReturnsDaemonServiceName)
{
    // Arrange/Act/Assert
    EXPECT_STREQ(SaveFileInterface::staticInterfaceName(), "com.deepin.editor.daemon");
    EXPECT_EQ(QByteArray(SaveFileInterface::staticInterfaceName()).size(), 24);
}

// B2+B3
TEST_F(SaveFileInterfaceTest, Construct_WithFakeConnection_StoresEndpointInfo)
{
    // Arrange
    const QDBusConnection conn = fakeConnection();
    // Act
    {
        SaveFileInterface iface(QStringLiteral("com.deepin.editor.daemon"),
                                QStringLiteral("/com/deepin/editor"),
                                conn);
        // Assert: service/path/interface 三元组被记录
        EXPECT_EQ(iface.service(), QStringLiteral("com.deepin.editor.daemon"));
        EXPECT_EQ(iface.path(), QStringLiteral("/com/deepin/editor"));
        EXPECT_EQ(iface.interface(), QLatin1String(SaveFileInterface::staticInterfaceName()));
        // 伪连接未连接 → 接口无效
        EXPECT_FALSE(iface.isValid());
    } // B4: 作用域析构安全
}

// B2（parent 路径）
TEST_F(SaveFileInterfaceTest, Construct_WithParent_KeepsQObjectParent)
{
    // Arrange
    QObject parent;
    // Act
    SaveFileInterface *iface = new SaveFileInterface(QStringLiteral("com.deepin.editor.daemon"),
                                                     QStringLiteral("/p"),
                                                     fakeConnection(),
                                                     &parent);
    // Assert
    EXPECT_EQ(iface->parent(), &parent);
    EXPECT_EQ(iface->path(), QStringLiteral("/p"));
    delete iface; // 父对象不接管，直接删除验证析构安全
}

// B4
TEST_F(SaveFileInterfaceTest, Destruct_ScopedInterface_CleansUpQuietly)
{
    // Arrange/Act: 构造后立即析构（含伪连接清理路径）
    {
        SaveFileInterface iface(QStringLiteral("svc"), QStringLiteral("/path"), fakeConnection());
        EXPECT_FALSE(iface.isValid());
    }
    // Assert: 析构后仍可再次构造（连接对象复用不残留）
    SaveFileInterface again(QStringLiteral("svc"), QStringLiteral("/path"), fakeConnection());
    EXPECT_EQ(again.service(), QStringLiteral("svc"));
}

// B5
TEST_F(SaveFileInterfaceTest, TypeAlias_DBusDaemonDBus_MatchesSaveFileInterface)
{
    // Arrange/Act/Assert: DBusDaemon::dbus 是 SaveFileInterface 的类型别名
    EXPECT_TRUE((std::is_same<DBusDaemon::dbus, SaveFileInterface>::value));
    EXPECT_STREQ(DBusDaemon::dbus::staticInterfaceName(), "com.deepin.editor.daemon");
}
