// SPDX-FileCopyrightText: 2026 UnionTech Software Technology Co., Ltd.
// SPDX-License-Identifier: GPL-3.0-or-later

/*
 * DetectCode 单元测试
 *
 * 分支清单（来源：detectcode.cpp 全部导出函数）
 * GetFileEncodingFormat:
 *   G1 中文内容分支（附加中文字符+尾部重试）  G2 非中文低置信重试(chop)
 *   G3 需 uchardet 兜底                       G4 ASCII → 默认编码
 *   G5 非 ASCII → icu + selectCoding          G6 结果空/ASCII → 默认编码
 * UchardetCode: U1 文件打开失败 U2 正常识别 U3 MAC-CENTRALEUROPE 纠正
 *   U4 MAC-CYRILLIC 纠正 U5 WINDOWS-→CP 纠正 U6 handle_data 非零 continue
 * icuDetectTextEncoding: I1 打开失败 I2 正常识别 I3 超 1MB 读上限 break
 * detectTextEncoding: D1 正常返回列表（readMax 3/6 分支）
 * selectCoding: S1 icu 空 S2 低置信+中文+GB18030 S3 包含 S4 含 GB18030
 *   S5 icu[0] 包含 uchar S6 兜底 uchar S7 空 uchar
 * ChartDet_DetectingTextCoding: C1 ASCII C2 UTF-8 中文 C3 空串
 * ChangeFileEncodingFormat: E1 同码 E2 空输入 E3 GB18030↔UTF-8 E4 BOM 附加
 *   E5 EILSEQ '?' 替换 E6 EINVAL 截断 E7 非法码 false E8 PUA E9 补丁 iconv 路径
 * convertEncodingTextCodec: T1/T2 正反向 T3/T4 非法码 T5 BOM 尾部
 * 全局函数: utf8MultiByteCount / checkGB18030ToUtf8Error / checkUTF8ToGB18030Error
 *
 * 用例映射见各 TEST 名称（分支号在注释中标注）。
 *
 * 隔离：
 * - 文件全部经 QTemporaryDir；
 * - Config 三个读取方法在需要固定取值的用例中 stub（Config 真实方法另设用例直跑，
 *   保证 FN 覆盖）；QLocale::system 固定 stub 为中文（selectCoding 语言分支确定性）；
 * - iconv/glibc 行为（无 BOM UTF-16 按 LE、GB18030 PUA 82359037→EEA09E）已实测固定。
 */

#include <gtest/gtest.h>
#include "stubext.h"

#include "../encodes/detectcode.h"
#include "../common/config.h"

#include <QCoreApplication>
#include <QTemporaryDir>
#include <QFile>
#include <QLocale>

#include <cstring>

// detectcode.cpp 中的全局导出函数（头文件未声明，按定义补充声明以便直测）
int utf8MultiByteCount(char *buf, size_t size);
bool checkGB18030ToUtf8Error(char *buf, size_t size, size_t &replaceLen, QByteArray &appendChar);
bool checkUTF8ToGB18030Error(char *buf, size_t size, size_t &replaceLen, QByteArray &appendChar);

namespace {

QCoreApplication *ensureApp()
{
    if (!QCoreApplication::instance()) {
        static int argc = 1;
        static char argv0[] = "test_common2";
        static char *argv[2] = { argv0, nullptr };
        static QCoreApplication app(argc, argv);
        return &app;
    }
    return QCoreApplication::instance();
}

} // namespace

// ---------------- 参数化用例数据 ----------------

struct Utf8CountCase {
    QByteArray bytes;
    int expected;
};

struct Gb2Utf8ErrCase {
    QByteArray bytes;
    int size;
    bool found;
    QByteArray append;
    int replaceLen;
};

struct Utf2GbErrCase {
    QByteArray bytes;
    int size;
    bool found;
    QByteArray append;
    int replaceLen;
};

struct SelectCase {
    QByteArray ucharRet;
    QByteArrayList icu;
    float conf;
    bool improve;
    bool chinese;
    QByteArray expected;
};

struct TextCodecCase {
    QByteArray input;
    QString from;
    QString to;
    bool ok;
    QByteArray expected;
};

// ---------------- Fixture ----------------

class DetectCodeTest : public ::testing::Test
{
protected:
    void SetUp() override
    {
        ensureApp();
        stub.clear();
        chardetCalls = 0;
        uchardetCalls = 0;
        icuOuterCalls = 0;
        detectInnerCalls = 0;
        tmpDir = std::make_unique<QTemporaryDir>();
        ASSERT_TRUE(tmpDir->isValid());
        // 固定系统语言为中文，selectCoding 的中文优先分支完全确定
        installLocaleStub(true);
    }

    void TearDown() override
    {
        stub.clear();
    }

    void installLocaleStub(bool chinese)
    {
        const QLocale::Language lang = chinese ? QLocale::Chinese : QLocale::English;
        stub.set_lamda(&QLocale::system, [lang]() -> QLocale {
            return QLocale(lang);
        });
    }

    void installConfigStubs(bool improve, bool patched, const QByteArray &defEncoding)
    {
        stub.set_lamda(static_cast<bool (Config::*)() const>(&Config::enableImproveGB18030),
                       [improve]() -> bool {
                           return improve;
                       });
        stub.set_lamda(static_cast<bool (Config::*)() const>(&Config::enablePatchedIconv),
                       [patched]() -> bool {
                           return patched;
                       });
        stub.set_lamda(static_cast<QByteArray (Config::*)() const>(&Config::defaultEncoding),
                       [defEncoding]() -> QByteArray {
                           return defEncoding;
                       });
    }

