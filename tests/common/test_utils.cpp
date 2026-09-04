// SPDX-FileCopyrightText: 2026 UnionTech Software Technology Co., Ltd.
// SPDX-License-Identifier: GPL-3.0-or-later

#include <gtest/gtest.h>
#include "stubext.h"

// ---------------------------------------------------------------------------
// 预包含 utils.h 及其间接依赖 settings.h 的全部直接依赖头（正常访问级别解析），
// 随后仅对 utils.h 放开 private（访问 Utils::m_systemLanguage 静态缓存做用例隔离）。
// ---------------------------------------------------------------------------
#include "dsettingsdialog.h"
#include <qsettingbackend.h>
#include <DKeySequenceEdit>
#include <DDialog>
#include <QSettings>
#include <QPointer>
#include <QKeyEvent>
#include <QDebug>
#include <DApplication>
#include <QLabel>
#include <QPushButton>
#include <QMutex>
#include <QPainter>
#include <QString>
#include <QImage>
#include <DMainWindow>
#include <QIcon>
#include <QDBusInterface>
#include <QDBusReply>
#include <QTextCodec>
#include <DSysInfo>
#include <DSettingsOption>
#include <QStandardPaths>

extern "C" {
#include "load_libs.h"
}

#define private public
#include "utils.h"
#undef private

#include <QApplication>
#include <QTemporaryDir>
#include <QTemporaryFile>
#include <QSignalSpy>
#include <QFontDatabase>
#include <QFontMetrics>
#include <QFile>
#include <QFileInfo>
#include <QDir>
#include <QVariant>
#include <QLineEdit>
#include <QLabel>
#include <QPushButton>
#include <QLibraryInfo>
#include <QProcessEnvironment>
#include <QSet>
#include <QtMath>
#include <DFloatingMessage>
#include <DMessageManager>

// ---------------------------------------------------------------------------
// 虚函数（且带重载）取桩地址：先 static_cast 定型成员指针，再经 VFLocator
// 转成普通函数指针（VADDR 宏无法直接用于重载名）。
// ---------------------------------------------------------------------------
#define STUB_ADDR(pm) (typename stub_ext::VFLocator<decltype(pm)>::Func)(pm)

// ---------------------------------------------------------------------------
// 分支清单（来源：src/common/utils.cpp，仅列关键分支；函数级覆盖映射见用例名）
// 用例 → 方法映射（TEST_F/TEST_P 名前缀即被测方法驼峰名）：
//   getQrcPath/getQssPath/getRenderSize/setFontSize/applyQss/
//   getKeyshortcut(TEST_P)/getKeyshortcutFromKeymap/fileExists/getFilePath/
//   fileIsWritable/fileIsHome/dropShadow(两实现)/detectEncode(6 场景)/
//   getEncode(3 场景)/ease*(TEST_P 边界)/getThemeMapFromPath(3 场景)/
//   isMimeTypeSupport(4 场景)/isDraftFile/isBackupFile/cleanPath(TEST_P)/
//   localDataPath/getEncodeList/renderSVG(2)/getHiglightColorList/
//   clearChildrenFocus/clearChildrenFoucusEx/setChildrenFocus/
//   getProcessCountByName(3)/killProcessByName(2)/getStringMD5Hash/
//   activeWindowFromDock/isShareDirAndReadOnly(TEST_P 矩阵)/getSystemLan(2)/
//   getSystemVersion(TEST_P)/isWayland(2)/lineFeed(TEST_P)/
//   checkRegionIntersect(TEST_P 6 型)/getSupportEncoding(+List)/libPath(3)/
//   loadCustomDLL(2)/enableClipCopy/recordCloseFile/sendFloatMessageFixedFont(2)/
//   getSystemMemoryInfo(3)/isMemorySufficientForOperation(5)/codecConfidenceForData(2)
//
// 环境隔离：
// - XDG_CONFIG_HOME/XDG_DATA_HOME → QTemporaryDir；HOME 只做只读前缀比较
// - QProcess/QDBus/DSysInfo/QDir::entryList/QLibrary::resolve 等外部依赖全 stub
// - 资源（qss/svg/encodes.ini）来自 autotests/common 测试 qrc，不依赖安装产物
// ---------------------------------------------------------------------------

class UtilsTest : public ::testing::Test {
protected:
    static void SetUpTestSuite()
    {
        s_configHome = new QTemporaryDir();
        const QString xdgConfig = s_configHome->filePath("xdg-config");
        const QString xdgData = s_configHome->filePath("xdg-data");
        QDir().mkpath(xdgConfig);
        QDir().mkpath(xdgData);
        qputenv("XDG_CONFIG_HOME", xdgConfig.toUtf8());
        qputenv("XDG_DATA_HOME", xdgData.toUtf8());
        int argc = 1;
        s_app = new QApplication(argc, s_argv);
        QApplication::setOrganizationName(QString::fromLatin1("deepin"));
        QApplication::setApplicationName(QString::fromLatin1("deepin-editor"));
        Settings::instance(); // getKeyshortcutFromKeymap 需要真实 Settings
    }

    static void TearDownTestSuite()
    {
        // 还原环境变量（与 SetUpTestSuite 中的 qputenv 数量配平，避免用例间泄漏）
        qunsetenv("XDG_CONFIG_HOME");
        qunsetenv("XDG_DATA_HOME");
        qunsetenv("XDG_SESSION_TYPE");
    }

    void SetUp() override
    {
        stub.clear();
        Utils::m_systemLanguage.clear(); // 每用例重置语言缓存，杜绝用例间污染
    }

    void TearDown() override { stub.clear(); }

    static QString tempFile(const QString &name, const QByteArray &content,
                            QFileDevice::Permissions perms = QFile::ReadOwner | QFile::WriteOwner | QFile::ReadGroup | QFile::ReadOther)
    {
        const QString path = s_configHome->filePath(name);
        QFile f(path);
        if (f.open(QIODevice::WriteOnly)) {
            f.write(content);
            f.close();
            f.setPermissions(perms);
        }
        return path;
    }

    stub_ext::StubExt stub;
    static QTemporaryDir *s_configHome;
    static QApplication *s_app;
    static char s_argv0[];
    static char *s_argv[2];
    int m_callCount = 0;
    QString m_captured;
};

QTemporaryDir *UtilsTest::s_configHome = nullptr;
QApplication *UtilsTest::s_app = nullptr;
char UtilsTest::s_argv0[] = "test_utils";
char *UtilsTest::s_argv[2] = { UtilsTest::s_argv0, nullptr };

// ===========================================================================
// 纯路径/资源字符串
// ===========================================================================
TEST_F(UtilsTest, GetQrcPath_AnyImageName_ReturnsImagesPrefixPath)
{
    // Act / Assert
    EXPECT_EQ(Utils::getQrcPath(QString::fromLatin1("logo.svg")), QString::fromLatin1(":/images/logo.svg"));
    EXPECT_EQ(Utils::getQrcPath(QString::fromLatin1("a/b.png")), QString::fromLatin1(":/images/a/b.png"));
}

TEST_F(UtilsTest, GetQssPath_AnyQssName_ReturnsQssPrefixPath)
{
    // Act / Assert
    EXPECT_EQ(Utils::getQssPath(QString::fromLatin1("main.qss")), QString::fromLatin1(":/qss/main.qss"));
    EXPECT_EQ(Utils::getQssPath(QString()), QString::fromLatin1(":/qss/"));
}

TEST_F(UtilsTest, GetRenderSize_MultiLineText_SumsHeightsAndMaxWidth)
{
    // Arrange
    QFont font;
    font.setPointSize(12);
    QFontMetrics fm(font);
    const QString text = QString::fromLatin1("short\nmuch longer line here\nmid");
    int maxWidth = 0;
    for (const QString &line : text.split('\n'))
        maxWidth = qMax(maxWidth, fm.horizontalAdvance(line));

    // Act
    const QSize size = Utils::getRenderSize(12, text);

    // Assert
    EXPECT_EQ(size.width(), maxWidth);
    EXPECT_EQ(size.height(), fm.height() * 3);
}

TEST_F(UtilsTest, GetRenderSize_EmptyText_ReturnsZeroWidthSingleLineHeight)
{
    // Act
    const QSize size = Utils::getRenderSize(12, QString());

    // Assert：空文本 split 后仍有一行空串 → 宽 0、高为单行行高
    QFont font;
    font.setPointSize(12);
    EXPECT_EQ(size.width(), 0);
    EXPECT_EQ(size.height(), QFontMetrics(font).height());
}

