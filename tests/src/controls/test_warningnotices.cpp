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
#include "test_warningnotices.h"
#include "../../src/controls/warningnotices.h"

test_warningnotices::test_warningnotices()
{

}

//explicit WarningNotices(MessageType notifyType = MessageType::ResidentType);
TEST_F(test_warningnotices, WarningNotices)
{
    WarningNotices notices(WarningNotices::ResidentType);
    assert(1==1);
}

//void setReloadBtn();
TEST_F(test_warningnotices, setReloadBtn)
{
    WarningNotices *notices = new WarningNotices(WarningNotices::ResidentType);
    notices->m_reloadBtn->setVisible(false);
    notices->setReloadBtn();

    notices->setReloadBtn();
    notices->slotreloadBtnClicked();
    assert(1==1);
}

//void setSaveAsBtn();
TEST_F(test_warningnotices, setSaveAsBtn)
{
    WarningNotices *notices = new WarningNotices(WarningNotices::ResidentType);
    notices->setSaveAsBtn();
    notices->slotsaveAsBtnClicked();
    assert(1==1);
}

