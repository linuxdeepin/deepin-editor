// SPDX-FileCopyrightText: 2019 - 2022 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include "ColorSelectWdg.h"
#include "../common/utils.h"
#include "../common/settings.h"
#include <QPainter>
#include <DSettingsOption>
#include <DFontSizeManager>

ColorLabel::ColorLabel(QColor color,QWidget *parent) : DWidget(parent),
    m_color(color)
{
    qDebug() << "ColorLabel constructor";
    setMouseTracking(true);
}

void ColorLabel::setColorSelected(bool bSelect)
{
    qDebug() << "Setting color selected state:" << bSelect;
    if(m_bSelected == bSelect) {
        qDebug() << "Color selected state is the same as current state, no need to update";
        return;
    }

    m_bSelected = bSelect;

    update();
    qDebug() << "Color selected state updated to:" << m_bSelected;
}

bool ColorLabel::isSelected()
{
    return m_bSelected;
}

QColor ColorLabel::getColor()
{
    return m_color;
}

void ColorLabel::paintEvent(QPaintEvent *event)
{
    Q_UNUSED(event)

    int distance = 2;

    QRect r = rect();

    QPainter painter(this);
    painter.setRenderHints(QPainter::Antialiasing |QPainter::SmoothPixmapTransform
#if (QT_VERSION < QT_VERSION_CHECK(6, 0, 0))
     | QPainter::Qt4CompatiblePainting
#endif
    );


    QPainterPath bigCircle;
    bigCircle.addEllipse(r);

    QPainterPath smallCircle;
    smallCircle.addEllipse(r.adjusted(2*distance,2*distance,-2*distance,-2*distance));

    //先画小圆
    painter.fillPath(smallCircle,m_color);


    //如果点击选择画　圆环
    if(m_bSelected){
        r = rect();
        QPainterPath sencondCircle;
        sencondCircle.addEllipse(r.adjusted(distance,distance,-distance,-distance));
        //大圆减小圆等于圆环
        QPainterPath path = bigCircle - sencondCircle;
        painter.fillPath(path,m_color);
    }

    painter.end();
}

void ColorLabel::mousePressEvent(QMouseEvent *e)
{
    qDebug() << "ColorLabel mouse press event";
    //没有选择点击有效
    if(e->button() == Qt::LeftButton){
       m_bSelected = true;
       update();
       emit sigColorClicked(m_bSelected,m_color);
       qDebug() << "ColorLabel mouse press event, color selected state updated to:" << m_bSelected;
    }
    qDebug() << "ColorLabel mouse press event end";
}

ColorSelectWdg::ColorSelectWdg(QString text,QWidget *parent):DWidget (parent),m_text(text)
{
    qDebug() << "ColorSelectWdg constructor with text:" << text;
    setFocusPolicy(Qt::NoFocus);
    setMouseTracking(true);
    if(!text.isEmpty())setFixedHeight(60);
    else setFixedHeight(35);
    initWidget();
    qDebug() << "ColorSelectWdg constructor end";
}

ColorSelectWdg::~ColorSelectWdg()
{
    qDebug() << "ColorSelectWdg destructor";
    if (m_pHLayout2 != nullptr) {
        qDebug() << "ColorSelectWdg destructor, deleting m_pHLayout2";
        delete m_pHLayout2;
        m_pHLayout2=nullptr;
    }

    if (m_pHLayout1 != nullptr) {
        qDebug() << "ColorSelectWdg destructor, deleting m_pHLayout1";
        delete m_pHLayout1;
        m_pHLayout1=nullptr;
    }

    if (m_pMainLayout != nullptr) {
        qDebug() << "ColorSelectWdg destructor, deleting m_pMainLayout";
        delete m_pMainLayout;
        m_pMainLayout=nullptr;
    }

    qDebug() << "ColorSelectWdg destructor end";
}

