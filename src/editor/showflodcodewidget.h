/*
* Copyright (C) 2019 ~ 2021 Uniontech Software Technology Co.,Ltd.
*
* Author:     liumaochuan <liumaochuan@uniontech.com>
*
* Maintainer: liumaochuan <liumaochuan@uniontech.com>
*
* This program is free software: you can redistribute it and/or modify
* it under the terms of the GNU General Public License as published by
* the Free Software Foundation, either version 3 of the License, or
* any later version.
*
* This program is distributed in the hope that it will be useful,
* but WITHOUT ANY WARRANTY; without even the implied warranty of
* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
* GNU General Public License for more details.
*
* You should have received a copy of the GNU General Public License
* along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/
#ifndef SHOWFLODCODEWIDGET_H
#define SHOWFLODCODEWIDGET_H
#include <DFrame>
#include <DPlainTextEdit>
#include <DApplicationHelper>
#include <KSyntaxHighlighting/repository.h>
#include <KSyntaxHighlighting/definition.h>
#include <KSyntaxHighlighting/syntaxhighlighter.h>
#include <KSyntaxHighlighting/theme.h>

DWIDGET_USE_NAMESPACE

class ShowFlodCodeWidget: public DFrame
{
    Q_OBJECT
public:
    explicit ShowFlodCodeWidget(DWidget *parent = nullptr);
    ~ShowFlodCodeWidget();

    /**
     * @author liumaochuan ut000616
     * @brief appendText 添加文本
     * @param strText 文本
     * @param maxWidth 当前窗口宽度
     */
    void appendText(QString strText, int maxWidth);

    void clear();

    /**
     * @author liumaochuan ut000616
     * @brief initHighLight 设置语法高亮
     * @param filepath 当前文件路径
     */
    void initHighLight(QString filepath, bool bIsLight);

    /**
     * @author liumaochuan ut000616
     * @brief setStyle 设置样式
     * @param bIsLineWrap  是否换行
     */
    void setStyle(bool bIsLineWrap);

    /**
     * @author liumaochuan ut000616
     * @brief hideFirstAndLastBlock 隐藏首末行
     */
    void hideFirstBlock();

private:
    DPlainTextEdit *m_pContentEdit;
    int m_nTextWidth = 0;///< 代码预览框宽度
    KSyntaxHighlighting::Repository m_repository;
    KSyntaxHighlighting::SyntaxHighlighter *m_highlighter;
};

#endif // SHOWFLODCODEWIDGET_H