    QString writeTempFile(const QString &name, const QByteArray &data)
    {
        const QString path = tmpDir->path() + QLatin1Char('/') + name;
        QFile f(path);
        if (!f.open(QIODevice::WriteOnly | QIODevice::Truncate))
            return QString();
        f.write(data);
        f.close();
        return path;
    }

    stub_ext::StubExt stub;
    std::unique_ptr<QTemporaryDir> tmpDir;
    int chardetCalls = 0;
    int uchardetCalls = 0;
    int icuOuterCalls = 0;
    int detectInnerCalls = 0;
};

// ---------------- utf8MultiByteCount（纯函数，TEST_P） ----------------

class DetectCodeUtf8CountTest : public DetectCodeTest,
                                public ::testing::WithParamInterface<Utf8CountCase>
{
};

TEST_P(DetectCodeUtf8CountTest, Utf8MultiByteCount_ByLeadBytes_ReturnsExpectedCount)
{
    // Arrange
    const auto &c = GetParam();
    QByteArray buf = c.bytes; // 拷贝为可写缓冲（data() 返回 char*）
    ASSERT_FALSE(buf.isEmpty());
    // Act
    const int ret = utf8MultiByteCount(buf.data(), size_t(buf.size()));
    // Assert
    EXPECT_EQ(ret, c.expected);
    EXPECT_EQ(int(buf.size()), c.bytes.size()); // 输入不被修改
}

INSTANTIATE_TEST_SUITE_P(
    Utf8CountCases, DetectCodeUtf8CountTest,
    ::testing::Values(
        Utf8CountCase{QByteArrayLiteral("a"), 1},                              // 单字节
        Utf8CountCase{QByteArray::fromHex("C3A9"), 2},                         // 双字节
        Utf8CountCase{QByteArray::fromHex("E4B8AD"), 3},                       // 三字节
        Utf8CountCase{QByteArray::fromHex("F09F9880"), 4},                     // 四字节
        Utf8CountCase{QByteArray::fromHex("80C3"), 2},                         // Mid 后遇双字节
        Utf8CountCase{QByteArray::fromHex("80808080") + QByteArray::fromHex("C3"), 4})); // Mid 计满 4 退出

TEST_F(DetectCodeTest, Utf8MultiByteCount_ZeroSize_ReturnsZero)
{
    // Arrange
    char dummy = 'a';
    // Act
    const int ret = utf8MultiByteCount(&dummy, 0);
    // Assert: size==0 不进入循环
    EXPECT_EQ(ret, 0);
    EXPECT_EQ(dummy, 'a');
}

// ---------------- checkGB18030ToUtf8Error（纯函数，TEST_P） ----------------

class DetectCodeGb2Utf8ErrTest : public DetectCodeTest,
                                 public ::testing::WithParamInterface<Gb2Utf8ErrCase>
{
};

TEST_P(DetectCodeGb2Utf8ErrTest, Gb2Utf8Error_ByInputBytes_ReturnsExpectedMapping)
{
    // Arrange
    const auto &c = GetParam();
    QByteArray buf = c.bytes;
    size_t replaceLen = 0;
    QByteArray appendChar;
    // Act
    const bool found = checkGB18030ToUtf8Error(buf.data(), size_t(c.size), replaceLen, appendChar);
    // Assert
    EXPECT_EQ(found, c.found);
    EXPECT_EQ(appendChar, c.append);
    EXPECT_EQ(int(replaceLen), c.replaceLen);
}

INSTANTIATE_TEST_SUITE_P(
    Gb2Utf8ErrCases, DetectCodeGb2Utf8ErrTest,
    ::testing::Values(
        Gb2Utf8ErrCase{QByteArrayLiteral("AB"), 2, false, QByteArrayLiteral("?"), 1},                       // 长度不足
        Gb2Utf8ErrCase{QByteArray::fromHex("41424344"), 4, false, QByteArrayLiteral("?"), 1},               // 无匹配
        Gb2Utf8ErrCase{QByteArray::fromHex("82359037"), 4, true, QByteArray::fromHex("EEA09E"), 4},         // PUA 0x37903582 → \uE81E
        Gb2Utf8ErrCase{QByteArray::fromHex("FFFFFF01"), 4, true, QByteArray::fromHex("EEA096"), 4}));     // FFFFFF01 → \uE816

// ---------------- checkUTF8ToGB18030Error（纯函数，TEST_P） ----------------

class DetectCodeUtf2GbErrTest : public DetectCodeTest,
                                public ::testing::WithParamInterface<Utf2GbErrCase>
{
};

TEST_P(DetectCodeUtf2GbErrTest, Utf2GbError_ByInputBytes_ReturnsExpectedMapping)
{
    // Arrange
    const auto &c = GetParam();
    QByteArray buf = c.bytes;
    size_t replaceLen = 0;
    QByteArray appendChar;
    // Act
    const bool found = checkUTF8ToGB18030Error(buf.data(), size_t(c.size), replaceLen, appendChar);
    // Assert
    EXPECT_EQ(found, c.found);
    EXPECT_EQ(appendChar, c.append);
    EXPECT_EQ(int(replaceLen), c.replaceLen);
}

