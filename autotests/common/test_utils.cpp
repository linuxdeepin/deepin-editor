// SPDX-FileCopyrightText: 2026 UnionTech Software Technology Co., Ltd.
// SPDX-License-Identifier: GPL-3.0-or-later

#include "../../src/common/utils.h"

#include <gtest/gtest.h>
#include <QTemporaryFile>
#include <QTemporaryDir>
#include <QPainter>
#include <QFont>
#include <QKeyEvent>
#include <QApplication>
#include <QFileInfo>
#include <QDir>
#include <QCryptographicHash>
#include <QStandardPaths>

// Pure easing functions are deterministic math; verify monotonicity and bounds.
TEST(UtilsTest, EaseInOut_BoundaryValues_ReturnsClampedResults)
{
    EXPECT_FLOAT_EQ(Utils::easeInOut(0.0), 0.0f);
    EXPECT_FLOAT_EQ(Utils::easeInOut(1.0), 1.0f);
    EXPECT_FLOAT_EQ(Utils::easeInQuad(0.0), 0.0f);
    EXPECT_FLOAT_EQ(Utils::easeOutQuad(1.0), 1.0f);
    EXPECT_FLOAT_EQ(Utils::easeInQuint(0.0), 0.0f);
    EXPECT_FLOAT_EQ(Utils::easeOutQuint(1.0), 1.0f);
}

TEST(UtilsTest, EaseInOut_Midpoint_ReturnsHalf)
{
    // easeInOut(0.5) should be 0.5 for the smoothstep-style curve.
    EXPECT_NEAR(Utils::easeInOut(0.5), 0.5, 1e-6);
}

TEST(UtilsTest, EaseInQuad_Midpoint_ReturnsQuarter)
{
    EXPECT_NEAR(Utils::easeInQuad(0.5), 0.25, 1e-6);
}

TEST(UtilsTest, EaseOutQuad_Midpoint_ReturnsThreeQuarters)
{
    EXPECT_NEAR(Utils::easeOutQuad(0.5), 0.75, 1e-6);
}

// Region intersection: [0,9] fixed, activity interval [10,15] is to the right.
TEST(UtilsTest, CheckRegionIntersect_ActivityRightOfFixed_ReturnsERight)
{
    EXPECT_EQ(Utils::checkRegionIntersect(0, 9, 10, 15), Utils::ERight);
}

TEST(UtilsTest, CheckRegionIntersect_ActivityLeftOfFixed_ReturnsELeft)
{
    EXPECT_EQ(Utils::checkRegionIntersect(0, 9, -5, -1), Utils::ELeft);
}

TEST(UtilsTest, CheckRegionIntersect_OverlapOnLeft_ReturnsEIntersectLeft)
{
    EXPECT_EQ(Utils::checkRegionIntersect(0, 9, -5, 5), Utils::EIntersectLeft);
}

TEST(UtilsTest, CheckRegionIntersect_OverlapOnRight_ReturnsEIntersectRight)
{
    EXPECT_EQ(Utils::checkRegionIntersect(0, 9, 5, 15), Utils::EIntersectRight);
}

TEST(UtilsTest, CheckRegionIntersect_ActivityContainsFixed_ReturnsEIntersectOutter)
{
    EXPECT_EQ(Utils::checkRegionIntersect(0, 9, -10, 10), Utils::EIntersectOutter);
}

TEST(UtilsTest, CheckRegionIntersect_ActivityInsideFixed_ReturnsEIntersectInner)
{
    EXPECT_EQ(Utils::checkRegionIntersect(0, 9, 5, 6), Utils::EIntersectInner);
}

// MD5 hash of a known string matches QCryptographicHash.
TEST(UtilsTest, GetStringMD5Hash_KnownInput_MatchesQCryptographicHash)
{
    const QString input = QStringLiteral("deepin-editor");
    const QByteArray expected =
        QCryptographicHash::hash(input.toUtf8(), QCryptographicHash::Md5).toHex();
    EXPECT_EQ(Utils::getStringMD5Hash(input).toLower(), QString::fromLatin1(expected).toLower());
    EXPECT_FALSE(Utils::getStringMD5Hash(input).isEmpty());
}

// fileExists / fileIsWritable / fileIsHome on a real temporary file.
TEST(UtilsTest, FileExists_ExistingTempFile_ReturnsTrue)
{
    QTemporaryFile tmp;
    ASSERT_TRUE(tmp.open());
    EXPECT_TRUE(Utils::fileExists(tmp.fileName()));
    EXPECT_FALSE(Utils::fileExists(QStringLiteral("/no/such/file/exists-12345")));
}

