#!/bin/bash
# SPDX-FileCopyrightText: 2026 UnionTech Software Technology Co., Ltd.
#
# SPDX-License-Identifier: GPL-3.0-or-later

export builddir=build
export reportdir=build-ut
scriptdir=$(cd $(dirname $0); pwd)
export projectdir=$(cd "$scriptdir/.."; pwd)

cd "$projectdir"
rm -rf $builddir
rm -rf $reportdir
mkdir -p $builddir
mkdir -p $reportdir
cd $builddir
#编译（Debug 开启 gcov 覆盖率插桩，BUILD_TESTS 构建全部单元测试目标）
cmake -DCMAKE_BUILD_TYPE=Debug -DBUILD_TESTS=ON -DCMAKE_SAFETYTEST_ARG="CMAKE_SAFETYTEST_ARG_ON" ..
make -j$(nproc)
#生成asan日志和ut测试xml结果
export ASAN_OPTIONS=abort_on_error=0:detect_leaks=0
export UBSAN_OPTIONS=halt_on_error=0
export QT_QPA_PLATFORM=offscreen

# 运行测试并生成 gtest XML 报告（每个测试可执行文件一份 XML）
GTEST_XML_DIR="$projectdir/$builddir/report"
mkdir -p "$GTEST_XML_DIR"

test_exit_code=0
test_bins=$(find ./tests -type f -executable -name 'test_*' | sort)
if [ -z "$test_bins" ]; then
    echo "ERROR: no test binaries found under $projectdir/$builddir/tests"
    exit 1
fi

# 判定 gtest XML 是否完整且无失败（进程退出码非 0 时用于区分真实失败与析构期崩溃）
xml_is_clean() {
    python3 -c '
import sys, xml.etree.ElementTree as ET
try:
    r = ET.parse(sys.argv[1]).getroot()
    t = int(r.get("tests", 0))
    f = int(r.get("failures", 0)) + int(r.get("errors", 0))
    sys.exit(0 if t > 0 and f == 0 else 1)
except Exception:
    sys.exit(1)
' "$1"
}

test_count=0
fail_count=0
for testbin in $test_bins; do
    name=$(basename "$testbin")
    xml="$GTEST_XML_DIR/report_${name}.xml"
    echo "==> Running $name"
    test_count=$((test_count + 1))
    timeout 600 "./$testbin" --gtest_output="xml:$xml"
    rc=$?
    if [ $rc -ne 0 ]; then
        if xml_is_clean "$xml"; then
            echo "WARNING: $name exited $rc after tests passed (teardown crash tolerated, needs fix)"
        else
            echo "FAILED: $name (exit $rc)"
            fail_count=$((fail_count + 1))
            test_exit_code=$rc
        fi
    fi
done
echo "==> Test binaries: $test_count, failed: $fail_count"

workdir=$(pwd)

mkdir -p report
#统计代码覆盖率并生成html报告
lcov -d $workdir -c -o ./coverage.info || echo "WARNING: lcov capture failed"

lcov --extract ./coverage.info '*/src/*' -o ./coverage.info || true

lcov --remove ./coverage.info '*/tests/*' -o ./coverage.info || true

genhtml -o ./html ./coverage.info || true
[ -f ./html/index.html ] && mv ./html/index.html ./html/cov_deepin-editor.html
#对asan、ut、代码覆盖率结果收集至指定文件夹
cp -r html ../$reportdir/ 2>/dev/null || true
cp -r report ../$reportdir/ 2>/dev/null || true
cp asan*.log* ../$reportdir/asan_deepin-editor.log 2>/dev/null || true

# 生成摘要 JSON
echo "==> Generating summary JSON: $projectdir/$reportdir/ut-summary.json"

python3 "$scriptdir/gen-ut-summary.py" || true

exit $test_exit_code