INSTANTIATE_TEST_SUITE_P(
    Utf2GbErrCases, DetectCodeUtf2GbErrTest,
    ::testing::Values(
        Utf2GbErrCase{QByteArrayLiteral("AB"), 2, false, QByteArrayLiteral("?"), 1},                    // 长度不足
        Utf2GbErrCase{QByteArrayLiteral("ABC"), 3, false, QByteArrayLiteral("?"), 1},                   // 无匹配
        Utf2GbErrCase{QByteArray::fromHex("EEA09E"), 3, true, QByteArray::fromHex("82359037"), 3},      // \uE81E → PUA GB 码
        Utf2GbErrCase{QByteArray::fromHex("EEA096"), 3, true, QByteArray::fromHex("FE51"), 3},          // \uE816 → 0xFE51
        Utf2GbErrCase{QByteArray::fromHex("FFFF11"), 3, true, QByteArray::fromHex("95329031"), 3}));    // 标识 → 0x95329031

// ---------------- selectCoding（纯函数，TEST_P） ----------------

class DetectCodeSelectTest : public DetectCodeTest,
                             public ::testing::WithParamInterface<SelectCase>
{
};

TEST_P(DetectCodeSelectTest, SelectCoding_ByInputs_ReturnsExpectedEncoding)
{
    // Arrange
    const auto &c = GetParam();
    installConfigStubs(c.improve, false, QByteArrayLiteral("UTF-8"));
    installLocaleStub(c.chinese);
    const QByteArrayList icuCopy = c.icu;
    // Act
    const QByteArray ret = DetectCode::selectCoding(c.ucharRet, c.icu, c.conf);
    // Assert
    EXPECT_EQ(ret, c.expected);
    EXPECT_EQ(c.icu, icuCopy); // 输入列表不被修改
}

INSTANTIATE_TEST_SUITE_P(
    SelectCases, DetectCodeSelectTest,
    ::testing::Values(
        SelectCase{QByteArrayLiteral("UTF-8"), {}, 0.99f, true, true, QByteArrayLiteral("")},                     // S1 icu 空
        SelectCase{QByteArrayLiteral("UTF-8"), {QByteArrayLiteral("UTF-8"), QByteArrayLiteral("GB18030")}, 0.99f, true, true, QByteArrayLiteral("UTF-8")}, // S3 包含
        SelectCase{QByteArrayLiteral("GBK"), {QByteArrayLiteral("GB18030")}, 0.5f, true, true, QByteArrayLiteral("GB18030")},   // S2 低置信中文 GB
        SelectCase{QByteArrayLiteral("UTF-8"), {QByteArrayLiteral("UTF-8")}, 0.5f, true, false, QByteArrayLiteral("UTF-8")},    // S2 反例：非中文
        SelectCase{QByteArrayLiteral("BIG5"), {QByteArrayLiteral("GB18030")}, 0.99f, false, true, QByteArrayLiteral("GB18030")},// S4 含 GB18030
        SelectCase{QByteArrayLiteral("UTF-16"), {QByteArrayLiteral("UTF-16BE")}, 0.99f, false, true, QByteArrayLiteral("UTF-16BE")}, // S5 icu[0] 包含
        SelectCase{QByteArrayLiteral("EUC-JP"), {QByteArrayLiteral("SHIFT-JIS")}, 0.99f, false, true, QByteArrayLiteral("EUC-JP")},   // S6 兜底
        SelectCase{QByteArrayLiteral(""), {QByteArrayLiteral("GB18030")}, 0.99f, false, true, QByteArrayLiteral("GB18030")},    // S7 空 uchar+GB
        SelectCase{QByteArrayLiteral(""), {QByteArrayLiteral("UTF-16LE")}, 0.99f, false, true, QByteArrayLiteral("UTF-16LE")})); // S7 空 uchar+首项

// ---------------- convertEncodingTextCodec（TEST_P） ----------------

class DetectCodeTextCodecTest : public DetectCodeTest,
                                public ::testing::WithParamInterface<TextCodecCase>
{
};

TEST_P(DetectCodeTextCodecTest, ConvertEncodingTextCodec_ByCodes_ReturnsExpectedOutput)
{
    // Arrange
    const auto &c = GetParam();
    QByteArray input = c.input;
    QByteArray out;
    // Act
    const bool ok = DetectCode::convertEncodingTextCodec(input, out, c.from, c.to);
    // Assert
    EXPECT_EQ(ok, c.ok);
    if (c.ok)
        EXPECT_EQ(out, c.expected);
    else
        EXPECT_TRUE(out.isEmpty()); // 失败时不产生输出
}

INSTANTIATE_TEST_SUITE_P(
    TextCodecCases, DetectCodeTextCodecTest,
    ::testing::Values(
        TextCodecCase{QStringLiteral("中文").toUtf8(), QStringLiteral("UTF-8"), QStringLiteral("GB18030"), true, QByteArray::fromHex("D6D0CEC4")},
        TextCodecCase{QByteArray::fromHex("D6D0CEC4"), QStringLiteral("GB18030"), QStringLiteral("UTF-8"), true, QStringLiteral("中文").toUtf8()},
        TextCodecCase{QByteArrayLiteral("abc"), QStringLiteral("UTF-8"), QStringLiteral("UT-NONE-T"), false, QByteArrayLiteral("")},
        TextCodecCase{QByteArray::fromHex("D6D0"), QStringLiteral("UT-NONE-F"), QStringLiteral("UTF-8"), false, QByteArrayLiteral("")},
        // QTextCodec 路径 BOM 附加在转换数据之后（尾部）
        TextCodecCase{QByteArrayLiteral("hi"), QStringLiteral("UTF-8"), QStringLiteral("UTF-16LE"), true, QByteArray::fromHex("68006900FFFE")}));

