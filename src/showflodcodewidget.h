#ifndef SHOWFLODCODEWIDGET_H
#define SHOWFLODCODEWIDGET_H
#include <DFrame>
#include <DPlainTextEdit>
#include <DApplicationHelper>
#include <KF5/KSyntaxHighlighting/repository.h>
#include <KF5/KSyntaxHighlighting/definition.h>
#include <KF5/KSyntaxHighlighting/syntaxhighlighter.h>
#include <KF5/KSyntaxHighlighting/theme.h>

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
    void initHighLight(QString filepath);

    /**
     * @author liumaochuan ut000616
     * @brief setStyle 设置样式
     * @param bIsLineWrap  是否换行
     */
    void setStyle(bool bIsLineWrap);

private:
    DPlainTextEdit *m_pContentEdit;
    int m_nTextWidth = 0;///< 代码预览框宽度
    KSyntaxHighlighting::Repository m_repository;
    KSyntaxHighlighting::SyntaxHighlighter *m_highlighter;
};

#endif // SHOWFLODCODEWIDGET_H
