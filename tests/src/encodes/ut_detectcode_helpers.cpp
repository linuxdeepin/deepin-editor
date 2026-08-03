// SPDX-FileCopyrightText: 2026 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later
//
// 覆盖 detectcode.cpp 中文件作用域的自由函数 (未在头文件导出，具有外部链接):
//   utf8MultiByteCount / checkGB18030ToUtf8Error / checkUTF8ToGB18030Error
// 及 utf8MultiByteCount 内部的 lambda。

#include <gtest/gtest.h>
#include <QByteArray>

// 声明 detectcode.cpp 中的文件作用域函数
int utf8MultiByteCount(char *buf, size_t size);
bool checkGB18030ToUtf8Error(char *buf, size_t size, size_t &replaceLen, QByteArray &appendChar);
bool checkUTF8ToGB18030Error(char *buf, size_t size, size_t &replaceLen, QByteArray &appendChar);

// utf8MultiByteCount: 单字节 ASCII
TEST(UT_Utf8MultiByteCount, SingleByte)
{
    char buf[] = {'A'};
    EXPECT_EQ(utf8MultiByteCount(buf, 1), 1);
}

// utf8MultiByteCount: 双字节起始符 (110xxxxx)
TEST(UT_Utf8MultiByteCount, DoubleBytes)
{
    char buf[] = {static_cast<char>(0xC2), static_cast<char>(0x80)};
    EXPECT_EQ(utf8MultiByteCount(buf, 2), 2);
}

// utf8MultiByteCount: 三字节起始符 (1110xxxx)
TEST(UT_Utf8MultiByteCount, ThreeBytes)
{
    char buf[] = {static_cast<char>(0xE0), static_cast<char>(0x80), static_cast<char>(0x80)};
    EXPECT_EQ(utf8MultiByteCount(buf, 3), 3);
}

// utf8MultiByteCount: 四字节起始符 (11110xxx)
TEST(UT_Utf8MultiByteCount, FourBytes)
{
    char buf[] = {static_cast<char>(0xF0), static_cast<char>(0x80), static_cast<char>(0x80), static_cast<char>(0x80)};
    EXPECT_EQ(utf8MultiByteCount(buf, 4), 4);
}

// utf8MultiByteCount: 连续字节 (10xxxxxx)，累加至 4 (触发内部 lambda)
TEST(UT_Utf8MultiByteCount, ContinuationBytes)
{
    char buf[] = {static_cast<char>(0x80), static_cast<char>(0x80), static_cast<char>(0x80), static_cast<char>(0x80)};
    EXPECT_EQ(utf8MultiByteCount(buf, 4), 4);
}

// utf8MultiByteCount: 连续字节，size 先耗尽
TEST(UT_Utf8MultiByteCount, ContinuationBytesShort)
{
    char buf[] = {static_cast<char>(0x80), static_cast<char>(0x80)};
    EXPECT_EQ(utf8MultiByteCount(buf, 2), 2);
}

// utf8MultiByteCount: 超过 4 个前导 1 的非法字节，default 返回 1
TEST(UT_Utf8MultiByteCount, InvalidLeadingBits)
{
    char buf[] = {static_cast<char>(0xF8)};
    EXPECT_EQ(utf8MultiByteCount(buf, 1), 1);
}

// checkGB18030ToUtf8Error: size 小于 4，返回 false
TEST(UT_CheckGB18030ToUtf8Error, ShortBuffer)
{
    char buf[] = {static_cast<char>(0x82), static_cast<char>(0x35)};
    size_t replaceLen = 0;
    QByteArray appendChar;
    bool ret = checkGB18030ToUtf8Error(buf, 2, replaceLen, appendChar);

    EXPECT_FALSE(ret);
    EXPECT_EQ(replaceLen, static_cast<size_t>(1));
    EXPECT_EQ(appendChar, QByteArray("?"));
}