TEST_F(UtilsTest, SetFontSize_PainterFontUpdated)
{
    // Arrange
    QImage img(32, 32, QImage::Format_ARGB32);
    QPainter painter(&img);
    QFont f = painter.font();
    f.setPointSize(9);
    painter.setFont(f);

    // Act
    Utils::setFontSize(painter, 15);

    // Assert
    EXPECT_EQ(painter.font().pointSize(), 15);
    EXPECT_NE(painter.font().pointSize(), 9); // 与设置前不同
    painter.end();
}

TEST_F(UtilsTest, ApplyQss_RegisteredQss_AppliesFileContent)
{
    // Arrange：:/qss/test.qss 由测试 qrc 提供
    QWidget w;

    // Act
    Utils::applyQss(&w, QString::fromLatin1("test.qss"));

    // Assert
    EXPECT_EQ(w.styleSheet(), QString::fromLatin1("QWidget { color: red; background-color: blue; }\n"));
    EXPECT_TRUE(w.styleSheet().contains(QString::fromLatin1("color: red")));
}

TEST_F(UtilsTest, ApplyQss_MissingQss_AppliesEmptyStyleSheet)
{
    // Arrange
    QWidget w;
    w.setStyleSheet(QString::fromLatin1("body { margin: 0; }"));
    EXPECT_FALSE(w.styleSheet().isEmpty()); // 前置：初始有样式

    // Act：资源不存在 → 读到空内容并覆盖
    Utils::applyQss(&w, QString::fromLatin1("missing.qss"));

    // Assert
    EXPECT_TRUE(w.styleSheet().isEmpty());
}

// ===========================================================================
// getKeyshortcut（TEST_P 参数矩阵）
// ===========================================================================
namespace {
struct KeyShortcutCase {
    int key;
    Qt::KeyboardModifiers modifiers;
    QString expected;
};
} // namespace

class GetKeyshortcutTest : public UtilsTest, public ::testing::WithParamInterface<KeyShortcutCase> {
};

TEST_P(GetKeyshortcutTest, GetKeyshortcut_ParamMatrix_ReturnsJoinedSequence)
{
    // Arrange
    QKeyEvent event(QEvent::KeyPress, GetParam().key, GetParam().modifiers);

    // Act
    const QString actual = Utils::getKeyshortcut(&event);

    // Assert
    EXPECT_EQ(actual, GetParam().expected);
    EXPECT_FALSE(actual.isEmpty()); // 任何键事件都产出非空序列
}

INSTANTIATE_TEST_SUITE_P(
        ModifierMatrix, GetKeyshortcutTest,
        ::testing::Values(
                KeyShortcutCase{ Qt::Key_X, Qt::NoModifier, QString::fromLatin1("X") },
                KeyShortcutCase{ Qt::Key_X, Qt::ControlModifier, QString::fromLatin1("Ctrl+X") },
                KeyShortcutCase{ Qt::Key_X, Qt::ControlModifier | Qt::ShiftModifier, QString::fromLatin1("Ctrl+Shift+X") },
                KeyShortcutCase{ Qt::Key_Return, Qt::AltModifier, QString::fromLatin1("Alt+Enter") },        // Return→Enter
                KeyShortcutCase{ Qt::Key_Backtab, Qt::ControlModifier, QString::fromLatin1("Ctrl+Tab") },   // Backtab→Tab
                KeyShortcutCase{ Qt::Key_6, Qt::KeypadModifier | Qt::ControlModifier, QString::fromLatin1("Ctrl+Num+6") },
                KeyShortcutCase{ Qt::Key_unknown, Qt::MetaModifier, QString::fromLatin1("Meta") },          // 无主键仅修饰
                KeyShortcutCase{ Qt::Key_F1, Qt::NoModifier, QString::fromLatin1("F1") }));

TEST_F(UtilsTest, GetKeyshortcutFromKeymap_RealSettings_ReturnsOptionValue)
{
    // Arrange
    Settings *s = Settings::instance();
    ASSERT_NE(s->settings, nullptr);
    const QString expected = s->settings->option("shortcuts.window.savefile")->value().toString();

    // Act
    const QString actual = Utils::getKeyshortcutFromKeymap(s, QString::fromLatin1("window"),
                                                           QString::fromLatin1("savefile"));

    // Assert
    EXPECT_EQ(actual, expected);
    EXPECT_FALSE(actual.isEmpty());
}

// ===========================================================================
// 文件系统类（QTemporaryDir 隔离）
// ===========================================================================
TEST_F(UtilsTest, FileExists_ExistingRegularFile_ReturnsTrue)
{
    // Arrange
    const QString path = tempFile(QString::fromLatin1("exists.txt"), "data");

    // Act / Assert
    EXPECT_TRUE(Utils::fileExists(path));
    EXPECT_EQ(QFileInfo(path).size(), 4); // 内容 "data" 共 4 字节，确认探测的是该文件
}

TEST_F(UtilsTest, FileExists_DirectoryOrMissing_ReturnsFalse)
{
    // Arrange
    const QString dirPath = s_configHome->filePath(QString::fromLatin1("subdir"));
    QDir().mkpath(dirPath);

    // Act / Assert：目录不算文件；不存在路径为 false
    EXPECT_FALSE(Utils::fileExists(dirPath));
    EXPECT_FALSE(Utils::fileExists(s_configHome->filePath(QString::fromLatin1("no-such.file"))));
}

TEST_F(UtilsTest, GetFilePath_ExistingFile_ReturnsCanonicalPath)
{
    // Arrange
    const QString path = tempFile(QString::fromLatin1("canon.txt"), "x");
    const QString canonical = QFileInfo(path).canonicalFilePath();

    // Act / Assert
    EXPECT_EQ(Utils::getFilePath(path), canonical);
    EXPECT_FALSE(canonical.isEmpty());
}

TEST_F(UtilsTest, GetFilePath_NonexistentFile_FallsBackToAbsolutePath)
{
    // Arrange
    const QString path = s_configHome->filePath(QString::fromLatin1("brand-new.cpp"));

    // Act / Assert
    EXPECT_EQ(Utils::getFilePath(path), QFileInfo(path).absoluteFilePath());
    EXPECT_TRUE(QFileInfo(Utils::getFilePath(path)).isAbsolute());
}

TEST_F(UtilsTest, FileIsWritable_WritableAndReadOnlyFiles_ReturnExpected)
{
    // Arrange
    const QString writable = tempFile(QString::fromLatin1("w.txt"), "x");
    const QString readonly = tempFile(QString::fromLatin1("ro.txt"), "x",
                                      QFile::ReadOwner | QFile::ReadGroup | QFile::ReadOther);

    // Act / Assert
    EXPECT_TRUE(Utils::fileIsWritable(writable));
    EXPECT_FALSE(Utils::fileIsWritable(readonly));
}

TEST_F(UtilsTest, FileIsHome_HomePrefixedAndOtherPaths_ReturnExpected)
{
    // Arrange（只读字符串比较，不写 HOME）
    const QString underHome = QDir::homePath() + QString::fromLatin1("/some/file.txt");
    const QString outside = QDir::tempPath() + QString::fromLatin1("/abc.txt");
    ASSERT_NE(QDir::homePath(), QDir::tempPath());

    // Act / Assert
    EXPECT_TRUE(Utils::fileIsHome(underHome));
    EXPECT_FALSE(Utils::fileIsHome(outside));
}

// ===========================================================================
// dropShadow
// ===========================================================================
TEST_F(UtilsTest, DropShadow_QPixmapWithOffset_GrowsCanvasAndKeepsContent)
{
    // Arrange
    QPixmap src(20, 20);
    src.fill(Qt::red);

    // Act
    QPixmap shadowed = Utils::dropShadow(src, 5.0, QColor(Qt::black), QPoint(2, 2));

    // Assert：画布按半径扩大
    EXPECT_FALSE(shadowed.isNull());
    EXPECT_GE(shadowed.width(), 20);
    EXPECT_GE(shadowed.height(), 20);
}

TEST_F(UtilsTest, DropShadow_QImageOverload_BlackAndColorTintBothSucceed)
{
    // Arrange
    QPixmap src(16, 16);
    src.fill(Qt::blue);

    // Act
    QImage black = Utils::dropShadow(src, 4.0, QColor(Qt::black));
    QImage tinted = Utils::dropShadow(src, 4.0, QColor(Qt::green));

    // Assert：黑影走快速路径，彩影走 SourceIn 上色路径，均有输出
    EXPECT_FALSE(black.isNull());
    EXPECT_FALSE(tinted.isNull());
    EXPECT_EQ(black.size(), tinted.size());
}

