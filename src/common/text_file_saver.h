// SPDX-FileCopyrightText: 2025 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

#ifndef TEXT_FILE_SAVER_H
#define TEXT_FILE_SAVER_H

#include <QString>
#include <QByteArray>
#include <QFile>

class QTextDocument;

class TextFileSaver
{
public:
    explicit TextFileSaver(QTextDocument *document);
    ~TextFileSaver();

    void setFilePath(const QString &filePath);
    void setEncoding(const QByteArray &toEncode);
    bool save();
    bool saveAs(const QString &newFilePath);
    QString errorString() const;

private:
    bool saveToFile(QFileDevice &file);
    bool convertEncoding(QByteArray &input, QByteArray &output) const;

private:
    QTextDocument *m_document;
    QString m_filePath;
    QByteArray m_fromEncode; // default is UTF-16 (QString)
    QByteArray m_toEncode;   // default is UTF-8
    QString m_errorString;
    static const int MAX_FILENAME_LENGTH = 245;
};

#endif // TEXT_FILE_SAVER_H