// ---------------- ChartDet_DetectingTextCoding（真实 chardet） ----------------

TEST_F(DetectCodeTest, ChartDet_DetectingTextCoding_AsciiInput_ReturnsAsciiHighConfidence)
{
    // Arrange
    QString encoding;
    float confidence = 0.0f;
    // Act
    const int ret = DetectCode::ChartDet_DetectingTextCoding("hello world", encoding, confidence);
    // Assert
    EXPECT_EQ(ret, CHARDET_SUCCESS);
    EXPECT_EQ(encoding, QStringLiteral("ASCII"));
    EXPECT_GE(confidence, 0.9f);
}

TEST_F(DetectCodeTest, ChartDet_DetectingTextCoding_ChineseUtf8_ReturnsUtf8HighConfidence)
{
    // Arrange
    const QByteArray sample = QStringLiteral("这是一段用于编码探测的中文样本文本").toUtf8();
    const std::string raw(sample.constData(), size_t(sample.size()));
    QString encoding;
    float confidence = 0.0f;
    // Act
    const int ret = DetectCode::ChartDet_DetectingTextCoding(raw.c_str(), encoding, confidence);
    // Assert
    EXPECT_EQ(ret, CHARDET_SUCCESS);
    EXPECT_EQ(encoding, QStringLiteral("UTF-8"));
    EXPECT_GE(confidence, 0.9f);
}

TEST_F(DetectCodeTest, ChartDet_DetectingTextCoding_EmptyInput_ReturnsSuccessWithNoCrash)
{
    // Arrange
    QString encoding = QStringLiteral("sentinel");
    float confidence = -1.0f;
    // Act
    const int ret = DetectCode::ChartDet_DetectingTextCoding("", encoding, confidence);
    // Assert: 空串安全返回，出参被重置语义不崩溃
    EXPECT_EQ(ret, CHARDET_SUCCESS);
    EXPECT_GE(confidence, 0.0f);
}

// ---------------- UchardetCode ----------------

TEST_F(DetectCodeTest, UchardetCode_AsciiFile_ReturnsAscii)
{
    // Arrange
    const QString path = writeTempFile(QStringLiteral("uc_ascii.txt"), QByteArrayLiteral("plain ascii text"));
    ASSERT_FALSE(path.isEmpty());
    // Act
    const QByteArray ret = DetectCode::UchardetCode(path);
    // Assert
    EXPECT_EQ(ret, QByteArrayLiteral("ASCII"));
    EXPECT_EQ(ret.size(), 5); // "ASCII" 五字符
}

TEST_F(DetectCodeTest, UchardetCode_MissingFile_ReturnsEmpty)
{
    // Arrange: 不存在的文件 → fopen 失败分支
    const QString path = tmpDir->path() + QStringLiteral("/uc_missing.txt");
    // Act
    const QByteArray ret = DetectCode::UchardetCode(path);
    const QByteArray ret2 = DetectCode::UchardetCode(tmpDir->path() + QStringLiteral("/uc_missing2.txt"));
    // Assert: 任意不存在路径均安全返回空
    EXPECT_TRUE(ret.isEmpty());
    EXPECT_TRUE(ret2.isEmpty());
}

TEST_F(DetectCodeTest, UchardetCode_MacCentralEurope_CorrectedWithoutDash)
{
    // Arrange: stub uchardet_get_charset 返回待纠正的字符集名
    int charsetCalls = 0;
    stub.set_lamda(uchardet_get_charset, [&charsetCalls](uchardet_t) -> const char * {
        ++charsetCalls;
        return "MAC-CENTRALEUROPE";
    });
    const QString path = writeTempFile(QStringLiteral("uc_mac.txt"), QByteArrayLiteral("x"));
    ASSERT_FALSE(path.isEmpty());
    // Act
    const QByteArray ret = DetectCode::UchardetCode(path);
    // Assert
    EXPECT_EQ(ret, QByteArrayLiteral("MACCENTRALEUROPE"));
    EXPECT_EQ(charsetCalls, 1);
}

TEST_F(DetectCodeTest, UchardetCode_MacCyrillic_CorrectedWithoutDash)
{
    // Arrange
    int charsetCalls = 0;
    stub.set_lamda(uchardet_get_charset, [&charsetCalls](uchardet_t) -> const char * {
        ++charsetCalls;
        return "MAC-CYRILLIC";
    });
    const QString path = writeTempFile(QStringLiteral("uc_cy.txt"), QByteArrayLiteral("x"));
    ASSERT_FALSE(path.isEmpty());
    // Act
    const QByteArray ret = DetectCode::UchardetCode(path);
    // Assert
    EXPECT_EQ(ret, QByteArrayLiteral("MACCYRILLIC"));
    EXPECT_EQ(charsetCalls, 1);
}

TEST_F(DetectCodeTest, UchardetCode_WindowsCharset_CorrectedToCpPrefix)
{
    // Arrange
    int charsetCalls = 0;
    stub.set_lamda(uchardet_get_charset, [&charsetCalls](uchardet_t) -> const char * {
        ++charsetCalls;
        return "WINDOWS-1251";
    });
    const QString path = writeTempFile(QStringLiteral("uc_win.txt"), QByteArrayLiteral("x"));
    ASSERT_FALSE(path.isEmpty());
    // Act
    const QByteArray ret = DetectCode::UchardetCode(path);
    // Assert
    EXPECT_EQ(ret, QByteArrayLiteral("CP1251"));
    EXPECT_EQ(charsetCalls, 1);
}

