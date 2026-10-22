// SPDX-FileCopyrightText: 2026 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

#ifndef DDLOG_H
#define DDLOG_H

#include <QDebug>
#include <QLoggingCategory>

// 全应用统一的日志分类，便于 DConfig 动态日志规则（调试模式）按应用过滤
inline Q_LOGGING_CATEGORY(appLog, "org.deepin.editor")

#endif   // DDLOG_H
