// SPDX-FileCopyrightText: 2026 UnionTech Software Technology Co., Ltd.
// SPDX-License-Identifier: GPL-3.0-or-later
//
// UT 编译垫片（仅经 -include 强制注入 daemon/src/dbus.cpp 单个翻译单元，不修改源码）：
// QTextStream::setCodec 为 Qt6 已移除 API，此处以函数式宏将其映射为
// setEncoding(QStringConverter::Utf8)。测试统一使用 UTF-8 encoding 入参，语义一致。
#pragma once

#include <QStringConverter>

#define setCodec(a) setEncoding(QStringConverter::Utf8)