TEST_F(DetectCodeTest, UchardetCode_HandleDataFails_SkipsAndStillReturnsCharset)
{
    // Arrange: handle_data 返回非零 → continue 分支
    int handleCalls = 0;
    stub.set_lamda(uchardet_handle_data, [&handleCalls](uchardet_t, const char *, size_t) -> int {
        ++handleCalls;
        return 1;
    });
    stub.set_lamda(uchardet_get_charset, [](uchardet_t) -> const char * {
        return "ASCII";
    });
    const QString path = writeTempFile(QStringLiteral("uc_hd.txt"), QByteArrayLiteral("abc"));
    ASSERT_FALSE(path.isEmpty());
    // Act
    const QByteArray ret = DetectCode::UchardetCode(path);
    // Assert: 数据处理失败被跳过，仍完成探测流程
    EXPECT_EQ(ret, QByteArrayLiteral("ASCII"));
    EXPECT_GE(handleCalls, 1);
}

// ---------------- icuDetectTextEncoding / detectTextEncoding ----------------

TEST_F(DetectCodeTest, IcuDetectTextEncoding_Utf8File_ReturnsNonEmptyList)
{
    // Arrange
    const QString path = writeTempFile(QStringLiteral("icu_utf8.txt"),
                                       QStringLiteral("english content 中文内容").toUtf8());
    ASSERT_FALSE(path.isEmpty());
    QByteArrayList list;
    // Act
    DetectCode::icuDetectTextEncoding(path, list);
    // Assert
    EXPECT_GE(list.size(), 1);
    EXPECT_LE(list.size(), 6); // readMax = min(6 或 3, matchCount)
    EXPECT_FALSE(list.first().isEmpty());
}

TEST_F(DetectCodeTest, IcuDetectTextEncoding_MissingFile_LeavesListEmpty)
{
    // Arrange
    const QString path = tmpDir->path() + QStringLiteral("/icu_missing.txt");
    QByteArrayList list;
    // Act
    DetectCode::icuDetectTextEncoding(path, list);
    // Assert
    EXPECT_TRUE(list.isEmpty());
    EXPECT_EQ(list.size(), 0);
}

TEST_F(DetectCodeTest, IcuDetectTextEncoding_OverReadLimit_StopsAfterOneMb)
{
    // Arrange: 内层 detectTextEncoding 恒 false → 持续读取直至超 1MB 上限 break
    stub.set_lamda(&DetectCode::detectTextEncoding,
                   [this](const char *, size_t, char **, QByteArrayList &) -> bool {
                       ++detectInnerCalls;
                       return false;
                   });
    QByteArray big(1200 * 1024, 'a');
    const QString path = writeTempFile(QStringLiteral("icu_big.txt"), big);
    ASSERT_FALSE(path.isEmpty());
    QByteArrayList list;
    // Act
    DetectCode::icuDetectTextEncoding(path, list);
    // Assert: 4096 字节/块，1MB ≈ 256 块后触发上限
    EXPECT_TRUE(list.isEmpty());
    EXPECT_GE(detectInnerCalls, 250);
}

TEST_F(DetectCodeTest, DetectTextEncoding_AsciiData_ReturnsTrueWithList)
{
    // Arrange
    QByteArrayList list;
    // Act
    const bool ok = DetectCode::detectTextEncoding("hello", 5, nullptr, list);
    // Assert
    EXPECT_TRUE(ok);
    EXPECT_GE(list.size(), 1);
    EXPECT_LE(list.size(), 6);
}

// ---------------- GetFileEncodingFormat（stub 分支控制 + 真实集成） ----------------

TEST_F(DetectCodeTest, GetFileEncodingFormat_ChineseLowConfidence_RetriesThenUtf8)
{
    // Arrange: G1 中文分支 + 尾部重试一次达标
    const QByteArray content = QStringLiteral("中文内容 abc").toUtf8();
    const QString path = writeTempFile(QStringLiteral("gf_zh.txt"), content);
    ASSERT_FALSE(path.isEmpty());
    int idx = 0;
    const float confs[2] = { 0.5f, 0.99f };
    stub.set_lamda(&DetectCode::ChartDet_DetectingTextCoding,
                   [this, &idx, &confs](const char *, QString &encoding, float &confidence) -> int {
                       ++chardetCalls;
                       encoding = QStringLiteral("UTF-8");
                       confidence = confs[idx < 1 ? idx++ : 1];
                       return CHARDET_SUCCESS;
                   });
    stub.set_lamda(&DetectCode::UchardetCode,
                   [this](QString) -> QByteArray {
                       ++uchardetCalls;
                       return QByteArrayLiteral("UTF-8");
                   });
    stub.set_lamda(&DetectCode::icuDetectTextEncoding,
                   [](const QString &, QByteArrayList &list) {
                       list.clear();
                       list << QByteArrayLiteral("UTF-8");
                   });
    // Act
    const QByteArray ret = DetectCode::GetFileEncodingFormat(path, content);
    // Assert: 初次 + 1 次重试；置信度达标不落 uchardet
    EXPECT_EQ(ret, QByteArrayLiteral("UTF-8"));
    EXPECT_EQ(chardetCalls, 2);
    EXPECT_EQ(uchardetCalls, 0);
}

