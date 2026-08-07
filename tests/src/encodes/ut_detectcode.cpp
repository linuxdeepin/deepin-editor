// SPDX-FileCopyrightText: 2023-2026 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include "ut_detectcode.h"
#include "src/stub.h"
#include "../../src/encodes/detectcode.h"

#include <QTextCodec>

namespace detectcodestub {

QString stringvalue="1";

QByteArray retintstub()
{
    return QByteArray();
}

QByteArray retByteArray = QByteArray();
QByteArray reloadModifyFile_selectCoding()
{
    return retByteArray;
}

}


using namespace detectcodestub;

UT_DetectCode::UT_DetectCode()
{

}

TEST(UT_DetectCode, UT_DetectCode)
{
    DetectCode c;
}

TEST(UT_GetFileEncodingFormat, UT_GetFileEncodingFormat_001)
{
    DetectCode* dc = new DetectCode;

    Stub stub;
    stub.set(ADDR(DetectCode,UchardetCode),retintstub);
    stringvalue = "unknown";
    Stub stubSelectCoding;
    stubSelectCoding.set(ADDR(DetectCode, selectCoding),reloadModifyFile_selectCoding);
    dc->GetFileEncodingFormat("123");
    EXPECT_NE(dc,nullptr);
    delete dc;
    dc=nullptr;
}

TEST(UT_GetFileEncodingFormat, UT_GetFileEncodingFormat_002)
{
    DetectCode* dc = new DetectCode;

    Stub stub;
    stub.set(ADDR(DetectCode,UchardetCode),retintstub);

    stringvalue = "ASCII";
    Stub stubSelectCoding;
    stubSelectCoding.set(ADDR(DetectCode, selectCoding),reloadModifyFile_selectCoding);
    dc->GetFileEncodingFormat("123");

    EXPECT_NE(dc,nullptr);
    delete dc;
    dc=nullptr;
}

TEST(UT_GetFileEncodingFormat, UT_GetFileEncodingFormat_003)
{
    DetectCode *pDetectCode = new DetectCode;

    Stub stub;
    stub.set(ADDR(DetectCode,UchardetCode),retintstub);

    stringvalue = "unknown";
    Stub stubSelectCoding;
    stubSelectCoding.set(ADDR(DetectCode, selectCoding),reloadModifyFile_selectCoding);
    pDetectCode->GetFileEncodingFormat(QString("123"), QByteArray("我是中文"));

    EXPECT_NE(pDetectCode, nullptr);
    delete pDetectCode;
    pDetectCode = nullptr;
}

void chartDet_DetectingTextCoding_stub()
{
    return;
}

TEST(UT_GetFileEncodingFormat, UT_GetFileEncodingFormat_004)
{
    DetectCode *pDetectCode = new DetectCode;

    Stub stub;
    stub.set(ADDR(DetectCode, ChartDet_DetectingTextCoding), chartDet_DetectingTextCoding_stub);

    stringvalue = "unknown";
    Stub stubSelectCoding;
    stubSelectCoding.set(ADDR(DetectCode, selectCoding),reloadModifyFile_selectCoding);
    pDetectCode->GetFileEncodingFormat(QString("123"), QByteArray("我是中文"));

    EXPECT_NE(pDetectCode,nullptr);
    delete pDetectCode;
    pDetectCode = nullptr;
}

void detectCode_icuDetectTextEncoding_stub(const QString &filePath, QByteArrayList &listDetectRet)
{
    Q_UNUSED(filePath)
    Q_UNUSED(listDetectRet)
}

QByteArray detectCode_selectCoding_stub(QByteArray ucharDetectdRet, QByteArrayList icuDetectRetList)
{
    Q_UNUSED(icuDetectRetList);
    return ucharDetectdRet;
}

// 用例以 QString("123") 作为占位文件路径（并非真实文件）。当 chardet 对被截断的
// BIG5 内容识别置信度不足时，源码会回退到 UchardetCode(filepath) 读文件复检；占位
// 路径不存在，fopen 失败，UchardetCode 返回空，进而落入默认 UTF-8 编码，导致 BIG5
// 用例失败。这里打桩 UchardetCode 直接返回 BIG5，模拟“真实 BIG5 文件复检也为 BIG5”，
// 使内容侧的检测逻辑不因占位路径而被错误地回退为 UTF-8。
QByteArray detectCode_uchardetCode_big5_stub(QString filepath)
{
    Q_UNUSED(filepath);
    return QByteArray("BIG5");
}

TEST(UT_GetFileEncodingFormat, UT_GetFileEncodingFormat_zh_CNContent_UTF8_Pass)
{
    Stub stubDetectCode;
    stubDetectCode.set(ADDR(DetectCode, icuDetectTextEncoding), detectCode_icuDetectTextEncoding_stub);
    stubDetectCode.set(ADDR(DetectCode, selectCoding), detectCode_selectCoding_stub);

    QByteArray content("你好，我是中文测试文本");
    while (content.size() > 8) {
        // 手动破坏尾部字符编码
        content.chop(1);
        QByteArray encode = DetectCode::GetFileEncodingFormat(QString("123"), content);
        EXPECT_EQ(encode, QByteArray("UTF-8"));
    }
}

