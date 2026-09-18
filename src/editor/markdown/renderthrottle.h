// SPDX-FileCopyrightText: 2026 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

#ifndef RENDERTHROTTLE_H
#define RENDERTHROTTLE_H

#include <QObject>
#include <QTimer>
#include <QString>

//
// RenderThrottle —— Markdown 内容渲染节流/缓存策略（纯逻辑，§5'.3）
//
// 职责：
//   - 300ms 节流（前沿 + 后沿）：冷却期外的首次变更立即渲染（前沿）；冷却期内累积的
//     变更在定时器到期时补发最新内容（后沿）并进入下一周期——连续输入期间每 300ms
//     渲染一次，任何变更的可见延迟 ≤ 300ms（满足需求"300ms 内增量刷新"，2026-08-19
//     修订：原纯尾沿去抖在连续输入下无限推迟渲染）
//   - 首切立即触发：flushNow() 跳过冷却
//   - 相同内容跳过：md == lastEmitted 不重复渲染
//   - ready 前缓存：renderer 未就绪时缓存 pending，ready 后 flush
//
// 上层（EditWrapper）把 textChanged 转发给 noteContent()，渲染由 renderRequested 驱动。
//
class RenderThrottle : public QObject
{
    Q_OBJECT
public:
    explicit RenderThrottle(QObject *parent = nullptr)
        : QObject(parent)
    {
        m_timer.setSingleShot(true);
        m_timer.setInterval(300);
        connect(&m_timer, &QTimer::timeout, this, &RenderThrottle::onTimeout);
    }

    int interval() const { return m_timer.interval(); }
    void setInterval(int ms) { m_timer.setInterval(ms); }

    bool isReady() const { return m_ready; }
    void setReady(bool ready)
    {
        m_ready = ready;
        if (m_ready) flushPending();
    }

    // 记录一次输入（节流：前沿立即 + 后沿补发）：
    // 冷却期外的首次变更立即渲染并进入 300ms 冷却；冷却期内的变更累积到 pending，
    // 定时器到期时补发最新内容——连续输入期间每 300ms 渲染一次，延迟有上界
    void noteContent(const QString &md)
    {
        if (md == m_lastEmitted) return;   // 相同跳过
        m_pending = md;
        if (m_timer.interval() <= 0) {
            // interval=0 表示首切立即
            flushPending();
            return;
        }
        if (!m_timer.isActive() && flushPending())
            m_timer.start();               // 前沿已渲染，进入冷却；冷却期内变更由后沿补
    }

    // 首切立即触发（跳过去抖，但受 ready 约束）
    void flushNow() { flushPending(); }

signals:
    void renderRequested(const QString &md);

private:
    // 渲染 pending（受 ready 约束与相同内容跳过约束）；返回是否实际发出。
    // 定时器生命周期由调用方管理（noteContent 前沿启动 / onTimeout 后沿续期）
    bool flushPending()
    {
        if (!m_ready) return false;       // 未 ready，缓存
        if (m_pending.isEmpty() && m_lastEmitted.isEmpty()) {
            // 首次空内容也允许（清空场景），但 pending 与 lastEmitted 都空时不发
            return false;
        }
        if (m_pending == m_lastEmitted) return false;
        m_lastEmitted = m_pending;
        emit renderRequested(m_pending);
        return true;
    }

    // 后沿：渲染冷却期内累积的最新内容；内容仍在变化则继续下一节流周期，
    // 本周期无新内容则定时器自然停止（不再续期）
    void onTimeout()
    {
        if (m_pending != m_lastEmitted && flushPending())
            m_timer.start();
    }

    QTimer m_timer;
    QString m_pending;
    QString m_lastEmitted;
    bool m_ready = false;
};

#endif // RENDERTHROTTLE_H
