#include "ut_detectcode.h"
#include "src/stub.h"
#include "../../src/encodes/detectcode.h"
namespace detectcodestub {

QString stringvalue="1";

int intvalue=1;
int retintstub()
{
    return intvalue;
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
    pDetectCode->GetFileEncodingFormat(QString("123"), QByteArray("我是中文"));

    EXPECT_NE(pDetectCode,nullptr);
    delete pDetectCode;
    pDetectCode = nullptr;
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

TEST(UT_ChartDet_DetectingTextCoding, UT_ChartDet_DetectingTextCoding_002)
{
    DetectCode *pDetectCode = new DetectCode;
    QByteArray newContent("我是中文");
    QString detectedResult;
    float chardetconfidence = 0;

    Stub stub;
    stub.set(detect, detect_stub_out_of_memory);
    int iRet = pDetectCode->ChartDet_DetectingTextCoding(newContent, detectedResult, chardetconfidence);

    ASSERT_TRUE(iRet == CHARDET_OUT_OF_MEMORY);
    delete pDetectCode;
    pDetectCode = nullptr;
}


int detect_stub_null_object()
{
    return CHARDET_NULL_OBJECT;
}

TEST(UT_ChartDet_DetectingTextCoding, UT_ChartDet_DetectingTextCoding_003)
{
    DetectCode *pDetectCode = new DetectCode;
    QByteArray newContent("我是中文");
    QString detectedResult;
    float chardetconfidence = 0;

    Stub stub;
    stub.set(detect, detect_stub_null_object);
    int iRet = pDetectCode->ChartDet_DetectingTextCoding(newContent, detectedResult, chardetconfidence);

    ASSERT_TRUE(iRet == CHARDET_NULL_OBJECT);
    delete pDetectCode;
    pDetectCode = nullptr;
}

TEST(UT_ChartDet_DetectingTextCoding, UT_ChartDet_DetectingTextCoding_004)
{
    DetectCode *pDetectCode = new DetectCode;
    QByteArray newContent("我是中文");
    QString detectedResult;
    float chardetconfidence = 0;
    int iRet = pDetectCode->ChartDet_DetectingTextCoding(newContent, detectedResult, chardetconfidence);

    ASSERT_TRUE(iRet == CHARDET_SUCCESS);
    delete pDetectCode;
    pDetectCode = nullptr;
}