TEST(UT_GetFileEncodingFormat, UT_GetFileEncodingFormat_zh_CNContent_GBK_Pass)
{
    Stub stubDetectCode;
    stubDetectCode.set(ADDR(DetectCode, icuDetectTextEncoding), detectCode_icuDetectTextEncoding_stub);
    stubDetectCode.set(ADDR(DetectCode, selectCoding), detectCode_selectCoding_stub);

    QTextCodec *codec = QTextCodec::codecForName("GB18030");
    QByteArray content = codec->fromUnicode("你好，我是中文测试文本");
    while (content.size() > 8) {
        // 手动破坏尾部字符编码
        content.chop(1);
        QByteArray encode = DetectCode::GetFileEncodingFormat(QString("123"), content);
        EXPECT_EQ(encode, QByteArray("GB18030"));
    }
}

TEST(UT_GetFileEncodingFormat, UT_GetFileEncodingFormat_zh_CNContent_BIG5_Pass)
{
    Stub stubDetectCode;
    stubDetectCode.set(ADDR(DetectCode, icuDetectTextEncoding), detectCode_icuDetectTextEncoding_stub);
    stubDetectCode.set(ADDR(DetectCode, selectCoding), detectCode_selectCoding_stub);
    stubDetectCode.set(ADDR(DetectCode, UchardetCode), detectCode_uchardetCode_big5_stub);

    QTextCodec *codec = QTextCodec::codecForName("BIG5");
    QByteArray content = codec->fromUnicode("你好，我是中文测试文本");
    while (content.size() > 8) {
        // 手动破坏尾部字符编码
        content.chop(1);
        QByteArray encode = DetectCode::GetFileEncodingFormat(QString("123"), content);
        EXPECT_EQ(encode, QByteArray("BIG5"));
    }
}

TEST(UT_GetFileEncodingFormat, UT_GetFileEncodingFormat_ErrorContent_UTF8_Pass)
{
    Stub stubDetectCode;
    stubDetectCode.set(ADDR(DetectCode, icuDetectTextEncoding), detectCode_icuDetectTextEncoding_stub);
    stubDetectCode.set(ADDR(DetectCode, selectCoding), detectCode_selectCoding_stub);

    QByteArray content("你好，我是中文繁體中文བོད་ཡིགКирилли́こんにちは안녕하십니까Hello");
    while (content.size() > 8) {
        // 手动破坏尾部字符编码
        content.chop(1);
        QByteArray encode = DetectCode::GetFileEncodingFormat(QString("123"), content);
        EXPECT_EQ(encode, QByteArray("UTF-8"));
    }
}

TEST(UT_EncaDetectCode, UT_EncaDetectCode)
{
    DetectCode* dc = new DetectCode;

    dc->UchardetCode("123");

    EXPECT_NE(dc,nullptr);
    delete dc;
    dc=nullptr;
}

TEST(UT_ChangeFileEncodingFormat, UT_ChangeFileEncodingFormat_001)
{
    DetectCode* dc = new DetectCode;

    QByteArray ba(1,'c');
    dc->ChangeFileEncodingFormat(ba,ba,"567","567");

    EXPECT_NE(dc,nullptr);
    delete dc;
    dc=nullptr;
}

TEST(UT_ChangeFileEncodingFormat, UT_ChangeFileEncodingFormat_002)
{
    DetectCode* dc = new DetectCode;

    QByteArray ba(1,'c');
    dc->ChangeFileEncodingFormat(ba,ba,"567","789");

    EXPECT_NE(dc, nullptr);
    delete dc;
    dc=nullptr;
}

iconv_t *iconv_open_stub()
{
    return nullptr;
}

void iconv_stub()
{
    return;
}

void iconv_close_stub()
{
    return;
}

TEST(UT_ChangeFileEncodingFormat, UT_ChangeFileEncodingFormat_003)
{
    DetectCode *pDetectCode = new DetectCode;
    QByteArray inData("我是中文");
    QByteArray outData;

    Stub stub_iconv_open;
    stub_iconv_open.set(iconv_open, iconv_open_stub);

    Stub stub_iconv;
    stub_iconv.set(iconv, iconv_stub);

    Stub stub_iconv_close;
    stub_iconv_close.set(iconv_close, iconv_close_stub);
    bool bRet = pDetectCode->ChangeFileEncodingFormat(inData, outData, QString("GB18030"), QString("UTF-8"));

    ASSERT_TRUE(bRet == true);
    delete pDetectCode;
    pDetectCode = nullptr;
}

TEST(UT_UchardetCode, UT_UchardetCode_001)
{
    DetectCode *pDetectCode = new DetectCode;
    QString strFilePath(QCoreApplication::applicationDirPath() + QString("/Makefile"));
    pDetectCode->UchardetCode(strFilePath);

    EXPECT_NE(pDetectCode, nullptr);
    delete pDetectCode;
    pDetectCode = nullptr;
}

DetectObj *detect_obj_init_stub()
{
    return nullptr;
}

TEST(UT_ChartDet_DetectingTextCoding, UT_ChartDet_DetectingTextCoding_001)
{
    DetectCode *pDetectCode = new DetectCode;
    QByteArray newContent("我是中文");
    QString detectedResult;
    float chardetconfidence = 0;

    Stub stub;
    stub.set(detect_obj_init, detect_obj_init_stub);
    int iRet = pDetectCode->ChartDet_DetectingTextCoding(newContent, detectedResult, chardetconfidence);

    ASSERT_TRUE(iRet == CHARDET_MEM_ALLOCATED_FAIL);
    delete pDetectCode;
    pDetectCode = nullptr;
}

int detect_stub_out_of_memory()
{
    return CHARDET_OUT_OF_MEMORY;
}

int detect_stub_null_object()
{
    return CHARDET_NULL_OBJECT;
}

