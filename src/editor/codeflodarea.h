#ifndef CODEFLODAREA_H
#define CODEFLODAREA_H
#include <QWidget>
class LeftAreaTextEdit;


class CodeFlodArea: public QWidget
{
    Q_OBJECT
public:
    CodeFlodArea(LeftAreaTextEdit *leftAreaWidget);
    ~CodeFlodArea() override;
    void paintEvent(QPaintEvent *e) override;
private:
    LeftAreaTextEdit* m_pLeftAreaWidget = nullptr;
};

#endif // CODEFLODAREA_H
