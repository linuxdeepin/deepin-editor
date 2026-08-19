// SPDX-FileCopyrightText: 2026 UnionCTechnology Software Technology Co., Ltd.
// SPDX-License-Identifier: GPL-3.0-or-later

#ifndef SCROLLSYNC_H
#define SCROLLSYNC_H

//
// ScrollSync —— 滚动比例计算/钳制（纯函数，§4.6 / §5'.3）
//
// 实时阅览下由左侧 TextEdit 滚动驱动右侧 MarkdownView：
//   ratio = (value - min) / (max - min)，钳制到 [0,1]，max<=0 返回 0（除零保护）
//
class ScrollSync
{
public:
    // 由 QScrollBar 的 value/min/max 计算 ratio，钳制 [0,1]
    static double ratioFromScrollBar(int value, int min, int max)
    {
        if (max <= min) return 0.0;
        double r = double(value - min) / double(max - min);
        return clampRatio(r);
    }

    // 钳制到 [0,1]
    static double clampRatio(double ratio)
    {
        if (ratio < 0.0) return 0.0;
        if (ratio > 1.0) return 1.0;
        return ratio;
    }
};

#endif // SCROLLSYNC_H
