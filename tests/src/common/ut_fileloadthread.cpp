/*
* Copyright (C) 2019 ~ 2020 Deepin Technology Co., Ltd.
*
* Author:     liumaochuan <liumaochuan@uniontech.com>
* Maintainer: liumaochuan <liumaochuan@uniontech.com>
* This program is free software: you can redistribute it and/or modify
* it under the terms of the GNU General Public License as published by
* the Free Software Foundation, either version 3 of the License, or
* any later version.
* This program is distributed in the hope that it will be useful,
* but WITHOUT ANY WARRANTY; without even the implied warranty of
* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
* GNU General Public License for more details.
* You should have received a copy of the GNU General Public License
* along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/
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
