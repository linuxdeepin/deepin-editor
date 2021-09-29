#include "ut_detectcode.h"
#include "src/stub.h"
#include "../../src/encodes/detectcode.h"
namespace detectcodestub {

QString stringvalue="1";
QString retstringstub()
{
    return stringvalue;
}

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
    stub.set(ADDR(DetectCode,EncaDetectCode),retstringstub);
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
    stub.set(ADDR(DetectCode,EncaDetectCode),retstringstub);
    stub.set(ADDR(DetectCode,UchardetCode),retintstub);

    stringvalue = "ASCII";
    dc->GetFileEncodingFormat("123");

    EXPECT_NE(dc,nullptr);
    delete dc;
    dc=nullptr;
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

    EXPECT_NE(dc,nullptr);
    delete dc;
    dc=nullptr;
}
