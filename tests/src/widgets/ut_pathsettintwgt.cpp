// SPDX-FileCopyrightText: 2026 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include "ut_pathsettintwgt.h"
#include "../stub.h"

#include <QFileDialog>
#include <QAbstractButton>

// Stub: QFileDialog::exec 返回 Rejected，触发对话框取消分支
static int stub_filedialog_exec_rejected()
{
    return QDialog::Rejected;
}

// 测试函数 PathSettingWdg::setEditText
TEST_F(test_pathsettintwgt, checkSetEditText)
{
    m_wgt->setEditText("/home/user/Documents/very/long/path/to/some/file/location/test.txt");
    EXPECT_FALSE(m_wgt->m_customEdit->text().isEmpty());
}

// 测试函数 PathSettingWdg::onBoxClicked
TEST_F(test_pathsettintwgt, checkOnBoxClicked)
{
    // CurFileBox: 自定义按钮禁用
    m_wgt->onBoxClicked(PathSettingWgt::CurFileBox);
    EXPECT_FALSE(m_wgt->m_customBtn->isEnabled());

    // LastOptBox: 自定义按钮禁用
    m_wgt->onBoxClicked(PathSettingWgt::LastOptBox);
    EXPECT_FALSE(m_wgt->m_customBtn->isEnabled());

    // CustomBox: 自定义按钮启用
    m_wgt->onBoxClicked(PathSettingWgt::CustomBox);
    EXPECT_TRUE(m_wgt->m_customBtn->isEnabled());

    // default 分支
    m_wgt->onBoxClicked(999);
    EXPECT_NE(m_wgt, nullptr);
}

// 测试函数 PathSettingWdg::onBtnClicked
TEST_F(test_pathsettintwgt, checkOnBtnClicked)
{
    typedef int (*Fptr)(QFileDialog *);
    Fptr fptr = (Fptr)(&QFileDialog::exec);
    Stub s;
    s.set(fptr, stub_filedialog_exec_rejected);
    // 对话框取消 -> 提前返回
    m_wgt->onBtnClicked();
    EXPECT_NE(m_wgt, nullptr);
}

// 测试 connections 内 lambda #1: QButtonGroup::buttonClicked(QAbstractButton*)
TEST_F(test_pathsettintwgt, checkConnectionsLambda)
{
    // 点击 m_customBox 触发 QButtonGroup 的 buttonClicked 信号，
    // 调用 connections() 中注册的 lambda，进而调用 onBoxClicked(CustomBox)
    m_wgt->m_customBox->click();
    EXPECT_TRUE(m_wgt->m_customBox->isChecked());
    EXPECT_TRUE(m_wgt->m_customBtn->isEnabled());
}
