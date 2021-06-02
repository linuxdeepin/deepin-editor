#include "test_fontitemdelegata.h"
#include "../src/controls/fontitemdelegate.h"
#include <QPainter>
test_fontitemdelegata::test_fontitemdelegata()
{

}

TEST_F(test_fontitemdelegata, FontItemDelegate)
{
    FontItemDelegate* delegate = new FontItemDelegate();
    delete delegate;
}


TEST_F(test_fontitemdelegata, paint)
{
    FontItemDelegate* delegate = new FontItemDelegate();
    QPainter* painter = new QPainter;
    QStyleOptionViewItem option;
    QModelIndex index;
    option.state = QStyle::State_Selected;
    delegate->paint(painter,option,index);

}

TEST_F(test_fontitemdelegata, sizeHint)
{
    FontItemDelegate* delegate = new FontItemDelegate();
    QPainter* painter = new QPainter;
    QStyleOptionViewItem option;
    QModelIndex index;
    option.state = QStyle::State_Selected;
    delegate->sizeHint(option,index);

}