// checkGB18030ToUtf8Error: 不匹配的 4 字节，返回 false
TEST(UT_CheckGB18030ToUtf8Error, NoMatch)
{
    char buf[] = {0x00, 0x01, 0x02, 0x03};
    size_t replaceLen = 0;
    QByteArray appendChar;
    bool ret = checkGB18030ToUtf8Error(buf, 4, replaceLen, appendChar);

    EXPECT_FALSE(ret);
    EXPECT_EQ(replaceLen, static_cast<size_t>(1));
    EXPECT_EQ(appendChar, QByteArray("?"));
}

// checkGB18030ToUtf8Error: 命中 PUA 映射 (小端序 0x37903582 -> \uE81E)
TEST(UT_CheckGB18030ToUtf8Error, MatchPUA)
{
    char buf[] = {static_cast<char>(0x82), static_cast<char>(0x35), static_cast<char>(0x90), static_cast<char>(0x37)};
    size_t replaceLen = 0;
    QByteArray appendChar;
    bool ret = checkGB18030ToUtf8Error(buf, 4, replaceLen, appendChar);

    EXPECT_TRUE(ret);
    EXPECT_EQ(replaceLen, static_cast<size_t>(4));
    EXPECT_FALSE(appendChar.isEmpty());
}

// checkUTF8ToGB18030Error: size 小于 3，返回 false
TEST(UT_CheckUTF8ToGB18030Error, ShortBuffer)
{
    char buf[] = {static_cast<char>(0xEE), static_cast<char>(0xA0)};
    size_t replaceLen = 0;
    QByteArray appendChar;
    bool ret = checkUTF8ToGB18030Error(buf, 2, replaceLen, appendChar);

    EXPECT_FALSE(ret);
    EXPECT_EQ(replaceLen, static_cast<size_t>(1));
    EXPECT_EQ(appendChar, QByteArray("?"));
}

// checkUTF8ToGB18030Error: 命中 gs_UTF8MapGB18030Data (U+E81E -> 0x37903582)
TEST(UT_CheckUTF8ToGB18030Error, MatchPUA)
{
    // U+E81E 的 UTF-8 编码: EE A0 9E
    char buf[] = {static_cast<char>(0xEE), static_cast<char>(0xA0), static_cast<char>(0x9E)};
    size_t replaceLen = 0;
    QByteArray appendChar;
    bool ret = checkUTF8ToGB18030Error(buf, 3, replaceLen, appendChar);

    EXPECT_TRUE(ret);
    EXPECT_EQ(replaceLen, static_cast<size_t>(3));
    EXPECT_FALSE(appendChar.isEmpty());
}

// checkUTF8ToGB18030Error: 命中 gs_ReplaceUtf8ToGB18030_2005Error (U+E816 -> FE51)
TEST(UT_CheckUTF8ToGB18030Error, Match2005Error)
{
    // U+E816 的 UTF-8 编码: EE A0 96
    char buf[] = {static_cast<char>(0xEE), static_cast<char>(0xA0), static_cast<char>(0x96)};
    size_t replaceLen = 0;
    QByteArray appendChar;
    bool ret = checkUTF8ToGB18030Error(buf, 3, replaceLen, appendChar);

    EXPECT_TRUE(ret);
    EXPECT_EQ(replaceLen, static_cast<size_t>(3));
    EXPECT_FALSE(appendChar.isEmpty());
}

// checkUTF8ToGB18030Error: 不匹配的 3 字节，返回 false
TEST(UT_CheckUTF8ToGB18030Error, NoMatch)
{
    char buf[] = {0x00, 0x01, 0x02};
    size_t replaceLen = 0;
    QByteArray appendChar;
    bool ret = checkUTF8ToGB18030Error(buf, 3, replaceLen, appendChar);

    EXPECT_FALSE(ret);
    EXPECT_EQ(replaceLen, static_cast<size_t>(1));
    EXPECT_EQ(appendChar, QByteArray("?"));
}