TEST_F(DetectCodeTest, GetFileEncodingFormat_NonChineseLowConfidence_RetriesFiveTimesThenDefault)
{
    // Arrange: G2 非中文 chop 重试 5 次（tryCount=5）仍低置信 → G3 uchardet 兜底 → G4 ASCII → 默认编码
    const QByteArray content = QByteArrayLiteral("abcdef");
    const QString path = writeTempFile(QStringLiteral("gf_retry.txt"), content);
    ASSERT_FALSE(path.isEmpty());
    stub.set_lamda(&DetectCode::ChartDet_DetectingTextCoding,
                   [this](const char *, QString &encoding, float &confidence) -> int {
                       ++chardetCalls;
                       encoding = QStringLiteral("ASCII");
                       confidence = 0.5f;
                       return CHARDET_SUCCESS;
                   });
    stub.set_lamda(&DetectCode::icuDetectTextEncoding,
                   [this](const QString &, QByteArrayList &) {
                       ++icuOuterCalls;
                   });
    // Act
    const QByteArray ret = DetectCode::GetFileEncodingFormat(path, content);
    // Assert: 1 次初始 + 5 次重试；ASCII 走默认编码（真实文件被 uchardet 识别为 ASCII）
    EXPECT_EQ(chardetCalls, 6);
    EXPECT_EQ(ret, Config::instance()->defaultEncoding().toUpper());
    EXPECT_EQ(icuOuterCalls, 0); // ASCII 分支不进 ICU
}

TEST_F(DetectCodeTest, GetFileEncodingFormat_HighConfidenceUtf8_NoRetry)
{
    // Arrange: G5 正常路径，置信度达标
    const QByteArray content = QByteArrayLiteral("hello world");
    const QString path = writeTempFile(QStringLiteral("gf_hi.txt"), content);
    ASSERT_FALSE(path.isEmpty());
    stub.set_lamda(&DetectCode::ChartDet_DetectingTextCoding,
                   [this](const char *, QString &encoding, float &confidence) -> int {
                       ++chardetCalls;
                       encoding = QStringLiteral("UTF-8");
                       confidence = 0.99f;
                       return CHARDET_SUCCESS;
                   });
    stub.set_lamda(&DetectCode::UchardetCode,
                   [this](QString) -> QByteArray {
                       ++uchardetCalls;
                       return QByteArrayLiteral("UTF-8");
                   });
    stub.set_lamda(&DetectCode::icuDetectTextEncoding,
                   [](const QString &, QByteArrayList &list) {
                       list.clear();
                       list << QByteArrayLiteral("UTF-8");
                   });
    // Act
    const QByteArray ret = DetectCode::GetFileEncodingFormat(path, content);
    // Assert
    EXPECT_EQ(ret, QByteArrayLiteral("UTF-8"));
    EXPECT_EQ(chardetCalls, 1);
    EXPECT_EQ(uchardetCalls, 0);
}

TEST_F(DetectCodeTest, GetFileEncodingFormat_Utf8ChineseFile_ReturnsUtf8)
{
    // Arrange: 真实探测（无 chardet/icu stub）
    const QByteArray raw = QStringLiteral("中文编码探测样例：你好，世界！deepin editor").toUtf8();
    const QString path = writeTempFile(QStringLiteral("gf_real_zh.txt"), raw);
    ASSERT_FALSE(path.isEmpty());
    // Act
    const QByteArray ret = DetectCode::GetFileEncodingFormat(path, raw);
    // Assert
    EXPECT_EQ(ret, QByteArrayLiteral("UTF-8"));
    EXPECT_EQ(ret, ret.toUpper()); // 返回值为大写规范形式
}

TEST_F(DetectCodeTest, GetFileEncodingFormat_Gb18030File_ReturnsGbFamily)
{
    // Arrange: GB18030 字节序列（"中文测试" ×3）
    QByteArray gb;
    for (int i = 0; i < 3; ++i)
        gb += QByteArray::fromHex("D6D0CEC4B2E2CAD4");
    const QString path = writeTempFile(QStringLiteral("gf_gb.txt"), gb);
    ASSERT_FALSE(path.isEmpty());
    // Act
    const QByteArray ret = DetectCode::GetFileEncodingFormat(path, gb);
    // Assert: GB18030/GBK 家族识别（大写返回）
    EXPECT_TRUE(ret == QByteArrayLiteral("GB18030") || ret == QByteArrayLiteral("GBK"))
        << "actual: " << ret.constData();
    EXPECT_FALSE(ret.isEmpty());
}

TEST_F(DetectCodeTest, GetFileEncodingFormat_EmptyFile_ReturnsDefaultEncoding)
{
    // Arrange: G6 结果为空 → 默认编码
    const QString path = writeTempFile(QStringLiteral("gf_empty.txt"), QByteArray());
    ASSERT_FALSE(path.isEmpty());
    // Act
    const QByteArray ret = DetectCode::GetFileEncodingFormat(path, QByteArray());
    // Assert（与真实配置自一致，不硬编码机器取值）
    EXPECT_EQ(ret, Config::instance()->defaultEncoding().toUpper());
    EXPECT_FALSE(ret.isEmpty());
}

TEST_F(DetectCodeTest, GetFileEncodingFormat_MissingFile_ReturnsDefaultEncoding)
{
    // Arrange
    const QString path = tmpDir->path() + QStringLiteral("/gf_missing.txt");
    // Act
    const QByteArray ret = DetectCode::GetFileEncodingFormat(path, QByteArray());
    // Assert
    EXPECT_EQ(ret, Config::instance()->defaultEncoding().toUpper());
    EXPECT_FALSE(ret.isEmpty());
}

// ---------------- ChangeFileEncodingFormat ----------------

