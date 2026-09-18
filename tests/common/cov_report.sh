#!/usr/bin/env bash
# B2 批次 lcov 函数覆盖率统计（按测试目标隔离对象目录）
# 用法: cov_report.sh <target_name> [source_file_filter]
#   source_file_filter: 可选，按源文件名过滤（如 utils.cpp / settings.cpp / urlinfo.h）
set -u
TARGET="$1"
FILTER="${2:-}"
BD=/home/uos/work/ut/deepin-editor/build
OBJDIR="$BD/tests/common/CMakeFiles/${TARGET}.dir"
lcov --capture --directory "$OBJDIR" -o "/tmp/b2_${TARGET}.info" -q 2>/dev/null
lcov --extract "/tmp/b2_${TARGET}.info" '*/src/common/*' -o "/tmp/b2_${TARGET}_f.info" -q 2>/dev/null
python3 - "/tmp/b2_${TARGET}_f.info" "$FILTER" <<'PYEOF'
import re, sys
info = open(sys.argv[1]).read()
filt = sys.argv[2] if len(sys.argv) > 2 else ""

# 按 SF 分段
per_file = {}
cur = None
for line in info.splitlines():
    if line.startswith("SF:"):
        cur = line[3:]
        per_file[cur] = {}
    elif line.startswith("FNDA:") and cur is not None:
        cnt, name = line[5:].split(",", 1)
        per_file[cur][name] = max(per_file[cur].get(name, 0), int(cnt))

total = hit = 0
uncovered = []
for sf, fns in per_file.items():
    if filt and not sf.endswith("/" + filt) and filt not in sf.split("/")[-1]:
        continue
    for name, cnt in fns.items():
        total += 1
        if cnt > 0:
            hit += 1
        else:
            uncovered.append(f"{sf.split('/')[-1]}: {name}")

if total == 0:
    print("NO_FUNCTION_DATA")
    sys.exit(1)
print(f"FUNCTION_COVERAGE[{filt or 'all'}]: {100.0*hit/total:.1f}% ({hit}/{total})")
for u in uncovered:
    print("UNCOVERED:", u)
PYEOF
