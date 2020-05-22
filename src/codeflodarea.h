#ifndef CODEFLODAREA_H
#define CODEFLODAREA_H
#include <QWidget>
class leftareaoftextedit;
class TextEdit;
class CodeFlodArea: public QWidget
{
    Q_OBJECT
public:
    CodeFlodArea(leftareaoftextedit *leftAreaWidget);
    ~CodeFlodArea() override;

    void paintEvent(QPaintEvent *e) override;

private:
    leftareaoftextedit *m_leftAreaWidget;
};

#endif // CODEFLODAREA_H
