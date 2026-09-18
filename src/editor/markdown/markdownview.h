// SPDX-FileCopyrightText: 2026 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

#ifndef MARKDOWNVIEW_H
#define MARKDOWNVIEW_H

#include "imarkdownrenderer.h"
#include "markdownbridge.h"

#include <QWebEngineView>
#include <QWebChannel>

//
// MarkdownView —— 渲染服务层
//
// 封装 QWebEngineView + Milkdown 内核，对上层只暴露"塞文本/取文本/主题/滚动"接口（IMarkdownRenderer）。
// 参考实现：uos-ai markdownEditor（Milkdown 7.x + KaTeX + remark-math）。
//
// 构造分离：构造函数只创建 bridge 并连接信号；init() 才 load qrc 页面 + 注册 WebChannel。
//   生产：构造后立即 init()。
//   测试：构造但不 init()，可在不启动 WebEngine 渲染的前提下验证转发逻辑。
//
// 阶段一（本期）：readOnly 预览。Editable / contentChanged 为阶段二（WYSIWYG 编辑）预留，
//                接口先到位，本期上层不连接。
//
class MarkdownView : public QWebEngineView, public IMarkdownRenderer
{
    Q_OBJECT
public:
    enum Mode { ReadOnly = 0, Editable = 1 };
    Q_ENUM(Mode)

    explicit MarkdownView(QWidget *parent = nullptr);
    ~MarkdownView() override;

    // 加载 qrc 页面 + 注册 WebChannel（生产调用；测试跳过）
    void init();

    // 仅供测试/上层访问内部 bridge（信号监听）
    MarkdownBridge *bridge() const { return m_bridge; }

    // —— IMarkdownRenderer ——
    bool isReady() const override { return m_ready; }
    void setMarkdown(const QString &md) override;
    void setMode(int mode) override;
    void applyTheme(const QVariantMap &themeMap) override;   // 阶段 3 接 ThemeSerializer
    void setLayout(int maxContentWidth, bool center) override;
    void scrollToRatio(double ratio) override;

    // 反向滚动比例（右栏自身滚动时回传，供上层同步左栏，§4.6 双向）
    double scrollRatio() const;

signals:
    void ready();
    void scrollRatioChanged(double ratio);   // 预留：右栏主动滚动回传（本期不连上层）
    void contentChanged(const QString &md);  // 预留：编辑模式回写源码（本期不触发）

private slots:
    void onBridgeReady();

private:
    MarkdownBridge *m_bridge;
    QWebChannel *m_channel{nullptr};
    bool m_ready{false};
    // 最近一次主题（§4.7）：页面异步加载/崩溃 Reload 后 ready 时补发，否则主题丢失停留在浅色
    QString m_lastThemeJson;
    bool m_lastThemeDark = false;
    bool m_hasTheme = false;
    // —— ready 前协议缓存（§5.3 时序）：页面异步加载完成前 JS 未接线，直接 emit 的信号全部丢失 ——
    QString m_pendingMd;                 // 待渲染内容（与上层 RenderThrottle 缓存叠加，双保险）
    bool m_hasPendingMd = false;
    int m_lastLayoutMaxW = 0;            // 最近一次布局约束
    bool m_lastLayoutCenter = false;
    bool m_hasLayout = false;
    double m_lastScrollRatio = 0.0;      // 最近一次滚动比例（初始对齐：恢复光标后左栏非顶部）
    bool m_hasScroll = false;            // 与内容/布局/主题同型的有效标记（0.0＝回顶部亦有效）
};

#endif // MARKDOWNVIEW_H
