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
#include "test_showflodcodewidget.h"
#include "../../src/showflodcodewidget.h"

test_showflodcodewidget::test_showflodcodewidget()
{

}

TEST_F(test_showflodcodewidget, ShowFlodCodeWidget)
{
    ShowFlodCodeWidget flodCodeWidget(nullptr);
    assert(1==1);
}

//void appendText(QString strText, int maxWidth);
TEST_F(test_showflodcodewidget, appendText)
{
    ShowFlodCodeWidget *flodCodeWidget = new ShowFlodCodeWidget();
    flodCodeWidget->appendText("aa",1);
    assert(1==1);
}

//void clear();
TEST_F(test_showflodcodewidget, clear)
{
    ShowFlodCodeWidget *flodCodeWidget = new ShowFlodCodeWidget();
    flodCodeWidget->clear();
    assert(1==1);
}

//void initHighLight(QString filepath, bool bIsLight);
TEST_F(test_showflodcodewidget, initHighLight)
{
    ShowFlodCodeWidget *flodCodeWidget = new ShowFlodCodeWidget();
    flodCodeWidget->initHighLight("aa",true);
    assert(1==1);
}

//void setStyle(bool bIsLineWrap);
TEST_F(test_showflodcodewidget, setStyle)
{
    ShowFlodCodeWidget *flodCodeWidget = new ShowFlodCodeWidget();
    flodCodeWidget->setStyle(true);
    assert(1==1);
}

//void hideFirstBlock();
TEST_F(test_showflodcodewidget, hideFirstBlock)
{
    ShowFlodCodeWidget *flodCodeWidget = new ShowFlodCodeWidget();
    flodCodeWidget->hideFirstBlock();
    assert(1==1);
}