void ColorSelectWdg::initWidget()
{
    qDebug() << "ColorSelectWdg initWidget";
    if(m_text.isEmpty()){
        qDebug() << "ColorSelectWdg initWidget, text is empty, using default layout";
        m_pHLayout2 = new QHBoxLayout(this);
    }
    else {
        qDebug() << "ColorSelectWdg initWidget, text is not empty, using custom layout";
        m_pMainLayout = new QVBoxLayout(this);
        m_pHLayout2 = new QHBoxLayout;
    }

    if(!m_text.isEmpty()){
        qDebug() << "ColorSelectWdg initWidget, text is not empty, creating button";
        m_pButton = new DPushButton(m_text,this);
        m_pButton->setMinimumSize(80,25);
        m_pButton->setFlat(true);
        connect(m_pButton,&QPushButton::clicked,this,[this](){
            qInfo() << "Color select button clicked, default color:" << m_defaultColor.name();
            //发送选择信号
            emit this->sigColorSelected(true,m_defaultColor);
        });
    }

    QList<QColor> colors = Utils::getHiglightColorList();
    for (int i = 0;i<colors.size();i++) {
        ColorLabel* colorlabel = new ColorLabel(colors[i],this);
        colorlabel->setFixedSize(m_labelWidth,m_labelHeight);

        //第一个设置默认标记颜色
        if(i == 0){
            qDebug() << "ColorSelectWdg initWidget, setting default color to:" << colors[i].name();
            m_defaultColor = colors[i];
            qDebug() << "Setting default color to:" << m_defaultColor.name();
            colorlabel->setColorSelected(true);
        }

        m_pHLayout2->addWidget(colorlabel);
        m_colorLabels.append(colorlabel);

        connect(colorlabel,&ColorLabel::sigColorClicked,this,[this,colorlabel](bool bSelect,QColor color){
            if(bSelect){
                qDebug() << "Color label clicked, selected color:" << color.name();
                foreach(ColorLabel* pLabel,m_colorLabels){
                    //如果选择　设置其他颜色label为未选中状态
                    if(pLabel != colorlabel && pLabel->isSelected()) {
                        qDebug() << "Deselecting color label:" << pLabel->getColor().name();
                        pLabel->setColorSelected(false);
                    }
                }
                m_defaultColor = color;
                //发送选择信号
                emit this->sigColorSelected(bSelect,color);
            }
        });
    }

    if(!m_text.isEmpty()){
        qDebug() << "ColorSelectWdg initWidget, text is not empty, creating button layout";
        m_pHLayout1 = new QHBoxLayout;
        m_pHLayout1->addWidget(m_pButton);
        m_pHLayout1->addSpacerItem(new QSpacerItem(100,25,QSizePolicy::Expanding,QSizePolicy::Preferred));

        m_pHLayout1->setContentsMargins(20,1,0,0);
        m_pHLayout2->setContentsMargins(5,2,5,2);

        m_pMainLayout->addLayout(m_pHLayout1);
        m_pMainLayout->addLayout(m_pHLayout2);
        m_pMainLayout->setContentsMargins(0,0,0,0);
        //this->setLayout(m_pMainLayout);
    }else {
        qDebug() << "ColorSelectWdg initWidget, text is empty, setting layout margins";
        m_pHLayout2->setContentsMargins(8+m_labelWidth,0,8+m_labelWidth,0);
        //this->setLayout(m_pHLayout2);
    }
    qDebug() << "ColorSelectWdg initWidget end";
}


void ColorSelectWdg::setTheme(const QString &theme)
{
    qDebug() << "ColorSelectWdg setTheme, theme:" << theme;
    //获取主题颜色
    if(theme == "light") {
        m_textColor = "#1f1c1b";
        qDebug() << "ColorSelectWdg setTheme, theme is light";
    }else if(theme == "dark"){
        m_textColor = "#cfcfc2";
        qDebug() << "ColorSelectWdg setTheme, theme is dark";
    }
    qDebug() << "ColorSelectWdg setTheme, theme end";
}

QColor ColorSelectWdg::getDefaultColor()
{
    return m_defaultColor;
}

bool ColorSelectWdg::eventFilter(QObject *object, QEvent *event)
{
    if(object == m_pLabel){
        if(event->type() == QEvent::MouseButtonPress){
            QMouseEvent *mouseEvent = static_cast<QMouseEvent*>(event);
            if(mouseEvent->button() == Qt::LeftButton){
                //发送选择信号
                emit this->sigColorSelected(true,m_defaultColor);
                qDebug() << "ColorSelectWdg eventFilter, left button pressed, sending sigColorSelected signal";
                return true;
            }
        }
        qDebug() << "ColorSelectWdg eventFilter, event not handled";
        return false;
    }

    return DWidget::eventFilter(object,event);
}
