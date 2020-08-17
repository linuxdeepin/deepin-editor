#include "ColorSelectWdg.h"
#include <QPainter>
#include "../utils.h"
#include <QHBoxLayout>
#include <QVBoxLayout>
#include "settings.h"
#include <DSettingsOption>
#include <DFontSizeManager>

ColorLabel::ColorLabel(QColor color,QWidget *parent) : DWidget(parent),
    m_color(color)
{
    setMouseTracking(true);
}

void ColorLabel::setColorSelected(bool bSelect)
{
    if(m_bSelected == bSelect) return;

    m_bSelected = bSelect;

    update();
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
    painter.setRenderHints(QPainter::Antialiasing |QPainter::SmoothPixmapTransform | QPainter::Qt4CompatiblePainting);


    QPainterPath bigCircle;
    bigCircle.addEllipse(r);

    QPainterPath smallCircle;
    smallCircle.addEllipse(r.adjusted(2*distance,2*distance,-2*distance,-2*distance));

    //先画小圆
    painter.fillPath(smallCircle,m_color);


    //如果点击选择画　圆环
    if(m_bSelected){
        QRect r = rect();
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
    //没有选择点击有效
    if(e->button() == Qt::LeftButton && !m_bSelected){
       m_bSelected = true;
       update();
       emit sigColorClicked(m_bSelected,m_color);
    }
}

ColorSelectWdg::ColorSelectWdg(QString text,QWidget *parent):DWidget (parent),m_text(text)
{
    setFocusPolicy(Qt::NoFocus);
    setMouseTracking(true);
    if(!text.isEmpty())setFixedHeight(60);
    else setFixedHeight(35);
    initWidget();
}

void ColorSelectWdg::initWidget()
{
    QVBoxLayout *pMainLayout = new QVBoxLayout();
    QHBoxLayout *pHLayout1 = new QHBoxLayout();
    QHBoxLayout *pHLayout2 = new QHBoxLayout();

    if(!m_text.isEmpty()){
        m_pButton = new DPushButton(m_text,this);
        m_pButton->setMinimumSize(80,25);
        m_pButton->setFlat(true);
        connect(m_pButton,&QPushButton::clicked,this,[this](){
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
            m_defaultColor = colors[i];
            colorlabel->setColorSelected(true);
        }

        pHLayout2->addWidget(colorlabel);
        m_colorLabels.append(colorlabel);

        connect(colorlabel,&ColorLabel::sigColorClicked,this,[this,colorlabel](bool bSelect,QColor color){
            if(bSelect){
                foreach(ColorLabel* pLabel,m_colorLabels){
                    //如果选择　设置其他颜色label为未选中状态
                    if(pLabel != colorlabel && pLabel->isSelected()) pLabel->setColorSelected(false);
                }
                m_defaultColor = color;
                //发送选择信号
                emit this->sigColorSelected(bSelect,color);
            }
        });
    }

    if(!m_text.isEmpty()){
        pHLayout1->addWidget(m_pButton);
        pHLayout1->addSpacerItem(new QSpacerItem(100,25,QSizePolicy::Expanding,QSizePolicy::Preferred));

        pHLayout1->setContentsMargins(20,1,0,0);
        pHLayout2->setContentsMargins(5,2,5,2);

        pMainLayout->addLayout(pHLayout1);
        pMainLayout->addLayout(pHLayout2);
        pMainLayout->setContentsMargins(0,0,0,0);
        this->setLayout(pMainLayout);
    }else {
        this->setLayout(pHLayout2);
    }
}


void ColorSelectWdg::setTheme(const QString &theme)
{

    //获取主题颜色
    if(theme == "light") {
        m_textColor = "#1f1c1b";
    }else if(theme == "dark"){
        m_textColor = "#cfcfc2";
    }
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
                return true;
            }
        }
        return false;
    }

    return DWidget::eventFilter(object,event);
}