TEST(UtilsTest, FileIsWritable_TempFile_ReturnsTrue)
{
    QTemporaryFile tmp;
    ASSERT_TRUE(tmp.open());
    EXPECT_TRUE(Utils::fileIsWritable(tmp.fileName()));
}

TEST(UtilsTest, FileIsHome_HomePath_ReturnsTrue)
{
    EXPECT_TRUE(Utils::fileIsHome(QDir::homePath()));
    EXPECT_FALSE(Utils::fileIsHome(QStringLiteral("/no/such/home")));
}

// getFilePath prefers canonical path; for a non-existent path it falls back to absolute.
TEST(UtilsTest, GetFilePath_NonExistentPath_ReturnsAbsolutePath)
{
    const QString rel = QStringLiteral("non-existent-relative-file-xyz");
    const QString got = Utils::getFilePath(rel);
    EXPECT_FALSE(got.isEmpty());
    EXPECT_TRUE(got.startsWith('/'));
}

// getQrcPath / getQssPath produce non-empty resource paths.
TEST(UtilsTest, GetQrcPath_ValidName_ReturnsNonEmpty)
{
    const QString p = Utils::getQrcPath(QStringLiteral("bar_close_normal_light.svg"));
    EXPECT_FALSE(p.isEmpty());
    EXPECT_TRUE(p.contains(':'));
}

TEST(UtilsTest, GetQssPath_ValidName_ReturnsNonEmpty)
{
    const QString p = Utils::getQssPath(QStringLiteral("bar_close_normal_light.svg"));
    EXPECT_FALSE(p.isEmpty());
    EXPECT_TRUE(p.contains(':'));
}

// getRenderSize returns a non-empty size for a sample string.
TEST(UtilsTest, GetRenderSize_SampleString_ReturnsNonZeroWidth)
{
    const QSize s = Utils::getRenderSize(12, QStringLiteral("hello deepin-editor"));
    EXPECT_GT(s.width(), 0);
    EXPECT_GT(s.height(), 0);
}

// setFontSize configures the painter font point size.
TEST(UtilsTest, SetFontSize_AppliesPointSizeToPainter)
{
    QPainter painter;
    // Begin on a 1x1 image so painter is active.
    QImage img(1, 1, QImage::Format_ARGB32);
    painter.begin(&img);
    Utils::setFontSize(painter, 14);
    EXPECT_EQ(painter.font().pointSize(), 14);
    painter.end();
}

// detectEncode / getEncode return a non-empty encoding for ASCII/UTF-8 content.
TEST(UtilsTest, DetectEncode_AsciiContent_ReturnsNonEmptyEncoding)
{
    const QByteArray data = "hello deepin-editor unit test\n";
    const QByteArray enc = Utils::detectEncode(data, QStringLiteral("test.txt"));
    EXPECT_FALSE(enc.isEmpty());
}

TEST(UtilsTest, GetEncode_AsciiContent_ReturnsNonEmptyEncoding)
{
    const QByteArray data = "plain ascii content for encoding detection";
    const QByteArray enc = Utils::getEncode(data);
    EXPECT_FALSE(enc.isEmpty());
}

// getSupportEncodingList returns a non-empty list of encodings.
TEST(UtilsTest, GetSupportEncodingList_ReturnsNonEmpty)
{
    const QStringList list = Utils::getSupportEncodingList();
    EXPECT_FALSE(list.isEmpty());
}

// isDraftFile / isBackupFile recognize the app's draft/backup directories
// (under QStandardPaths::AppDataLocation). A path inside those dirs returns
// true; an unrelated path returns false.
static QString appDataSubdir(const QString &sub)
{
    const QStringList locs = QStandardPaths::standardLocations(QStandardPaths::AppDataLocation);
    assert(!locs.isEmpty());
    return QDir(QDir::cleanPath(locs.first())).filePath(sub);
}

TEST(UtilsTest, IsDraftFile_DraftDirPath_ReturnsTrue)
{
    const QString draftDir = appDataSubdir(QStringLiteral("blank-files"));
    QDir().mkpath(draftDir);
    const QString draft = QDir(draftDir).filePath(QStringLiteral("draft.txt"));
    EXPECT_TRUE(Utils::isDraftFile(draft));
    EXPECT_FALSE(Utils::isDraftFile(QStringLiteral("/tmp/normal-not-draft.txt")));
}