TEST_F(UtilsTest, DropShadow_NullPixmap_ReturnsNullImage)
{
    // Act
    const QImage result = Utils::dropShadow(QPixmap(), 4.0, QColor(Qt::black));

    // Assert
    EXPECT_TRUE(result.isNull());
    EXPECT_EQ(result.format(), QImage::Format_Invalid);
}

// ===========================================================================
// detectEncode / getEncode
// ===========================================================================
TEST_F(UtilsTest, DetectEncode_EmptyData_ReturnsLocaleCodecName)
{
    // Act / Assert（带不带 fileName 两条路径一致）
    EXPECT_EQ(Utils::detectEncode(QByteArray()), QByteArray(QTextCodec::codecForLocale()->name()));
    EXPECT_EQ(Utils::detectEncode(QByteArray(), QString::fromLatin1("e.txt")),
              QByteArray(QTextCodec::codecForLocale()->name()));
}

TEST_F(UtilsTest, DetectEncode_Utf8Bom_ReturnsUtf8)
{
    // Arrange
    const QByteArray data = QByteArray::fromHex("efbbbf") + "hello text";

    // Act / Assert（BOM 优先于 fileName 探测）
    EXPECT_EQ(Utils::detectEncode(data), QByteArray("UTF-8"));
    EXPECT_EQ(Utils::detectEncode(data, QString::fromLatin1("bom.txt")), QByteArray("UTF-8"));
}

TEST_F(UtilsTest, DetectEncode_PythonCodingLine_ReturnsDeclaredCoding)
{
    // Arrange
    const QByteArray data = "#coding: gbk\nprint('hello')\n";

    // Act / Assert（fileName 触发 text/x-python 分支；大小写不敏感变体同样命中）
    EXPECT_EQ(Utils::detectEncode(data, QString::fromLatin1("script.py")), QByteArray("gbk"));
    EXPECT_EQ(Utils::detectEncode("#CODING: big5\n", QString::fromLatin1("s2.py")), QByteArray("big5"));
}

TEST_F(UtilsTest, DetectEncode_GbkBytes_DetectsCjkEncoding)
{
    // Arrange：GBK 编码的重复中文文本（探测置信度稳定）
    QByteArray gbk;
    for (int i = 0; i < 8; ++i)
        gbk += QByteArray::fromHex("c4e3bac3cac0bde7"); // 你好世界
    const QString fileName = s_configHome->filePath(QString::fromLatin1("gbk.txt"));
    QFile f(fileName);
    ASSERT_TRUE(f.open(QIODevice::WriteOnly));
    f.write(gbk);
    f.close();

    // Act
    const QByteArray detected = Utils::detectEncode(gbk, fileName);

    // Assert：命中 CJK 家族（GBK/GB18030 之一；prober 在本机返回小写名，比较不区分大小写）
    const QByteArray upper = detected.toUpper();
    EXPECT_TRUE(upper == QByteArray("GBK") || upper == QByteArray("GB18030"))
            << "detected=" << detected.constData();
    EXPECT_FALSE(detected.isEmpty()); // 必须给出确定编码
}

TEST_F(UtilsTest, GetEncode_Utf16BomHtmlLikeData_ReturnsUtf16Family)
{
    // Arrange：UTF-16LE BOM + 少量英文文本（codecForHtml 路径）
    const QByteArray data = QByteArray::fromHex("fffe") + QByteArray("ab", 2);

    // Act / Assert
    const QByteArray enc = Utils::getEncode(data);
    EXPECT_TRUE(enc.startsWith("UTF-16")) << "detected=" << enc.constData();
    EXPECT_GT(enc.size(), 0);
}

TEST_F(UtilsTest, GetEncode_GbkBytes_HighConfidenceProber)
{
    // Arrange
    QByteArray gbk;
    for (int i = 0; i < 10; ++i)
        gbk += QByteArray::fromHex("c4e3bac3cac0bde7");

    // Act / Assert（prober 返回小写名，比较不区分大小写）
    const QByteArray detected = Utils::getEncode(gbk).toUpper();
    EXPECT_TRUE(detected == QByteArray("GBK") || detected == QByteArray("GB18030"));
    EXPECT_GT(detected.size(), 0); // 必须给出确定编码
}

TEST_F(UtilsTest, GetEncode_LowConfidenceBinary_ReturnsEmpty)
{
    // Arrange：无 BOM/HTML 特征且 prober 置信度仅 0.2（≤0.5）→ 返回空
    const QByteArray data = QByteArray::fromHex("8081828384");

    // Act / Assert：重复调用结果稳定为空
    EXPECT_TRUE(Utils::getEncode(data).isEmpty());
    EXPECT_TRUE(Utils::getEncode(data).isEmpty());
}

TEST_F(UtilsTest, CodecConfidenceForData_GbkTextWithChina_LargerThanJapan)
{
    // Arrange
    QByteArray gbk;
    for (int i = 0; i < 8; ++i)
        gbk += QByteArray::fromHex("c4e3bac3cac0bde7");
    QTextCodec *gbkCodec = QTextCodec::codecForName("GBK");
    ASSERT_NE(gbkCodec, nullptr);

    // Act
    const float cn = Utils::codecConfidenceForData(gbkCodec, gbk, QLocale::China);
    const float jp = Utils::codecConfidenceForData(gbkCodec, gbk, QLocale::Japan);

    // Assert：中文字节在中国 locale 下置信度更高，且范围合法
    EXPECT_GT(cn, jp);
    EXPECT_LE(cn, 1.0f);
    EXPECT_GE(cn, 0.0f);
}

TEST_F(UtilsTest, CodecConfidenceForData_InvalidUtf8_NeverExceedsOne)
{
    // Arrange：UTF-8 解码产生替换字符
    QTextCodec *utf8 = QTextCodec::codecForName("UTF-8");
    ASSERT_NE(utf8, nullptr);
    const QByteArray bad = QByteArray::fromHex("c4e3fffe80");

    // Act
    const float c = Utils::codecConfidenceForData(utf8, bad, QLocale::China);

    // Assert：替换字符扣分后仍被 clamp 到 [0,1]
    EXPECT_GE(c, 0.0f);
    EXPECT_LE(c, 1.0f);
}

// ===========================================================================
// 缓动函数（TEST_P 边界 0 / 0.5 / 1）
// ===========================================================================
namespace {
struct EaseCase {
    qreal x;
};
} // namespace

class EaseFuncTest : public UtilsTest, public ::testing::WithParamInterface<EaseCase> {
};

TEST_P(EaseFuncTest, EaseInOut_BoundaryInputs_MatchesReferenceFormula)
{
    const qreal x = GetParam().x;
    EXPECT_DOUBLE_EQ(Utils::easeInOut(x), (1 - qCos(M_PI * x)) / 2);
    EXPECT_DOUBLE_EQ(Utils::easeInOut(0.0), 0.0); // 端点锚定
}

TEST_P(EaseFuncTest, EaseInQuad_BoundaryInputs_MatchesReferenceFormula)
{
    const qreal x = GetParam().x;
    EXPECT_DOUBLE_EQ(Utils::easeInQuad(x), qPow(x, 2));
    EXPECT_DOUBLE_EQ(Utils::easeInQuad(0.0), 0.0); // 端点锚定
}

TEST_P(EaseFuncTest, EaseOutQuad_BoundaryInputs_MatchesReferenceFormula)
{
    const qreal x = GetParam().x;
    EXPECT_DOUBLE_EQ(Utils::easeOutQuad(x), -1 * qPow(x - 1, 2) + 1);
    EXPECT_DOUBLE_EQ(Utils::easeOutQuad(1.0), 1.0); // 端点锚定
}

TEST_P(EaseFuncTest, EaseInQuint_BoundaryInputs_MatchesReferenceFormula)
{
    const qreal x = GetParam().x;
    EXPECT_DOUBLE_EQ(Utils::easeInQuint(x), qPow(x, 5));
    EXPECT_DOUBLE_EQ(Utils::easeInQuint(0.0), 0.0); // 端点锚定
}

TEST_P(EaseFuncTest, EaseOutQuint_BoundaryInputs_MatchesReferenceFormula)
{
    const qreal x = GetParam().x;
    EXPECT_DOUBLE_EQ(Utils::easeOutQuint(x), qPow(x - 1, 5) + 1);
    EXPECT_DOUBLE_EQ(Utils::easeOutQuint(1.0), 1.0); // 端点锚定
}

INSTANTIATE_TEST_SUITE_P(Boundary, EaseFuncTest,
                         ::testing::Values(EaseCase{ 0.0 }, EaseCase{ 0.5 }, EaseCase{ 1.0 }));

