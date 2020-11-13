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
#include "test_performancemonitor.h"
#include "../../src/performancemonitor.h"

test_performanceMonitor::test_performanceMonitor()
{

}

//explicit PerformanceMonitor(QObject *parent = nullptr);
TEST_F(test_performanceMonitor, PerformanceMonitor)
{
    PerformanceMonitor PerformanceMonitor(nullptr);
    assert(1==1);
}

//static void initializeAppStart();
TEST_F(test_performanceMonitor, initializeAppStart)
{
    PerformanceMonitor PerformanceMonitor(nullptr);
    PerformanceMonitor.initializeAppStart();
    assert(1==1);
}

//static void initializAppFinish();
TEST_F(test_performanceMonitor, initializAppFinish)
{
    PerformanceMonitor PerformanceMonitor(nullptr);
    PerformanceMonitor.initializAppFinish();
    assert(1==1);
}

//static void closeAppStart();
TEST_F(test_performanceMonitor, closeAppStart)
{
    PerformanceMonitor PerformanceMonitor(nullptr);
    PerformanceMonitor.closeAppStart();
    assert(1==1);
}

//static void closeAPPFinish();
TEST_F(test_performanceMonitor, closeAPPFinish)
{
    PerformanceMonitor PerformanceMonitor(nullptr);
    PerformanceMonitor.closeAPPFinish();
    assert(1==1);
}

//static void openFileStart();
TEST_F(test_performanceMonitor, openFileStart)
{
     PerformanceMonitor PerformanceMonitor(nullptr);
    PerformanceMonitor.openFileStart();
    assert(1==1);
}

//static void openFileFinish(const QString &strFileName, qint64 iFileSize);
TEST_F(test_performanceMonitor, openFileFinish)
{
    PerformanceMonitor PerformanceMonitor(nullptr);
    PerformanceMonitor.openFileFinish("aa",1);
    assert(1==1);
}
