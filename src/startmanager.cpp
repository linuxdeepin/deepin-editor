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
    // qDebug() << "Open files in window: " << files;
    foreach (QString file, files) {
        QList<int> fileIndexes = fileIsOpened(file);
        if (fileIndexes.size() > 0) {
            qDebug() << QString("Open file %1 in window %2 with tab %3").arg(file).arg(fileIndexes[0]).arg(fileIndexes[1]);
        } else {
            Window *window = new Window();

            QScreen *screen = QGuiApplication::primaryScreen();
            QRect  screenGeometry = screen->geometry();
            window->setMinimumSize(QSize(screenGeometry.width() * 2 / 3, screenGeometry.height() * 2 / 3));
            Dtk::Widget::moveToCenter(window);
            window->show();

            window->addTab(file);

            windows << window;
        }
    }
}

void StartManager::openFilesInTab(QStringList files)
{
    // qDebug() << "Open files in tab: " << files;
    foreach (QString file, files) {
        QList<int> fileIndexes = fileIsOpened(file);
        if (fileIndexes.size() > 0) {
            qDebug() << QString("Open file %1 in window %2 with tab %3").arg(file).arg(fileIndexes[0]).arg(fileIndexes[1]);
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

QList<int> StartManager::fileIsOpened(QString file)
{
    QList<int> fileIndexes;
    foreach (Window *window, windows) {
        int tabIndex = window->fileIsInTabs(file);
        if (tabIndex >= 0) {
            fileIndexes << windows.indexOf(window) << tabIndex;
            return fileIndexes;
        }
    }

    return fileIndexes;
}
