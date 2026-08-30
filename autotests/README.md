# deepin-editor autotests

Google Test unit-test suite for **deepin-editor**, generated and maintained with
the `qt-autotest-generator` skill.

* **Framework**: Google Test / GoogleMock only (no QtTest).
* **Layout**: per-class test files under `autotests/<module>/test_<classname>.cpp`.
* **Coverage**: built with `-fprofile-arcs -ftest-coverage` in `Debug`; coverage
  is captured with `lcov` and reported as HTML/CSV by `run-ut.sh`.
* **Stub runtime**: `3rdparty/stub/` (built-in stub-ext, never downloaded).

## Build & run (one command)

```bash
./autotests/run-ut.sh
```

This will configure, compile, run all tests, collect `lcov` coverage, and emit:

* `build-autotests/test-reports/test_report.html` — human-readable report.
* `build-autotests/coverage/html/index.html` — lcov HTML coverage report.
* `build-autotests/coverage/filtered.info` — filtered coverage data (`*/src/*`).

## Reproducible build environment

This project links `Qt6`, `DTK6`, `KF6::Codecs`, `KF6::SyntaxHighlighting`,
`ICU`, `uchardet` and `chardet`. On a host where some `-dev` packages are not
installed system-wide, `run-ut.sh` auto-sources `~/ut-deps/env.sh` (a
user-local prefix of extracted `-dev` packages) and passes
`-DCMAKE_PREFIX_PATH` plus `-DQT_DISABLE_NO_DEFAULT_PATH_IN_QT_PACKAGES=ON`.

To rebuild the local prefix (requires `apt-get download`, no root):

```bash
cd /tmp && mkdir ut-debs && cd ut-debs
apt-get download qt6-svg-dev qt6-5compat-dev qt6-base-private-dev \
  qt6-tools-dev qt6-tools-dev-tools qt6-l10n-tools libgmock-dev \
  libkf6codecs-dev libkf6syntaxhighlighting-dev libchardet-dev \
  libuchardet-dev libicu-dev lcov
DEST=~/ut-deps && mkdir -p "$DEST" && for d in *.deb; do dpkg-deb -x "$d" "$DEST"; done
# (run-ut.sh re-derives CMAKE_PREFIX_PATH from $UT_DEPS in ~/ut-deps/env.sh)
```

## Run a single class

```bash
cd build-autotests && ctest -R UtilsTest --output-on-failure
```

## Conventions

* Test class name: `<ClassName>Test` (no round/batch suffixes).
* Case name: `{Feature}_{Scenario}_{ExpectedResult}`.
* Source code is **never** modified; suspected source defects are recorded in
  the test report (highlighted), not fixed here.