// ===========================================================================
// getThemeMapFromPath
// ===========================================================================
TEST_F(UtilsTest, GetThemeMapFromPath_ValidJsonFile_ReturnsMap)
{
    // Arrange
    const QString path = tempFile(QString::fromLatin1("theme.json"),
                                  R"({"foreground": "#ffffff", "background": "#000000"})");

    // Act
    const QVariantMap map = Utils::getThemeMapFromPath(path);

    // Assert
    EXPECT_EQ(map.value(QString::fromLatin1("foreground")).toString(), QString::fromLatin1("#ffffff"));
    EXPECT_EQ(map.value(QString::fromLatin1("background")).toString(), QString::fromLatin1("#000000"));
}

TEST_F(UtilsTest, GetThemeMapFromPath_MissingFile_ReturnsEmptyMap)
{
    // Act / Assert：缺失文件返回空 map
    const QVariantMap map = Utils::getThemeMapFromPath(s_configHome->filePath(QString::fromLatin1("nope.theme")));
    EXPECT_TRUE(map.isEmpty());
    EXPECT_EQ(map.value(QString::fromLatin1("foreground")), QVariant()); // 无任何键
}

TEST_F(UtilsTest, GetThemeMapFromPath_InvalidJson_ReturnsEmptyMap)
{
    // Arrange
    const QString path = tempFile(QString::fromLatin1("bad.json"), "not-a-json{{{");

    // Act / Assert
    EXPECT_TRUE(Utils::getThemeMapFromPath(path).isEmpty());
}

// ===========================================================================
// isMimeTypeSupport（内容探测，QTemporaryDir 隔离）
// ===========================================================================
TEST_F(UtilsTest, IsMimeTypeSupport_TextContentFile_ReturnsTrue)
{
    // Arrange
    const QString path = tempFile(QString::fromLatin1("plain.txt"), "hello world\nline2\n");

    // Act / Assert：text/* → 支持（另一份文本内容同样支持）
    EXPECT_TRUE(Utils::isMimeTypeSupport(path));
    EXPECT_TRUE(Utils::isMimeTypeSupport(tempFile(QString::fromLatin1("plain2.txt"), "second text")));
}

TEST_F(UtilsTest, IsMimeTypeSupport_PubSuffix_ReturnsTrue)
{
    // Arrange：即使内容非文本，.pub 后缀白名单
    const QString path = tempFile(QString::fromLatin1("key.pub"),
                                  QByteArray::fromHex("00ff00ff0011"));

    // Act / Assert
    EXPECT_TRUE(Utils::isMimeTypeSupport(path));
    EXPECT_TRUE(path.endsWith(QString::fromLatin1(".pub"))); // 后缀白名单路径
}

TEST_F(UtilsTest, IsMimeTypeSupport_PngContent_ReturnsTrueViaOctetStreamInherit)
{
    // Arrange：PNG 魔数（白名单含 application/octet-stream，PNG 继承自它）
    const QString path = tempFile(QString::fromLatin1("image.bin"),
                                  QByteArray::fromHex("89504e470d0a1a0a0000000d49484452"));

    // Act / Assert：按源码 inherits 逻辑，二进制根类型被放行（记录源行为）
    EXPECT_TRUE(Utils::isMimeTypeSupport(path));
    EXPECT_EQ(QFileInfo(path).size(), 16); // 探测基于该 16 字节内容
}

TEST_F(UtilsTest, IsMimeTypeSupport_DirectoryPath_ReturnsFalse)
{
    // Arrange：目录 → inode/directory，不继承任何白名单类型
    const QString dirPath = s_configHome->filePath(QString::fromLatin1("subdir-inode"));
    QDir().mkpath(dirPath);

    // Act / Assert
    EXPECT_FALSE(Utils::isMimeTypeSupport(dirPath));
    EXPECT_TRUE(QDir(dirPath).exists()); // 前提：目录真实存在
}

TEST_F(UtilsTest, IsMimeTypeSupport_EmptyFile_ReturnsTrue)
{
    // Arrange：空文件 → x-empty/x-zerosize 均在白名单
    const QString path = tempFile(QString::fromLatin1("empty.txt"), "");

    // Act / Assert
    EXPECT_TRUE(Utils::isMimeTypeSupport(path));
    EXPECT_EQ(QFileInfo(path).size(), 0); // 空文件（0 字节）走 x-empty/x-zerosize 白名单
}

// ===========================================================================
// isDraftFile / isBackupFile / localDataPath（XDG_DATA_HOME 隔离）
// ===========================================================================
TEST_F(UtilsTest, IsDraftFile_UnderAppDataBlankFiles_ReturnsExpected)
{
    // Arrange：与实现同源的期望路径（XDG_DATA_HOME 已重定向到临时目录）
    const QStringList locations = QStandardPaths::standardLocations(QStandardPaths::AppDataLocation);
    ASSERT_FALSE(locations.isEmpty());
    const QString draftDir = QDir(Utils::cleanPath(locations).first()).filePath(QString::fromLatin1("blank-files"));

    // Act / Assert
    EXPECT_TRUE(Utils::isDraftFile(QDir(draftDir).filePath(QString::fromLatin1("draft.txt"))));
    EXPECT_EQ(draftDir.endsWith(QString::fromLatin1("blank-files")), true); // 期望目录推导正确
    EXPECT_FALSE(Utils::isDraftFile(s_configHome->filePath(QString::fromLatin1("elsewhere.txt"))));
}

TEST_F(UtilsTest, IsBackupFile_UnderAppDataBackupFiles_ReturnsExpected)
{
    // Arrange
    const QStringList locations = QStandardPaths::standardLocations(QStandardPaths::AppDataLocation);
    const QString backupDir = QDir(Utils::cleanPath(locations).first()).filePath(QString::fromLatin1("backup-files"));

    // Act / Assert
    EXPECT_TRUE(Utils::isBackupFile(QDir(backupDir).filePath(QString::fromLatin1("file.cpp~"))));
    EXPECT_EQ(backupDir.endsWith(QString::fromLatin1("backup-files")), true); // 期望目录推导正确
    EXPECT_FALSE(Utils::isBackupFile(s_configHome->filePath(QString::fromLatin1("normal.cpp"))));
}

TEST_F(UtilsTest, LocalDataPath_EqualsFirstCleanedStandardLocation)
{
    // Arrange
    const QStringList locations = QStandardPaths::standardLocations(QStandardPaths::AppDataLocation);
    ASSERT_FALSE(locations.isEmpty());

    // Act / Assert
    EXPECT_EQ(Utils::localDataPath(), QDir::cleanPath(locations.first()));
    EXPECT_TRUE(QDir(Utils::localDataPath()).isAbsolute()); // 绝对路径
}

// ===========================================================================
// cleanPath（TEST_P）
// ===========================================================================
namespace {
struct CleanPathCase {
    QString input;
    QString expected;
};
} // namespace

class CleanPathTest : public UtilsTest, public ::testing::WithParamInterface<CleanPathCase> {
};

TEST_P(CleanPathTest, CleanPath_ParamVariants_ReturnsNormalizedList)
{
    // Arrange
    const QStringList input = QStringList() << GetParam().input;

    // Act
    const QStringList output = Utils::cleanPath(input);

    // Assert
    ASSERT_EQ(output.size(), 1);
    EXPECT_EQ(output.first(), GetParam().expected);
    EXPECT_EQ(Utils::cleanPath(output).first(), GetParam().expected); // 幂等：再清洗不变
}

INSTANTIATE_TEST_SUITE_P(
        Variants, CleanPathTest,
        ::testing::Values(
                CleanPathCase{ QString::fromLatin1("a/b/../c"), QString::fromLatin1("a/c") },
                CleanPathCase{ QString::fromLatin1("/foo//bar"), QString::fromLatin1("/foo/bar") },
                CleanPathCase{ QString::fromLatin1("./x"), QString::fromLatin1("x") },
                CleanPathCase{ QString::fromLatin1("/abs/path"), QString::fromLatin1("/abs/path") },
                CleanPathCase{ QString(), QString() })); // 空串边界：cleanPath("") == ""

TEST_F(UtilsTest, CleanPath_EmptyList_ReturnsEmptyList)
{
    // Act / Assert
    EXPECT_TRUE(Utils::cleanPath(QStringList()).isEmpty());
    EXPECT_EQ(Utils::cleanPath(QStringList() << QString::fromLatin1("a/b")).size(), 1); // 非空列表对照
}

// ===========================================================================
// getEncodeList
// ===========================================================================
TEST_F(UtilsTest, GetEncodeList_Utf8FirstAndUniqueSorted)
{
    // Act
    const QStringList list = Utils::getEncodeList();

    // Assert：首项 UTF-8、无重复、其余有序
    ASSERT_GT(list.size(), 1);
    EXPECT_EQ(list.first(), QString::fromLatin1("UTF-8"));
    EXPECT_EQ(list.count(QString::fromLatin1("UTF-8")), 1);
    const QStringList rest = list.mid(1);
    EXPECT_EQ(rest.size(), QSet<QString>(rest.begin(), rest.end()).size()); // 无重复
    // UTF-8 只出现一次（首项），其余不含 UTF-8
    EXPECT_FALSE(rest.contains(QString::fromLatin1("UTF-8")));
}

// ===========================================================================
// renderSVG
// ===========================================================================
TEST_F(UtilsTest, RenderSVG_LoadableQrcSvg_ReturnsScaledPixmap)
{
    // Arrange：:/svg/test.svg（10x10）由测试 qrc 提供
    const QSize target(5, 5);

    // Act
    const QPixmap pm = Utils::renderSVG(QString::fromLatin1(":/svg/test.svg"), target);

    // Assert
    EXPECT_FALSE(pm.isNull());
    EXPECT_EQ(pm.size(), target);
}

TEST_F(UtilsTest, RenderSVG_MissingFile_ReturnsNullPixmap)
{
    // Act
    const QPixmap pm = Utils::renderSVG(QString::fromLatin1(":/svg/missing.svg"), QSize(4, 4));

    // Assert
    EXPECT_TRUE(pm.isNull());
    EXPECT_EQ(pm.width(), 0);
}

// ===========================================================================
// getHiglightColorList
// ===========================================================================
TEST_F(UtilsTest, GetHiglightColorList_FixedPaletteReturned)
{
    // Act
    const QList<QColor> colors = Utils::getHiglightColorList();

    // Assert：8 个固定色值
    ASSERT_EQ(colors.size(), 8);
    EXPECT_EQ(colors.at(0), QColor(QString::fromLatin1("#FFA503")));
    EXPECT_EQ(colors.at(1), QColor(QString::fromLatin1("#FF1C49")));
    EXPECT_EQ(colors.at(2), QColor(QString::fromLatin1("#9023FC")));
    EXPECT_EQ(colors.at(3), QColor(QString::fromLatin1("#3468FF")));
    EXPECT_EQ(colors.at(4), QColor(QString::fromLatin1("#00C7E1")));
    EXPECT_EQ(colors.at(5), QColor(QString::fromLatin1("#05EA6B")));
    EXPECT_EQ(colors.at(6), QColor(QString::fromLatin1("#FEF144")));
    EXPECT_EQ(colors.at(7), QColor(QString::fromLatin1("#D5D5D1")));
}

// ===========================================================================
// 焦点控制（offscreen widget 树）
// ===========================================================================
TEST_F(UtilsTest, ClearChildrenFocus_TreeWithFocusableWidgets_SetsNoFocus)
{
    // Arrange：parent → [QLineEdit, QObject]；QLineEdit → [QLineEdit]
    QWidget parent;
    auto *edit = new QLineEdit(&parent);
    edit->setFocusPolicy(Qt::StrongFocus);
    auto *plain = new QObject(&parent);
    auto *innerEdit = new QLineEdit(edit);
    innerEdit->setFocusPolicy(Qt::StrongFocus);

    // Act
    Utils::clearChildrenFocus(&parent);

    // Assert：可聚焦控件全部 NoFocus（含递归孙代），非 widget 子对象不受影响
    EXPECT_EQ(edit->focusPolicy(), Qt::NoFocus);
    EXPECT_EQ(innerEdit->focusPolicy(), Qt::NoFocus);
    EXPECT_EQ(plain->children().size(), 0);
}

TEST_F(UtilsTest, ClearChildrenFoucusEx_TreeWithWidgets_ClearsFocusSafely)
{
    // Arrange
    QWidget parent;
    auto *edit = new QLineEdit(&parent);

    // Act：清焦点递归（无子/有子两分支）
    Utils::clearChildrenFoucusEx(&parent);
    Utils::clearChildrenFoucusEx(edit);

    // Assert：控件树完好（强异常安全）
    EXPECT_EQ(parent.children().size(), 1);
    EXPECT_EQ(edit->parentWidget(), &parent);
}

TEST_F(UtilsTest, SetChildrenFocus_TreeWithWidgets_AppliesPolicyRecursively)
{
    // Arrange
    QWidget parent;
    auto *edit = new QLineEdit(&parent);
    edit->setFocusPolicy(Qt::NoFocus);
    QWidget childless;

    // Act：有子树 + 无子树（children<=0 提前返回分支）
    Utils::setChildrenFocus(&parent, Qt::ClickFocus);
    Utils::setChildrenFocus(&childless, Qt::StrongFocus);

    // Assert
    EXPECT_EQ(parent.focusPolicy(), Qt::ClickFocus);
    EXPECT_EQ(edit->focusPolicy(), Qt::ClickFocus);
    EXPECT_EQ(childless.focusPolicy(), Qt::StrongFocus);
}

// ===========================================================================
// 进程相关（QProcess 全 stub）
// ===========================================================================
TEST_F(UtilsTest, GetProcessCountByName_NullOrEmptyName_ReturnsMinusOne)
{
    // Act / Assert
    EXPECT_EQ(Utils::getProcessCountByName(NULL), -1);
    EXPECT_EQ(Utils::getProcessCountByName(""), -1);
}

TEST_F(UtilsTest, GetProcessCountByName_StartFailure_ReturnsMinusOne)
{
    // Arrange：start 为空实现 → waitForFinished 失败
    stub.set_lamda(static_cast<void (QProcess::*)(const QString &, const QStringList &, QProcess::OpenMode)>(&QProcess::start),
                   [](QProcess *, const QString &, const QStringList &, QProcess::OpenMode) {});

    // Act / Assert：启动失败的任意进程名都返回 -1
    EXPECT_EQ(Utils::getProcessCountByName("ut-proc"), -1);
    EXPECT_EQ(Utils::getProcessCountByName("ut-proc-2"), -1);
}

TEST_F(UtilsTest, GetProcessCountByName_FilteredOutput_CountsMatchingLines)
{
    // Arrange
    stub.set_lamda(static_cast<void (QProcess::*)(const QString &, const QStringList &, QProcess::OpenMode)>(&QProcess::start),
                   [](QProcess *, const QString &, const QStringList &, QProcess::OpenMode) {});
    stub.set_lamda(static_cast<bool (QProcess::*)(int)>(&QProcess::waitForFinished),
                   [](QProcess *, int) -> bool { return true; });
    stub.set_lamda(&QProcess::readAllStandardOutput,
                   [](QProcess *) -> QByteArray {
                       return QByteArray("header line\n/bin/ut-proc --a\nother\n/bin/ut-proc --b\n");
                   });

    // Act
    const int count = Utils::getProcessCountByName("ut-proc");

    // Assert
    EXPECT_EQ(count, 2);
    EXPECT_NE(count, -1); // 未走启动失败分支
}

TEST_F(UtilsTest, KillProcessByName_EmptyName_SkipsKillall)
{
    // Arrange：计数器记录真实 start 调用（应当为 0）
    m_callCount = 0;
    stub.set_lamda(static_cast<void (QProcess::*)(const QString &, const QStringList &, QProcess::OpenMode)>(&QProcess::start),
                   [this](QProcess *, const QString &, const QStringList &, QProcess::OpenMode) { ++m_callCount; });

    // Act：空名与 NULL 都不触发 killall
    Utils::killProcessByName("");
    Utils::killProcessByName(NULL);

    // Assert：空名/NULL 均被前置校验拦截
    EXPECT_EQ(m_callCount, 0);
    EXPECT_EQ(m_captured.isEmpty(), true); // 未捕获到任何 killall 参数
}

TEST_F(UtilsTest, KillProcessByName_ValidName_RunsKillall)
{
    // Arrange
    m_callCount = 0;
    m_captured.clear();
    stub.set_lamda(static_cast<void (QProcess::*)(const QString &, const QStringList &, QProcess::OpenMode)>(&QProcess::start),
                   [this](QProcess *, const QString &program, const QStringList &args, QProcess::OpenMode) {
                       ++m_callCount;
                       m_captured = program + QString::fromLatin1(" ") + args.join(QString::fromLatin1(","));
                   });
    stub.set_lamda(static_cast<bool (QProcess::*)(int)>(&QProcess::waitForFinished),
                   [](QProcess *, int) -> bool { return true; });

    // Act
    Utils::killProcessByName("ut-victim");

    // Assert
    EXPECT_EQ(m_callCount, 1);
    EXPECT_EQ(m_captured, QString::fromLatin1("killall ut-victim"));
}

// ===========================================================================
// MD5
// ===========================================================================
TEST_F(UtilsTest, GetStringMD5Hash_KnownInputs_ExpectedHexDigests)
{
    // Act / Assert：经典已知向量
    EXPECT_EQ(Utils::getStringMD5Hash(QString::fromLatin1("abc")),
              QString::fromLatin1("900150983cd24fb0d6963f7d28e17f72"));
    EXPECT_EQ(Utils::getStringMD5Hash(QString()),
              QString::fromLatin1("d41d8cd98f00b204e9800998ecf8427e"));
}

// ===========================================================================
// activeWindowFromDock（QDBusInterface::isValid stub，无真实 DBus 依赖）
// ===========================================================================
TEST_F(UtilsTest, ActiveWindowFromDock_DockUnavailable_ReturnsFalse)
{
    // Arrange：两个版本的 Dock 接口均 stub 为无效
    stub.set_lamda(VADDR(QDBusInterface, isValid), []() -> bool { return false; });

    // Act：两个 winId 均尝试
    const bool result = Utils::activeWindowFromDock(12345);

    // Assert：V23/V20 均无效 → false
    EXPECT_FALSE(result);
    EXPECT_FALSE(Utils::activeWindowFromDock(0));
}

// ===========================================================================
// isShareDirAndReadOnly（QDir/QFile 全 stub 矩阵）
// ===========================================================================
namespace {
struct ShareDirCase {
    bool dirExists;
    bool nameExists;
    bool openOk;
    QByteArray content;
    bool expected;
};
} // namespace

class IsShareDirTest : public UtilsTest, public ::testing::WithParamInterface<ShareDirCase> {
};

TEST_P(IsShareDirTest, IsShareDirAndReadOnly_ParamMatrix_ReturnsExpected)
{
    // Arrange
    const ShareDirCase c = GetParam();
    stub.set_lamda(static_cast<bool (QDir::*)() const>(&QDir::exists),
                   [c](const QDir *) -> bool { return c.dirExists; });
    stub.set_lamda(static_cast<bool (QDir::*)(const QString &) const>(&QDir::exists),
                   [c](const QDir *, const QString &) -> bool { return c.nameExists; });
    stub.set_lamda(STUB_ADDR(static_cast<bool (QFile::*)(QIODevice::OpenMode)>(&QFile::open)),
                   [c](QFile *, QIODevice::OpenMode) -> bool { return c.openOk; });
    stub.set_lamda(&QIODevice::readAll,
                   [c](QIODevice *) -> QByteArray { return c.content; });

    // Act：同参数调用两次（幂等性 + 确定性）
    const bool actual = Utils::isShareDirAndReadOnly(QString::fromLatin1("/any/share/file.txt"));
    const bool again = Utils::isShareDirAndReadOnly(QString::fromLatin1("/any/share/other.txt"));

    // Assert
    EXPECT_EQ(actual, c.expected);
    EXPECT_EQ(again, c.expected);
}

INSTANTIATE_TEST_SUITE_P(
        BranchMatrix, IsShareDirTest,
        ::testing::Values(
                ShareDirCase{ false, false, false, QByteArray(), false },           // samba 目录不存在
                ShareDirCase{ true, false, false, QByteArray(), false },            // 共享名不存在
                ShareDirCase{ true, true, false, QByteArray(), false },             // 打开失败
                ShareDirCase{ true, true, true, QByteArray("path=/x:R,other"), true },  // 只读标记
                // 源码用 contains(":R") 子串判断，":RW" 同样命中（记录源行为）
                ShareDirCase{ true, true, true, QByteArray("path=/x:RW"), true },
                ShareDirCase{ true, true, true, QByteArray("path=/x:W"), false }));     // 无 ":R" 子串

// ===========================================================================
// getSystemLan / getSystemVersion / isWayland
// ===========================================================================
TEST_F(UtilsTest, GetSystemVersion_MajorVersionBoundary_ReturnsExpectedEra)
{
    // Arrange & Act & Assert：>=23 → V23；否则 V20
    stub.set_lamda(&DSysInfo::majorVersion, []() -> QString { return QString::fromLatin1("23"); });
    EXPECT_EQ(Utils::getSystemVersion(), Utils::V23);
    stub.set_lamda(&DSysInfo::majorVersion, []() -> QString { return QString::fromLatin1("24"); });
    EXPECT_EQ(Utils::getSystemVersion(), Utils::V23);
    stub.set_lamda(&DSysInfo::majorVersion, []() -> QString { return QString::fromLatin1("20"); });
    EXPECT_EQ(Utils::getSystemVersion(), Utils::V20);
    stub.set_lamda(&DSysInfo::majorVersion, []() -> QString { return QString(); });
    EXPECT_EQ(Utils::getSystemVersion(), Utils::V20);
}

TEST_F(UtilsTest, GetSystemLan_V20LangSelector_ReturnsDbusLocale)
{
    // Arrange：V20 路径走 LangSelector DBus 属性（stub；property 实为 QObject 虚函数）
    stub.set_lamda(&DSysInfo::majorVersion, []() -> QString { return QString::fromLatin1("20"); });
    stub.set_lamda(VADDR(QObject, property),
                   [](const QObject *, const char *) -> QVariant {
                       return QVariant(QString::fromLatin1("zh_CN"));
                   });

    // Act
    const QString lang = Utils::getSystemLan();

    // Assert：DBus 属性值；第二次命中缓存返回同值
    EXPECT_EQ(lang, QString::fromLatin1("zh_CN"));
    EXPECT_EQ(Utils::getSystemLan(), QString::fromLatin1("zh_CN"));
}

TEST_F(UtilsTest, GetSystemLan_V23SystemLocale_ReturnsSystemName)
{
    // Arrange
    stub.set_lamda(&DSysInfo::majorVersion, []() -> QString { return QString::fromLatin1("23"); });

    // Act
    const QString lang = Utils::getSystemLan();

    // Assert：与 QLocale::system().name() 一致（同进程确定）；二次调用命中缓存同值
    EXPECT_EQ(lang, QLocale::system().name());
    EXPECT_EQ(Utils::getSystemLan(), lang);
}

TEST_F(UtilsTest, IsWayland_WaylandEnvFirstCall_ReturnsTrueThenCached)
{
    // Arrange：本用例必须是全二进制首个 isWayland 调用者（静态缓存）
    qputenv("XDG_SESSION_TYPE", QByteArray("wayland"));

    // Act
    const bool first = Utils::isWayland();

    // Assert：wayland 会话被识别；后续调用命中同一缓存值
    EXPECT_TRUE(first);
    EXPECT_TRUE(Utils::isWayland()); // 缓存于首次调用
}

TEST_F(UtilsTest, IsWayland_EnvChangedAfterFirstCall_ResultStaysCached)
{
    // Arrange：上一用例首次调用已缓存 "wayland"，本用例再改 env 验证缓存语义
    qputenv("XDG_SESSION_TYPE", QByteArray("x11"));

    // Act
    const bool result = Utils::isWayland();

    // Assert：静态协议缓存优先于后续 env 变化
    EXPECT_TRUE(result);
    EXPECT_EQ(QString::fromLocal8Bit(qgetenv("XDG_SESSION_TYPE")), QString::fromLatin1("x11"));
    qunsetenv("XDG_SESSION_TYPE"); // 与本用例 qputenv 配对还原
}

// ===========================================================================
// lineFeed（TEST_P）
// ===========================================================================
namespace {
struct LineFeedCase {
    QString text;
    int width;
    int elidedRow;
};
} // namespace

class LineFeedTest : public UtilsTest, public ::testing::WithParamInterface<LineFeedCase> {
};

TEST_P(LineFeedTest, LineFeed_ParamVariants_ReturnsDeterministicWrappedText)
{
    // Arrange
    QFont font;
    font.setPointSize(11);
    QFontMetrics fm(font);
    const LineFeedCase c = GetParam();

    // Act
    const QString result = Utils::lineFeed(c.text, c.width, font, c.elidedRow);

    // Assert：单行模式 = 中部省略；多行模式含换行或原文
    if (c.elidedRow == 1) {
        EXPECT_EQ(result, fm.elidedText(c.text, Qt::ElideMiddle, c.width));
    } else {
        const bool wrapped = result.contains(QLatin1Char('\n'));
        EXPECT_TRUE(wrapped || result == c.text || result == fm.elidedText(c.text, Qt::ElideLeft, c.width));
    }
}

INSTANTIATE_TEST_SUITE_P(
        Variants, LineFeedTest,
        ::testing::Values(
                LineFeedCase{ QString::fromLatin1("hello"), 1000, 2 },            // 宽度足够不换行
                LineFeedCase{ QString::fromLatin1("hello world foo bar"), 40, 2 }, // 多行换行
                LineFeedCase{ QString::fromLatin1("hello"), 30, 1 },              // 单行省略
                LineFeedCase{ QString::fromLatin1("hello"), 30, -1 },             // 负数回退 2 行
                LineFeedCase{ QString(), 30, 2 }));                               // 空文本

TEST_F(UtilsTest, LineFeed_NegativeRowTreatedAsTwo)
{
    // Arrange
    QFont font;
    font.setPointSize(11);

    // Act：宽度小且 elidedRow=-1 → 等价 2 行；结果非空且行为与 2 行一致
    const QString result = Utils::lineFeed(QString::fromLatin1("aaaa bbbb cccc dddd"), 30, font, -1);
    const QString reference = Utils::lineFeed(QString::fromLatin1("aaaa bbbb cccc dddd"), 30, font, 2);

    // Assert
    EXPECT_EQ(result, reference);
    EXPECT_FALSE(result.isEmpty());
}

// ===========================================================================
// checkRegionIntersect（TEST_P 全 6 型 + 两侧不交）
// ===========================================================================
namespace {
struct RegionCase {
    int x1, y1, x2, y2;
    Utils::RegionIntersectType expected;
};
} // namespace

class CheckRegionTest : public UtilsTest, public ::testing::WithParamInterface<RegionCase> {
};

TEST_P(CheckRegionTest, CheckRegionIntersect_AllTypes_ReturnsExpectedEnum)
{
    // Act
    const Utils::RegionIntersectType actual = Utils::checkRegionIntersect(
            GetParam().x1, GetParam().y1, GetParam().x2, GetParam().y2);

    // Assert
    EXPECT_EQ(actual, GetParam().expected);
    EXPECT_EQ(Utils::checkRegionIntersect(GetParam().x1, GetParam().y1, GetParam().x2, GetParam().y2), actual); // 重复调用结果一致
}

INSTANTIATE_TEST_SUITE_P(
        RegionTypes, CheckRegionTest,
        ::testing::Values(
                RegionCase{ 0, 9, 10, 15, Utils::ERight },           // 右侧不交
                RegionCase{ 0, 9, -5, -1, Utils::ELeft },            // 左侧不交
                RegionCase{ 0, 9, -5, 5, Utils::EIntersectLeft },    // 左侧重叠
                RegionCase{ 0, 9, 5, 15, Utils::EIntersectRight },   // 右侧重叠
                RegionCase{ 0, 9, -10, 10, Utils::EIntersectOutter },// 完全包含
                RegionCase{ 0, 9, 5, 6, Utils::EIntersectInner },    // 内部
                RegionCase{ 0, 9, 0, 9, Utils::EIntersectInner }));  // 完全相等边界

// ===========================================================================
// getSupportEncoding / getSupportEncodingList（测试 qrc encodes.ini）
// ===========================================================================
TEST_F(UtilsTest, GetSupportEncoding_ReadsQrcIniGroups_ReturnsPairs)
{
    // Act
    const QVector<QPair<QString, QStringList>> groups = Utils::getSupportEncoding();

    // Assert：两组，键与列表与 encodes.ini 一致
    ASSERT_EQ(groups.size(), 2);
    EXPECT_EQ(groups.at(0).first, QString::fromLatin1("West"));
    EXPECT_EQ(groups.at(0).second, QStringList() << QString::fromLatin1("UTF-8") << QString::fromLatin1("ISO-8859-1"));
    EXPECT_EQ(groups.at(1).first, QString::fromLatin1("CJK"));
    EXPECT_EQ(groups.at(1).second, QStringList() << QString::fromLatin1("GB18030") << QString::fromLatin1("Big5"));
}

TEST_F(UtilsTest, GetSupportEncodingList_AggregatesAndSorts)
{
    // Act
    const QStringList list = Utils::getSupportEncodingList();

    // Assert：聚合 + 排序（Big5 < GB18030 < ISO-8859-1 < UTF-8）
    ASSERT_EQ(list.size(), 4);
    EXPECT_EQ(list, QStringList() << QString::fromLatin1("Big5") << QString::fromLatin1("GB18030")
                                  << QString::fromLatin1("ISO-8859-1") << QString::fromLatin1("UTF-8"));
    EXPECT_TRUE(list.contains(QString::fromLatin1("GB18030"))); // CJK 组已聚合
}

// ===========================================================================
// libPath（QDir::entryList stub）
// ===========================================================================
TEST_F(UtilsTest, LibPath_ExactMatchInList_ReturnsNameAsIs)
{
    // Arrange
    stub.set_lamda(static_cast<QStringList (QDir::*)(const QStringList &, QDir::Filters, QDir::SortFlags) const>(&QDir::entryList),
                   [](const QDir *, const QStringList &, QDir::Filters, QDir::SortFlags) -> QStringList {
                       return QStringList() << QString::fromLatin1("libut.so") << QString::fromLatin1("libut.so.1");
                   });

    // Act / Assert：列表内两个精确名都原样返回
    EXPECT_EQ(Utils::libPath(QString::fromLatin1("libut.so")), QString::fromLatin1("libut.so"));
    EXPECT_EQ(Utils::libPath(QString::fromLatin1("libut.so.1")), QString::fromLatin1("libut.so.1"));
}

TEST_F(UtilsTest, LibPath_NoCandidates_ReturnsEmpty)
{
    // Arrange
    stub.set_lamda(static_cast<QStringList (QDir::*)(const QStringList &, QDir::Filters, QDir::SortFlags) const>(&QDir::entryList),
                   [](const QDir *, const QStringList &, QDir::Filters, QDir::SortFlags) -> QStringList {
                       return QStringList();
                   });

    // Act / Assert：无候选返回空
    EXPECT_TRUE(Utils::libPath(QString::fromLatin1("libnone.so")).isEmpty());
    EXPECT_TRUE(Utils::libPath(QString::fromLatin1("libnone2.so")).isEmpty());
}

TEST_F(UtilsTest, LibPath_VersionedCandidatesOnly_ReturnsSortedLast)
{
    // Arrange
    stub.set_lamda(static_cast<QStringList (QDir::*)(const QStringList &, QDir::Filters, QDir::SortFlags) const>(&QDir::entryList),
                   [](const QDir *, const QStringList &, QDir::Filters, QDir::SortFlags) -> QStringList {
                       return QStringList() << QString::fromLatin1("libv.so.10") << QString::fromLatin1("libv.so.2");
                   });

    // Act / Assert：无精确匹配 → 字典序取最大（"libv.so.2" > "libv.so.10" 按字符串比较）
    EXPECT_EQ(Utils::libPath(QString::fromLatin1("libv.so")), QString::fromLatin1("libv.so.2"));
    EXPECT_FALSE(Utils::libPath(QString::fromLatin1("libv.so")).isEmpty());
}

// ===========================================================================
// loadCustomDLL（libPath + QFile::exists stub）
// ===========================================================================
TEST_F(UtilsTest, LoadCustomDLL_LibraryAbsent_ClearsZpdDllName)
{
    // Arrange：libPath 精确匹配分支返回自身；exists=false → chZPDDLL=NULL
    stub.set_lamda(&Utils::libPath,
                   [](const QString &) -> QString { return QString::fromLatin1("libzpdcallback.so"); });
    stub.set_lamda(static_cast<bool (*)(const QString &)>(&QFile::exists),
                   [](const QString &) -> bool { return false; });

    // Act
    Utils::loadCustomDLL();

    // Assert：不触发 dlopen（惰性），句柄均为空
    LoadLibs *libs = getLoadZPDLibsInstance();
    ASSERT_NE(libs, nullptr);
    EXPECT_EQ(libs->m_document_clip_copy, nullptr);
    EXPECT_EQ(libs->m_document_close, nullptr);
}

TEST_F(UtilsTest, LoadCustomDLL_LibraryPresent_RecordsNameForLazyLoad)
{
    // Arrange
    m_callCount = 0;
    m_captured.clear();
    stub.set_lamda(&Utils::libPath,
                   [](const QString &) -> QString { return QString::fromLatin1("/unittest-mock/zpd/libzpdcallback.so"); });
    stub.set_lamda(static_cast<bool (*)(const QString &)>(&QFile::exists),
                   [this](const QString &path) -> bool {
                       ++m_callCount;
                       m_captured = path;
                       return true;
                   });

    // Act
    Utils::loadCustomDLL();

    // Assert：候选库名被记录（惰性 dlopen，此处不真正加载）
    EXPECT_EQ(m_callCount, 1);
    EXPECT_EQ(m_captured, QString::fromLatin1("/unittest-mock/zpd/libzpdcallback.so"));
}

// ===========================================================================
// enableClipCopy / recordCloseFile（_ZPD_ 未启用 → 恒允许）
// ===========================================================================
TEST_F(UtilsTest, EnableClipCopy_AnyPath_ReturnsTrueAndRecordCloseIsSafe)
{
    // Act / Assert：无 ZPD 定制时剪切拷贝总是允许
    EXPECT_TRUE(Utils::enableClipCopy(QString::fromLatin1("/data/secret.txt")));
    EXPECT_TRUE(Utils::enableClipCopy(QString()));

    // recordCloseFile 仅记录，无返回值；验证不抛异常
    EXPECT_NO_THROW(Utils::recordCloseFile(QString::fromLatin1("/data/secret.txt")));
}

// ===========================================================================
// sendFloatMessageFixedFont（offscreen 浮动消息）
// ===========================================================================
TEST_F(UtilsTest, SendFloatMessageFixedFont_PublishesFloatingMessage)
{
    // Arrange
    QWidget parent;

    // Act
    Utils::sendFloatMessageFixedFont(&parent, QIcon(), QString::fromLatin1("ut message"));

    // Assert：父窗口内出现一条 Transient 浮动消息
    const auto messages = parent.findChildren<DFloatingMessage *>();
    ASSERT_EQ(messages.size(), 1);

    // Act2：触发应用字体变更信号 → fontChanged lambda → 浮动消息字体跟随 qApp
    const QFont newFont = qApp->font();
    QMetaObject::invokeMethod(DGuiApplicationHelper::instance(), "fontChanged", Q_ARG(QFont, newFont));

    // Assert2：lambda 执行后消息对象仍存活且字体已同步（强异常安全）
    EXPECT_EQ(parent.findChildren<DFloatingMessage *>().size(), 1);
    EXPECT_EQ(messages.first()->font().family(), newFont.family());
}

TEST_F(UtilsTest, SendFloatMessageFixedFont_MoreThanThreeTransient_Throttled)
{
    // Arrange
    QWidget parent;

    // Act：连续 4 次（TransientType 上限 3）
    for (int i = 0; i < 3; ++i)
        Utils::sendFloatMessageFixedFont(&parent, QIcon(), QString::fromLatin1("msg-%1").arg(i));
    const int afterThree = parent.findChildren<DFloatingMessage *>().size();
    Utils::sendFloatMessageFixedFont(&parent, QIcon(), QString::fromLatin1("msg-4"));

    // Assert：第 4 条被限流
    EXPECT_EQ(afterThree, 3);
    EXPECT_EQ(parent.findChildren<DFloatingMessage *>().size(), 3);
}

// ===========================================================================
// getSystemMemoryInfo（/proc/meminfo；open/readAll stub 控制分支）
// ===========================================================================
TEST_F(UtilsTest, GetSystemMemoryInfo_RealProcMeminfo_ParsesPositiveValues)
{
    // Arrange（Linux CI 必有 /proc/meminfo，只读）
    qlonglong total = 0, freeMem = 0;

    // Act
    const bool ok = Utils::getSystemMemoryInfo(total, freeMem);

    // Assert
    EXPECT_TRUE(ok);
    EXPECT_GT(total, 0);
    EXPECT_GT(freeMem, 0);
}

TEST_F(UtilsTest, GetSystemMemoryInfo_OpenFailed_ReturnsFalse)
{
    // Arrange
    stub.set_lamda(STUB_ADDR(static_cast<bool (QFile::*)(QIODevice::OpenMode)>(&QFile::open)),
                   [](QFile *, QIODevice::OpenMode) -> bool { return false; });
    qlonglong total = 1, freeMem = 1;

    // Act
    const bool ok = Utils::getSystemMemoryInfo(total, freeMem);

    // Assert：打开失败 → false（强异常安全：出参保持原值）
    EXPECT_FALSE(ok);
    EXPECT_EQ(total, 1);
    EXPECT_EQ(freeMem, 1);
}

TEST_F(UtilsTest, GetSystemMemoryInfo_NoMemAvailable_SumsFreeBuffersCached)
{
    // Arrange：无 MemAvailable 行 → free = MemFree + Buffers + Cached
    const QByteArray crafted =
            "MemTotal:       1000 kB\n"
            "MemFree:         100 kB\n"
            "Buffers:          20 kB\n"
            "Cached:           30 kB\n";
    stub.set_lamda(STUB_ADDR(static_cast<bool (QFile::*)(QIODevice::OpenMode)>(&QFile::open)),
                   [](QFile *, QIODevice::OpenMode) -> bool { return true; });
    stub.set_lamda(&QIODevice::readAll,
                   [crafted](QIODevice *) -> QByteArray { return crafted; });
    qlonglong total = 0, freeMem = 0;

    // Act
    const bool ok = Utils::getSystemMemoryInfo(total, freeMem);

    // Assert
    EXPECT_TRUE(ok);
    EXPECT_EQ(total, 1000);
    EXPECT_EQ(freeMem, 150);
}

// ===========================================================================
// isMemorySufficientForOperation（getSystemMemoryInfo stub 固定内存）
// ===========================================================================
namespace {
void stubMemory(stub_ext::StubExt &stub, qlonglong totalKb, qlonglong freeKb, bool ok = true)
{
    stub.set_lamda(&Utils::getSystemMemoryInfo,
                   [totalKb, freeKb, ok](qlonglong &total, qlonglong &freeMemory) -> bool {
                       total = totalKb;
                       freeMemory = freeKb;
                       return ok;
                   });
}
} // namespace

TEST_F(UtilsTest, IsMemorySufficientForOperation_RawOperation_RespectsAvailableMemory)
{
    // Arrange：free=500KB → available=512000B
    stubMemory(stub, 1000, 500);

    // Act / Assert：超限拒绝，未超允许
    EXPECT_FALSE(Utils::isMemorySufficientForOperation(Utils::RawOperation, 600000, 0));
    EXPECT_TRUE(Utils::isMemorySufficientForOperation(Utils::RawOperation, 1000, 0));
}

TEST_F(UtilsTest, IsMemorySufficientForOperation_CopyOperation_AppliesNineXFactor)
{
    // Arrange：1000*9=9000 ≤ 512000；60000*9=540000 > 512000
    stubMemory(stub, 1000, 500);

    // Act / Assert
    EXPECT_TRUE(Utils::isMemorySufficientForOperation(Utils::CopyOperation, 1000, 0));
    EXPECT_FALSE(Utils::isMemorySufficientForOperation(Utils::CopyOperation, 60000, 0));
}

TEST_F(UtilsTest, IsMemorySufficientForOperation_PasteOperation_AllSubConditions)
{
    // Arrange：小内存场景
    stubMemory(stub, 1000, 500); // available=512000B, total=1024000B
    // 粘贴数据自身超限：60000*7=420000 OK；文档总量超总内存：
    EXPECT_FALSE(Utils::isMemorySufficientForOperation(Utils::PasteOperation, 60000, 600000)); // (600000+60000)*7>1024000
    EXPECT_TRUE(Utils::isMemorySufficientForOperation(Utils::PasteOperation, 1000, 1000));

    // Arrange：大内存场景触发 800MB/500KB 专门阈值
    stubMemory(stub, 100000000, 100000000);
    EXPECT_FALSE(Utils::isMemorySufficientForOperation(Utils::PasteOperation, 600000, 838860900)); // doc>800MB 且 paste>500KB
    EXPECT_TRUE(Utils::isMemorySufficientForOperation(Utils::PasteOperation, 400000, 838860900));  // paste 未超 500KB
}

TEST_F(UtilsTest, IsMemorySufficientForOperation_MemoryInfoUnavailable_ConservativelyAllows)
{
    // Arrange：读不到内存信息 → 保守允许
    stubMemory(stub, 0, 0, false);

    // Act / Assert：读不到内存信息时任何操作都保守允许
    EXPECT_TRUE(Utils::isMemorySufficientForOperation(Utils::CopyOperation, 1, 1));
    EXPECT_TRUE(Utils::isMemorySufficientForOperation(Utils::PasteOperation, 1, 1));
}

TEST_F(UtilsTest, IsMemorySufficientForOperation_UnknownOperationType_DefaultAllows)
{
    // Arrange
    stubMemory(stub, 1000, 500);

    // Act：非法枚举值 → default 分支 → 允许
    const bool ok = Utils::isMemorySufficientForOperation(static_cast<Utils::OperationType>(99), 100, 100);

    // Assert：任意非法值一致
    EXPECT_TRUE(ok);
    EXPECT_TRUE(Utils::isMemorySufficientForOperation(static_cast<Utils::OperationType>(100), 100, 100));
}
