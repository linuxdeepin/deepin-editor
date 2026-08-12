// SPDX-FileCopyrightText: 2026 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include "ut_warningnotices.h"
#include "../../src/controls/warningnotices.h"

#include <QFont>
#include <QApplication>
#include <DGuiApplicationHelper>
DGUI_USE_NAMESPACE
#include <DDialogCloseButton>

test_warningnotices::test_warningnotices()
{
}

// Constructor
TEST_F(test_warningnotices, WarningNotices)
{
    WarningNotices notices(DFloatingMessage::ResidentType);
    EXPECT_NE(&notices, nullptr);
}

// void setEditAnywayBtn();
TEST_F(test_warningnotices, setEditAnywayBtn)
{
    WarningNotices *notices = new WarningNotices(DFloatingMessage::ResidentType);
    EXPECT_NO_FATAL_FAILURE(notices->setEditAnywayBtn());
    // m_editAnywayBtn is shown (not hidden), reload/saveAs are hidden
    EXPECT_EQ(notices->m_editAnywayBtn->isHidden(), false);
    EXPECT_EQ(notices->m_reloadBtn->isHidden(), true);
    EXPECT_EQ(notices->m_saveAsBtn->isHidden(), true);
    notices->deleteLater();
}

// void clearBtn();
TEST_F(test_warningnotices, clearBtn)
{
    WarningNotices *notices = new WarningNotices(DFloatingMessage::ResidentType);
    notices->setReloadBtn();
    EXPECT_NO_FATAL_FAILURE(notices->clearBtn());
    EXPECT_EQ(notices->m_reloadBtn->isVisible(), false);
    EXPECT_EQ(notices->m_saveAsBtn->isVisible(), false);
    notices->deleteLater();
}

// void slotEditAnywayBtnClicked();
TEST_F(test_warningnotices, slotEditAnywayBtnClicked)
{
    WarningNotices *notices = new WarningNotices(DFloatingMessage::ResidentType);
    bool signalReceived = false;
    QObject::connect(notices, &WarningNotices::editAnywayBtnClicked, [&]() { signalReceived = true; });

    notices->show();
    EXPECT_NO_FATAL_FAILURE(notices->slotEditAnywayBtnClicked());
    EXPECT_EQ(signalReceived, true);

    notices->deleteLater();
}

// Constructor lambda #3 connected to DFloatingMessage::closeButtonClicked
TEST_F(test_warningnotices, ConstructorCloseButtonClickedLambda)
{
    WarningNotices *notices = new WarningNotices(DFloatingMessage::ResidentType);
    notices->show();

    // Directly emit the protected signal (allowed via -fno-access-control) to
    // guarantee the constructor lambda #3 executes.
    emit notices->closeButtonClicked();

    notices->deleteLater();
}

// Constructor lambda #1 connected to DGuiApplicationHelper::sizeModeChanged
TEST_F(test_warningnotices, ConstructorSizeModeLambda)
{
    WarningNotices *notices = new WarningNotices(DFloatingMessage::ResidentType);
    auto helper = DGuiApplicationHelper::instance();
    auto origMode = helper->sizeMode();

    EXPECT_NO_FATAL_FAILURE(helper->setSizeMode(origMode == DGuiApplicationHelper::NormalMode
                                                    ? DGuiApplicationHelper::CompactMode
                                                    : DGuiApplicationHelper::NormalMode));
    helper->setSizeMode(origMode); // restore

    notices->deleteLater();
}

// Constructor lambda #2 connected to DGuiApplicationHelper::fontChanged
TEST_F(test_warningnotices, ConstructorFontChangedLambda)
{
    WarningNotices *notices = new WarningNotices(DFloatingMessage::ResidentType);

    // Directly emit the signal (allowed via -fno-access-control) to guarantee
    // the constructor lambda #2 executes.
    auto helper = DGuiApplicationHelper::instance();
    QFont font = notices->font();
    EXPECT_NO_FATAL_FAILURE(emit helper->fontChanged(font));

    notices->deleteLater();
}

// void setSaveAsBtn();
TEST_F(test_warningnotices, setSaveAsBtn)
{
    WarningNotices *notices = new WarningNotices(DFloatingMessage::ResidentType);
    EXPECT_NO_FATAL_FAILURE(notices->setSaveAsBtn());
    EXPECT_EQ(notices->m_saveAsBtn->isHidden(), false);
    EXPECT_EQ(notices->m_reloadBtn->isHidden(), true);
    notices->deleteLater();
}

// void slotreloadBtnClicked();
TEST_F(test_warningnotices, slotreloadBtnClicked)
{
    WarningNotices *notices = new WarningNotices(DFloatingMessage::ResidentType);
    bool signalReceived = false;
    QObject::connect(notices, &WarningNotices::reloadBtnClicked, [&]() { signalReceived = true; });

    notices->show();
    EXPECT_NO_FATAL_FAILURE(notices->slotreloadBtnClicked());
    EXPECT_EQ(signalReceived, true);

    notices->deleteLater();
}

// void slotsaveAsBtnClicked();
TEST_F(test_warningnotices, slotsaveAsBtnClicked)
{
    WarningNotices *notices = new WarningNotices(DFloatingMessage::ResidentType);
    bool signalReceived = false;
    QObject::connect(notices, &WarningNotices::saveAsBtnClicked, [&]() { signalReceived = true; });

    notices->show();
    EXPECT_NO_FATAL_FAILURE(notices->slotsaveAsBtnClicked());
    EXPECT_EQ(signalReceived, true);

    notices->deleteLater();
}
