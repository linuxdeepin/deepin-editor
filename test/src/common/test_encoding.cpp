#include "test_encoding.h"

test_encoding::test_encoding()
{

}

TEST_F(test_encoding, FileLoadThread)
{
    extern bool validateUTF8 (const QByteArray byteArray);
    char i = 'a';
    QByteArray a(3,i);
    detectCharset(a);
    assert(1==1);
}
