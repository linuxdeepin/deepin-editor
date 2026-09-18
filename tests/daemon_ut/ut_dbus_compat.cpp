// SPDX-FileCopyrightText: 2026 UnionTech Software Technology Co., Ltd.
// SPDX-License-Identifier: GPL-3.0-or-later
//
// UT 编译垫片（不修改 daemon 源码）：
// 1) ut_setcodec_compat.h 将 Qt6 已移除的 QTextStream::setCodec 宏映射为 setEncoding(Utf8)；
// 2) 显式包含 dbus.h 让 AUTOMOC 为 Q_OBJECT 生成 moc（dbus.cpp 位于 daemon/ 下，
//    其 Q_OBJECT 符号随本翻译单元编入 daemon_ut_src）；
// 3) 编译 daemon/src/dbus.cpp 原实现（相对路径，仅测试构建使用）。
#include "ut_setcodec_compat.h"

#include "dbus.h"

#include "../../daemon/src/dbus.cpp"

// AUTOMOC：ut_dbus_compat.cpp 与 dbus.h 基名不同，需显式包含 moc 产物以编入 Q_OBJECT 符号
#include "moc_dbus.cpp"
