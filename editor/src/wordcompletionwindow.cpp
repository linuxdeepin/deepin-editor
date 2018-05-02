#include "wordcompletionwindow.h"
#include "wordcompletionitem.h"

WordCompletionWindow::WordCompletionWindow(QWidget *parent) : QWidget(parent)
{
    setWindowFlags(Qt::FramelessWindowHint | Qt::X11BypassWindowManagerHint);
    
    lineHeight = 20;
    windowWidth = 150;
    windowHeight = 14;
    
    setFixedSize(windowWidth, windowHeight);
    
    listview = new DSimpleListView(this);
    listview->setFixedSize(windowWidth, windowHeight);
    listview->setRowHeight(lineHeight);
    
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
        windowHeight = 10 * lineHeight;
    } else {
        windowHeight = words.size() * lineHeight;
    }
    
    setFixedSize(windowWidth, windowHeight);
    listview->setFixedSize(windowWidth, windowHeight);
}