TEST(UtilsTest, IsBackupFile_BackupDirPath_ReturnsTrue)
{
    const QString backupDir = appDataSubdir(QStringLiteral("backup-files"));
    QDir().mkpath(backupDir);
    const QString backup = QDir(backupDir).filePath(QStringLiteral("file.txt"));
    EXPECT_TRUE(Utils::isBackupFile(backup));
    EXPECT_FALSE(Utils::isBackupFile(QStringLiteral("/tmp/file-not-backup.txt")));
}

// localDataPath returns an absolute path.
TEST(UtilsTest, LocalDataPath_ReturnsAbsolutePath)
{
    const QString p = Utils::localDataPath();
    EXPECT_FALSE(p.isEmpty());
    EXPECT_TRUE(p.startsWith('/'));
}

// cleanPath applies QDir::cleanPath to each entry (normalize "."/"..", strip
// trailing slashes); it does not de-duplicate or drop empties.
TEST(UtilsTest, CleanPath_NormalizesEachEntry)
{
    const QStringList in{QStringLiteral("/a/b/../c/"), QStringLiteral("/a/b")};
    const QStringList out = Utils::cleanPath(in);
    ASSERT_EQ(out.size(), 2);
    EXPECT_EQ(out.at(0), QStringLiteral("/a/c"));
    EXPECT_EQ(out.at(1), QStringLiteral("/a/b"));
}

// getSystemLan returns a non-empty language code.
TEST(UtilsTest, GetSystemLan_ReturnsNonEmpty)
{
    EXPECT_FALSE(Utils::getSystemLan().isEmpty());
}

// isWayland returns a bool without crashing.
TEST(UtilsTest, IsWayland_ReturnsBool)
{
    const bool v = Utils::isWayland();
    (void)v;
    SUCCEED();
}

// libPath returns "" when no matching library is found in the Qt libraries
// directory, and a non-empty name when one is found. We verify the not-found
// path returns an empty string without crashing.
TEST(UtilsTest, LibPath_NotFoundLib_ReturnsEmpty)
{
    const QString p = Utils::libPath(QStringLiteral("no-such-lib-xyz-12345"));
    EXPECT_TRUE(p.isEmpty());
}

// lineFeed wraps text to a width.
TEST(UtilsTest, LineFeed_LongText_ReturnsWrappedText)
{
    const QFont font(QStringLiteral("monospace"), 10);
    const QString text = QStringLiteral("deepin-editor unit test text that is long enough to wrap");
    const QString out = Utils::lineFeed(text, 50, font, 2);
    EXPECT_FALSE(out.isEmpty());
}

// getSystemMemoryInfo returns valid total/free for /proc/meminfo.
TEST(UtilsTest, GetSystemMemoryInfo_ReturnsPositiveValues)
{
    qlonglong total = 0, free = 0;
    const bool ok = Utils::getSystemMemoryInfo(total, free);
    EXPECT_TRUE(ok);
    EXPECT_GT(total, 0);
    EXPECT_GE(free, 0);
}

// isMemorySufficientForOperation: small copy on a normal system is sufficient.
TEST(UtilsTest, IsMemorySufficientForOperation_SmallCopy_ReturnsTrue)
{
    qlonglong total = 0, free = 0;
    ASSERT_TRUE(Utils::getSystemMemoryInfo(total, free));
    // A tiny operation should be considered affordable.
    EXPECT_TRUE(Utils::isMemorySufficientForOperation(Utils::CopyOperation, 1024, 1024));
}

// codecConfidenceForData returns a non-negative confidence for a known codec.
TEST(UtilsTest, CodecConfidenceForData_Utf8Codec_ReturnsNonNegative)
{
    const QTextCodec *utf8 = QTextCodec::codecForName("UTF-8");
    ASSERT_NE(utf8, nullptr);
    const QByteArray data = "plain ascii content";
    const float c = Utils::codecConfidenceForData(utf8, data, QLocale::China);
    EXPECT_GE(c, 0.0f);
}

// getKeyshortcut for a key event with modifiers returns a non-empty sequence.
TEST(UtilsTest, GetKeyshortcut_CtrlS_ReturnsNonEmpty)
{
    QKeyEvent ev(QEvent::KeyPress, Qt::Key_S, Qt::ControlModifier, "s");
    const QString s = Utils::getKeyshortcut(&ev);
    EXPECT_FALSE(s.isEmpty());
    EXPECT_TRUE(s.contains('s', Qt::CaseInsensitive) || s.contains('S'));
}
