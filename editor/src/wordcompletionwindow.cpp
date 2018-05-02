#include "wordcompletionwindow.h"
#include "wordcompletionitem.h"

WordCompletionWindow::WordCompletionWindow(QWidget *parent) : QWidget(parent)
{
    setWindowFlags(Qt::FramelessWindowHint | Qt::X11BypassWindowManagerHint);
    setFixedSize(150, 200);
    
    listview = new DSimpleListView(this);
    listview->setFixedSize(150, 200);
    listview->setRowHeight(20);
    
    listview->setClipRadius(0);
}

WordCompletionWindow::~WordCompletionWindow()
{
}

void WordCompletionWindow::addWords(QStringList words)
{
    items.clear();
    for (auto word : words) {
        WordCompletionItem *item = new WordCompletionItem(word);
        items << item;
    }
    
    listview->refreshItems(items);
    listview->selectFirstItem();
    
    if (words.size() > 10) {
        setFixedSize(150, 200);
        listview->setFixedSize(150, 200);
    } else {
        setFixedSize(150, words.size() * 20);
        listview->setFixedSize(150, words.size() * 20);
    }
}