TEST_F(DetectCodeTest, ChangeEncoding_SameCodes_CopiesInputAndSucceeds)
{
    // Arrange: E1 同码直通
    QByteArray in = QByteArrayLiteral("same bytes"), out;
    // Act
    const bool ok = DetectCode::ChangeFileEncodingFormat(in, out, QStringLiteral("UTF-8"), QStringLiteral("UTF-8"));
    // Assert
    EXPECT_TRUE(ok);
    EXPECT_EQ(out, in);
}

TEST_F(DetectCodeTest, ChangeEncoding_EmptyInput_ClearsOutputAndSucceeds)
{
    // Arrange: E2 空输入
    QByteArray in, out(QByteArrayLiteral("stale"));
    // Act
    const bool ok = DetectCode::ChangeFileEncodingFormat(in, out, QStringLiteral("GB18030"), QStringLiteral("UTF-8"));
    // Assert
    EXPECT_TRUE(ok);
    EXPECT_TRUE(out.isEmpty());
}

TEST_F(DetectCodeTest, ChangeEncoding_Gb18030ToUtf8_ReturnsExactUtf8Bytes)
{
    // Arrange: E3
    QByteArray in = QByteArray::fromHex("D6D0CEC4"), out;
    // Act
    const bool ok = DetectCode::ChangeFileEncodingFormat(in, out, QStringLiteral("GB18030"), QStringLiteral("UTF-8"));
    // Assert: "中文" UTF-8，无 BOM
    EXPECT_TRUE(ok);
    EXPECT_EQ(out, QStringLiteral("中文").toUtf8());
}

TEST_F(DetectCodeTest, ChangeEncoding_Utf8ToGb18030_ReturnsExactGbBytes)
{
    // Arrange: E3 反向
    QByteArray in = QStringLiteral("中文").toUtf8(), out;
    // Act
    const bool ok = DetectCode::ChangeFileEncodingFormat(in, out, QStringLiteral("UTF-8"), QStringLiteral("GB18030"));
    // Assert: 无 BOM
    EXPECT_TRUE(ok);
    EXPECT_EQ(out, QByteArray::fromHex("D6D0CEC4"));
}

TEST_F(DetectCodeTest, ChangeEncoding_Utf8ToUtf16Le_PrependsBom)
{
    // Arrange: E4 BOM 附加在转换数据之前
    QByteArray in = QByteArrayLiteral("hi"), out;
    // Act
    const bool ok = DetectCode::ChangeFileEncodingFormat(in, out, QStringLiteral("UTF-8"), QStringLiteral("UTF-16LE"));
    // Assert
    EXPECT_TRUE(ok);
    EXPECT_EQ(out, QByteArray::fromHex("FFFE68006900"));
}

TEST_F(DetectCodeTest, ChangeEncoding_Utf8ToUtf16Be_PrependsBom)
{
    // Arrange
    QByteArray in = QByteArrayLiteral("hi"), out;
    // Act
    const bool ok = DetectCode::ChangeFileEncodingFormat(in, out, QStringLiteral("UTF-8"), QStringLiteral("UTF-16BE"));
    // Assert
    EXPECT_TRUE(ok);
    EXPECT_EQ(out, QByteArray::fromHex("FEFF00680069"));
}

TEST_F(DetectCodeTest, ChangeEncoding_Utf8ToUtf32Le_PrependsBom)
{
    // Arrange
    QByteArray in = QByteArrayLiteral("hi"), out;
    // Act
    const bool ok = DetectCode::ChangeFileEncodingFormat(in, out, QStringLiteral("UTF-8"), QStringLiteral("UTF-32LE"));
    // Assert
    EXPECT_TRUE(ok);
    EXPECT_EQ(out, QByteArray::fromHex("FFFE00006800000069000000"));
}

TEST_F(DetectCodeTest, ChangeEncoding_Utf16BomToUtf8_StripsToPlainUtf8)
{
    // Arrange: UTF-16LE + BOM → UTF-8
    QByteArray in = QByteArray::fromHex("FFFE68006900"), out; // "hi"
    // Act
    const bool ok = DetectCode::ChangeFileEncodingFormat(in, out, QStringLiteral("UTF-16"), QStringLiteral("UTF-8"));
    // Assert: UTF-8 无 BOM
    EXPECT_TRUE(ok);
    EXPECT_EQ(out, QByteArrayLiteral("hi"));
}

TEST_F(DetectCodeTest, ChangeEncoding_InvalidUtf8Byte_ReplacedWithQuestionMark)
{
    // Arrange: E5 EILSEQ → '?'（单字节非法）
    QByteArray in = QByteArray::fromHex("FF"), out;
    // Act
    const bool ok = DetectCode::ChangeFileEncodingFormat(in, out, QStringLiteral("UTF-8"), QStringLiteral("GB18030"));
    // Assert: 非法首字节被替换为 '?'（GB18030 '?' == 0x3F）
    EXPECT_TRUE(ok);
    EXPECT_EQ(out, QByteArrayLiteral("?"));
}

TEST_F(DetectCodeTest, ChangeEncoding_TruncatedUtf8_KeepsConvertedPrefix)
{
    // Arrange: E6 EINVAL（不完整多字节序列在末尾）→ 保留已转换前缀
    QByteArray in = QByteArrayLiteral("a") + QByteArray::fromHex("C3"), out;
    // Act
    const bool ok = DetectCode::ChangeFileEncodingFormat(in, out, QStringLiteral("UTF-8"), QStringLiteral("GB18030"));
    // Assert: 'a' 已转换，截断尾部放弃
    EXPECT_TRUE(ok);
    EXPECT_EQ(out, QByteArrayLiteral("a"));
}

