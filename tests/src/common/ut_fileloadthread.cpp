// SPDX-FileCopyrightText: 2019 - 2022 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include "ut_fileloadthread.h"
#include "../../src/common/fileloadthread.h"

test_fileloadthread::test_fileloadthread()
{
}

void test_fileloadthread::SetUp()
{
    fthread = new FileLoadThread("aa");

    EXPECT_NE(fthread,nullptr);

}

void test_fileloadthread::TearDown()
{
    delete fthread;
    fthread=nullptr;
}

//FileLoadThread(const QString &filepath, QObject *QObject = nullptr);
TEST_F(test_fileloadthread, FileLoadThread)
{
    FileLoadThread thread("aa");

    EXPECT_EQ(thread.m_strFilePath,"aa");
}

//void run();
TEST_F(test_fileloadthread, run)
{
    FileLoadThread *thread = new FileLoadThread("aa");
    thread->run();

    EXPECT_NE(thread,nullptr);
    thread->deleteLater();
}

//void setEncodeInfo(QStringList pathList,QStringList codeList);
TEST_F(test_fileloadthread, setEncodeInfo)
{
    QStringList pathList, codeList;
    FileLoadThread *thread = new FileLoadThread("aa");
    //thread->setEncodeInfo(pathList,codeList);


    EXPECT_NE(thread,nullptr);
    thread->deleteLater();
}

//QString getCodec();
TEST_F(test_fileloadthread, getCodec)
{
    FileLoadThread *thread = new FileLoadThread("aa");
    //  thread->getCodec();

    EXPECT_NE(thread,nullptr);
    thread->deleteLater();
}
