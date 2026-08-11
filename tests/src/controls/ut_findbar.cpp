// SPDX-FileCopyrightText: 2019-2026 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include "ut_findbar.h"
#include "../../src/controls/findbar.h"
#include <QApplication>
#include <QFocusEvent>
#include <QEvent>

test_findbar::test_findbar()
{

}

TEST_F(test_findbar, FindBar)
{
    FindBar findBar(nullptr);
    
}

//bool isFocus();
TEST_F(test_findbar, isFocus)
{
    FindBar *findBar = new FindBar();

    EXPECT_NE(findBar,nullptr);
    EXPECT_EQ(findBar->isFocus(),false);

    findBar->deleteLater();
    
}

//void focus();
TEST_F(test_findbar, focus)
{
    FindBar *findBar = new FindBar();
    findBar->focus();
    EXPECT_NE(findBar,nullptr);

    findBar->deleteLater();
    
}

//void activeInput(QString text, QString file, int row, int column, int scrollOffset);
TEST_F(test_findbar, activeInput)
{
    FindBar *findBar = new FindBar();
    findBar->activeInput("aa","aa",1,1,1);


    EXPECT_NE(findBar,nullptr);
    EXPECT_EQ(findBar->m_editLine->lineEdit()->text(),"aa");

    findBar->deleteLater();

    
}

//void setMismatchAlert(bool isAlert);
TEST_F(test_findbar, setMismatchAlert)
{
    FindBar *findBar = new FindBar();
    findBar->setMismatchAlert(true);

    EXPECT_NE(findBar,nullptr);
    EXPECT_EQ(findBar->m_editLine->isAlert(),true);
    findBar->deleteLater();
    
}

//void receiveText(QString t);
TEST_F(test_findbar, receiveText)
{
    FindBar *findBar = new FindBar();
    findBar->receiveText("aa");

    EXPECT_NE(findBar,nullptr);
    EXPECT_EQ(findBar->m_receivedText,"aa");

    findBar->deleteLater();
    
}

//void setSearched(bool _);
TEST_F(test_findbar, setSearched)
{
    FindBar *findBar = new FindBar();
    findBar->setSearched(true);

    EXPECT_NE(findBar,nullptr);
    EXPECT_EQ(findBar->searched,true);
    findBar->deleteLater();
    
}

//void findPreClicked();
TEST_F(test_findbar, findPreClicked)
{
    FindBar *findBar = new FindBar();
    findBar->findPreClicked();


    EXPECT_NE(findBar,nullptr);
    EXPECT_EQ(findBar->searched,true);
    findBar->deleteLater();
}

//public slots:
//void findCancel();
TEST_F(test_findbar, findCancel)
{
    FindBar *findBar = new FindBar();
    findBar->findCancel();

    EXPECT_NE(findBar,nullptr);
    EXPECT_EQ(findBar->isVisible(),false);
    findBar->deleteLater();
    
}

//void handleContentChanged();
TEST_F(test_findbar, handleContentChanged)
{
    FindBar *findBar = new FindBar();
    findBar->handleContentChanged();
    findBar->handleFindPrev();
    findBar->handleFindNext();

    EXPECT_NE(findBar,nullptr);
    findBar->deleteLater();
    
}

//void slot_ifClearSearchWord();
//TEST_F(test_findbar, slot_ifClearSearchWord)
//{
//    FindBar *findBar = new FindBar();
//    findBar->slot_ifClearSearchWord();
//    
//}

//protected:
//void hideEvent(QHideEvent *event) override;
TEST_F(test_findbar, hideEvent)
{
    QHideEvent *event = new QHideEvent();
    FindBar *findBar = new FindBar();
    findBar->hideEvent(event);

    EXPECT_NE(event,nullptr);
    EXPECT_NE(findBar,nullptr);
    findBar->deleteLater();
    delete event;
    
}

//bool focusNextPrevChild(bool next) override;
TEST_F(test_findbar, focusNextPrevChild)
{
    FindBar *findBar = new FindBar();

    EXPECT_EQ(findBar->focusNextPrevChild(true),false);
    EXPECT_NE(findBar,nullptr);
    findBar->deleteLater();
    
}

//void keyPressEvent(QKeyEvent *e) override;
TEST_F(test_findbar, keyPressEvent)
{
    FindBar *findBar = new FindBar();

    QKeyEvent *e = new QKeyEvent(QEvent::KeyPress,Qt::Key_Tab,Qt::NoModifier);
    findBar->m_closeButton->setFocus();
    findBar->keyPressEvent(e);
    EXPECT_EQ(findBar->m_closeButton->hasFocus(),false);

    QKeyEvent *e1 = new QKeyEvent(QEvent::KeyPress,Qt::Key_Escape,Qt::NoModifier);
    findBar->keyPressEvent(e1);

    QKeyEvent *e2 = new QKeyEvent(QEvent::KeyPress,Qt::Key_Enter,Qt::NoModifier);
    findBar->m_findPrevButton->setFocus();
    findBar->m_findNextButton->setFocus();
    EXPECT_EQ(findBar->m_findPrevButton->hasFocus(),false);

    findBar->keyPressEvent(e2);

    

    EXPECT_NE(findBar,nullptr);


    findBar->deleteLater();
    delete e;  e = nullptr;
    delete e1; e1 = nullptr;
    delete e2; e2 = nullptr;
}

// void handleSwitchToReplace();
TEST_F(test_findbar, handleSwitchToReplace)
{
    FindBar *findBar = new FindBar();
    bool signalReceived = false;
    QObject::connect(findBar, &FindBar::sigSwitchToReplaceBar, [&]() { signalReceived = true; });

    EXPECT_NO_FATAL_FAILURE(findBar->handleSwitchToReplace());
    EXPECT_EQ(signalReceived, true);

    findBar->deleteLater();
}

// QString getCurrentSearchText() const;
TEST_F(test_findbar, getCurrentSearchText)
{
    FindBar *findBar = new FindBar();
    EXPECT_EQ(findBar->getCurrentSearchText(), QString(""));

    findBar->m_editLine->lineEdit()->setText("searchkeyword");
    EXPECT_EQ(findBar->getCurrentSearchText(), QString("searchkeyword"));

    findBar->deleteLater();
}

//slotUpdateMatchCount 转发到内部 m_editLine
TEST_F(test_findbar, slotUpdateMatchCount_Forward)
{
    FindBar *findBar = new FindBar();
    findBar->show();
    qApp->processEvents();
    findBar->m_editLine->m_matchCountLabel->hide();

    findBar->slotUpdateMatchCount(3, 7);

    EXPECT_EQ(findBar->m_editLine->m_matchCountLabel->text().toStdString(), "第3/7项");
    EXPECT_TRUE(findBar->m_editLine->m_matchCountLabel->isVisible());

    findBar->deleteLater();
}

//slotUpdateMatchCount (0,0) 隐藏
TEST_F(test_findbar, slotUpdateMatchCount_ZeroHide)
{
    FindBar *findBar = new FindBar();
    findBar->show();
    qApp->processEvents();
    findBar->slotUpdateMatchCount(3, 7);

    findBar->slotUpdateMatchCount(0, 0);

    EXPECT_FALSE(findBar->m_editLine->m_matchCountLabel->isVisible());

    findBar->deleteLater();
}
