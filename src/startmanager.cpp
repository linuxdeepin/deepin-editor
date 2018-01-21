#include "startmanager.h"
#include <DWidgetUtil>
#include <QScreen>
#include <DApplication>
#include <QDebug>

DWIDGET_USE_NAMESPACE

StartManager::StartManager(QObject *parent) : QObject(parent)
{
}

void StartManager::openFilesInWindow(QStringList files)
{
    if (files.size() == 0) {
        Window *window = new Window();

        QScreen *screen = QGuiApplication::primaryScreen();
        QRect  screenGeometry = screen->geometry();
        window->setMinimumSize(QSize(screenGeometry.width() * 2 / 3, screenGeometry.height() * 2 / 3));
        window->show();

        if (windows.size() == 0) {
            Dtk::Widget::moveToCenter(window);
        } else {
            int windowOffset = 50;
            window->move(windows.size() * windowOffset, windows.size() * windowOffset);
        }

        window->addBlankTab();

        windows << window;
        
        window->activateWindow();
    } else {
        foreach (QString file, files) {
            QList<int> fileIndexes = fileIsOpened(file);
            if (fileIndexes.size() > 0) {
                int windowIndex = fileIndexes[0];
                int tabIndex = fileIndexes[1];

                windows[windowIndex]->activeTab(tabIndex);
            } else {
                Window *window = new Window();

                QScreen *screen = QGuiApplication::primaryScreen();
                QRect  screenGeometry = screen->geometry();
                window->setMinimumSize(QSize(screenGeometry.width() * 2 / 3, screenGeometry.height() * 2 / 3));
                window->show();

                if (windows.size() == 0) {
                    Dtk::Widget::moveToCenter(window);
                } else {
                    int windowOffset = 50;
                    window->move(windows.size() * windowOffset, windows.size() * windowOffset);
                }

                window->addTab(file);

                windows << window;
            }
        }
    }
}

void StartManager::openFilesInTab(QStringList files)
{
    if (files.size() == 0) {
        if (windows.size() == 0) {
            Window *window = new Window();

            QScreen *screen = QGuiApplication::primaryScreen();
            QRect  screenGeometry = screen->geometry();
            window->setMinimumSize(QSize(screenGeometry.width() * 2 / 3, screenGeometry.height() * 2 / 3));
            Dtk::Widget::moveToCenter(window);
            window->show();

            window->addBlankTab();

            windows << window;
        } else {
            windows[0]->activateWindow();
        }
    } else {
        foreach (QString file, files) {
            QList<int> fileIndexes = fileIsOpened(file);
            if (fileIndexes.size() > 0) {
                int windowIndex = fileIndexes[0];
                int tabIndex = fileIndexes[1];

                windows[windowIndex]->activeTab(tabIndex);
            } else {
                if (windows.size() == 0) {
                    Window *window = new Window();

                    QScreen *screen = QGuiApplication::primaryScreen();
                    QRect  screenGeometry = screen->geometry();
                    window->setMinimumSize(QSize(screenGeometry.width() * 2 / 3, screenGeometry.height() * 2 / 3));
                    Dtk::Widget::moveToCenter(window);
                    window->show();

                    window->addTab(file);

                    windows << window;
                } else {
                    windows[0]->addTab(file);
                }
            }
        }
    }
}

QList<int> StartManager::fileIsOpened(QString file)
{
    QList<int> fileIndexes;
    foreach (Window *window, windows) {
        int tabIndex = window->isFileInTabs(file);
        if (tabIndex >= 0) {
            fileIndexes << windows.indexOf(window) << tabIndex;
            return fileIndexes;
        }
    }

    return fileIndexes;
}
