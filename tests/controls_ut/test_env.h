// SPDX-FileCopyrightText: 2026 UnionTech Software Technology Co., Ltd.
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

// controls_ut 测试共享环境：
// - 一个测试二进制承载 9 个套件，QApplication(offscreen) 全进程仅构造一次
// - XDG_CONFIG_HOME / XDG_DATA_HOME 重定向 QTemporaryDir，杜绝触碰真实用户目录
//   （Settings 单例 / Utils::localDataPath 等随之隔离）

#include <QApplication>
#include <QTemporaryDir>
#include <QtTest/QSignalSpy>

#include "stubext.h"

namespace controlsut {

inline QTemporaryDir &tempRoot()
{
    static QTemporaryDir dir;  // 进程级 RAII，退出时清理
    return dir;
}

inline QApplication *ensureApp()
{
    if (QApplication::instance() == nullptr) {
        // 先隔离 XDG 再构造 QApplication（QStandardPaths 启动期缓存定位结果）
        const QByteArray cfg = (tempRoot().path() + "/config").toUtf8();
        const QByteArray data = (tempRoot().path() + "/data").toUtf8();
        qputenv("XDG_CONFIG_HOME", cfg);
        qputenv("XDG_DATA_HOME", data);
        static int argc = 1;
        static char appName[] = "test_controls_ut";
        static char *argv[] = { appName, nullptr };
        auto *app = new QApplication(argc, argv);
        // 配对还原：QApplication 析构（进程收尾）时恢复原环境变量，
        // qputenv/qunsetenv 计数平衡，避免向宿主进程泄漏
        qAddPostRoutine([]() {
            qunsetenv("XDG_CONFIG_HOME");
            qunsetenv("XDG_DATA_HOME");
        });
        return app;
    }
    return static_cast<QApplication *>(QApplication::instance());
}

}  // namespace controlsut