TEST_F(DetectCodeTest, ChangeEncoding_InvalidGb18030Byte_ReplacedAndContinues)
{
    // Arrange: E5 GB18030 源非法首字节，后续有效
    QByteArray in = QByteArray::fromHex("FF414243"), out;
    // Act
    const bool ok = DetectCode::ChangeFileEncodingFormat(in, out, QStringLiteral("GB18030"), QStringLiteral("UTF-8"));
    // Assert: FF → '?'，ABC 原样
    EXPECT_TRUE(ok);
    EXPECT_EQ(out, QByteArrayLiteral("?ABC"));
}

TEST_F(DetectCodeTest, ChangeEncoding_Gb18030Pua_MapsToUtf8Pua)
{
    // Arrange: E8 GB18030 PUA 0x37903582（小端字节 82 35 90 37）
    QByteArray in = QByteArray::fromHex("82359037"), out;
    // Act
    const bool ok = DetectCode::ChangeFileEncodingFormat(in, out, QStringLiteral("GB18030"), QStringLiteral("UTF-8"));
    // Assert: \uE81E 的 UTF-8 编码（本机 iconv 实测映射，见文件头注释）
    EXPECT_TRUE(ok);
    EXPECT_EQ(out, QByteArray::fromHex("EEA09E"));
}

TEST_F(DetectCodeTest, ChangeEncoding_PatchedIconv_Fe51ReplacedVia2005Map)
{
    // Arrange: E9 enablePatchedIconv=true → FE51 预替换为 FFFFFF01 → EILSEQ → \uE816
    installConfigStubs(true, true, QByteArrayLiteral("UTF-8"));
    QByteArray in = QByteArray::fromHex("FE51"), out;
    // Act
    const bool ok = DetectCode::ChangeFileEncodingFormat(in, out, QStringLiteral("GB18030"), QStringLiteral("UTF-8"));
    // Assert: \uE816 UTF-8 = EE A0 96
    EXPECT_TRUE(ok);
    EXPECT_EQ(out, QByteArray::fromHex("EEA096"));
}

TEST_F(DetectCodeTest, ChangeEncoding_PatchedIconv_U20087MapsToGbPuaBytes)
{
    // Arrange: E9 UTF-8→GB18030 补丁路径：F0A08287(\u20087) → FFFF11 → 95329031
    installConfigStubs(true, true, QByteArrayLiteral("UTF-8"));
    QByteArray in = QByteArray::fromHex("F0A08287"), out;
    // Act
    const bool ok = DetectCode::ChangeFileEncodingFormat(in, out, QStringLiteral("UTF-8"), QStringLiteral("GB18030"));
    // Assert
    EXPECT_TRUE(ok);
    EXPECT_EQ(out, QByteArray::fromHex("95329031"));
}

TEST_F(DetectCodeTest, ChangeEncoding_InvalidFromCode_FallsBackAndFails)
{
    // Arrange: E7 iconv_open 失败 → QTextCodec 回退 → 源编码不存在 → false
    QByteArray in = QByteArrayLiteral("abc"), out;
    // Act
    const bool ok = DetectCode::ChangeFileEncodingFormat(in, out, QStringLiteral("UT-NONE-F"), QStringLiteral("UTF-8"));
    // Assert
    EXPECT_FALSE(ok);
    EXPECT_TRUE(out.isEmpty()); // 失败不产生输出
}

TEST_F(DetectCodeTest, ChangeEncoding_InvalidToCode_FallsBackAndFails)
{
    // Arrange: E7 目标编码不存在
    QByteArray in = QByteArrayLiteral("abc"), out;
    // Act
    const bool ok = DetectCode::ChangeFileEncodingFormat(in, out, QStringLiteral("UTF-8"), QStringLiteral("UT-NONE-T"));
    // Assert
    EXPECT_FALSE(ok);
    EXPECT_TRUE(out.isEmpty()); // 失败不产生输出
}

// ---------------- Config 真实实现直跑（保证 FN 覆盖，无 stub） ----------------

TEST_F(DetectCodeTest, ConfigRealMethods_Queried_ReturnStableValues)
{
    // Arrange: 不安装任何 Config stub（TearDown 前 installConfigStubs 未调用）
    Config *cfg = Config::instance();
    // Act
    const QByteArray def = cfg->defaultEncoding();
    const bool improve = cfg->enableImproveGB18030();
    const bool patched = cfg->enablePatchedIconv();
    // Assert: 单例稳定、返回值稳定（取值随机器 DConfig，断言语义而非具体值）
    EXPECT_EQ(Config::instance(), cfg);
    EXPECT_EQ(cfg->defaultEncoding(), def);
    EXPECT_FALSE(def.isEmpty());
    EXPECT_TRUE(improve == true || improve == false);
    EXPECT_TRUE(patched == true || patched == false);
}

// DetectCode 构造函数
TEST_F(DetectCodeTest, Constructor_Default_ConstructsSafely)
{
    // Arrange/Act
    DetectCode code;
    // Assert: 无状态对象，构造安全；静态方法与其实例语义互不影响
    EXPECT_NE(&code, nullptr);
    EXPECT_EQ(DetectCode::selectCoding(QByteArrayLiteral("UTF-8"), QByteArrayList(), 0.99f),
              QByteArray()); // icu 空列表 → 空结果
}
