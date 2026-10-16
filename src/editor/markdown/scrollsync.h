// SPDX-FileCopyrightText: 2026 UnionCTechnology Software Technology Co., Ltd.
// SPDX-License-Identifier: GPL-3.0-or-later

#ifndef SCROLLSYNC_H
#define SCROLLSYNC_H

//
// ScrollSync —— 行号同步计算/钳制（纯函数，§4.6 / §5'.3）
//
// 实时阅览双向同步按"顶部可见源码行号"对齐（VS Code markdown preview 同款语义）：
//   左→右：TextEdit 顶部可见块号（firstVisibleBlock().blockNumber()）直接下发；
//   右→左：JS 由锚点插值回传行号（可带小数），经本类折算为左栏滚动条目标值。
// 旧比例方案的缺陷：滚动条比例 value/(max-min) 与行号比例 line/总行数语义错位
//（max = 总行数 - 可见行数），中段滚动系统性偏差可达半个视口，两栏持续错位。
//

class ScrollSync
{
public:
    // 行号钳制到 [lo, hi]；退化区间（lo > hi）返回 lo
    static double clampLine(double line, double lo, double hi)
    {
        if (lo > hi) return lo;
        if (line < lo) return lo;
        if (line > hi) return hi;
        return line;
    }

    // 行号 → 滚动条目标值：四舍五入取整并钳制到 [min, max]
    static int scrollValueForLine(double line, int min, int max)
    {
        const double clamped = clampLine(line, min, max);
        return int(clamped + 0.5);
    }
};

#endif // SCROLLSYNC_H
