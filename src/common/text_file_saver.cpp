// SPDX-FileCopyrightText: 2025 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

/**
 * @file file_saver.cpp
 * @brief Implementation of FileSaver class for safe file saving with encoding conversion
 */

#include "text_file_saver.h"
#include "utils.h"
#include "../encodes/detectcode.h"

#include <QSaveFile>
#include <QApplication>
#include <QFileInfo>
#include <QTextCodec>
#include <QDebug>
#include <QIODevice>
#include <QTextDocument>
#include <QObject>

/**
 * @brief Constructs a TextFileSaver with the given text document
 * @param document The QTextDocument to be saved
 */
TextFileSaver::TextFileSaver(QTextDocument *document)
    : m_document(document)
    , m_fromEncode("UTF-16")
    , m_toEncode("UTF-8")
{
    qDebug() << "TextFileSaver created for document with" << document->characterCount() << "characters";
}

TextFileSaver::~TextFileSaver()
{
    qDebug() << "TextFileSaver destructor entry";
}

/**
 * @brief Sets the target file path for saving
 * @param filePath The full path of the file to save to
 */
void TextFileSaver::setFilePath(const QString &filePath)
{
    qDebug() << "Setting file path to:" << filePath;
    m_filePath = filePath;
}

/**
 * @brief Sets the target encoding for the saved file
 * @param toEncode The target encoding for the saved file
 */
void TextFileSaver::setEncoding(const QByteArray &toEncode)
{
    qDebug() << "Setting target encoding to:" << toEncode;
    m_toEncode = toEncode;
}

/**
 * @brief Saves the document to the preset file path
 * @return true if the file was saved successfully, false otherwise
 * @note Uses QSaveFile for atomic writes when filename is not too long
 */
bool TextFileSaver::save()
{
    qDebug() << "Entering save";
    if (m_filePath.isEmpty()) {
        m_errorString = QObject::tr("File path is empty");
        qWarning() << "Cannot save - file path is empty";
        return false;
    }

    // TODO: 暂时禁用QSaveFile，后续再考虑, 因为QSaveFile在某些情况下会修改文件所属权限
    // WARNING: For long filenames (>245 chars), QSaveFile may create temporary files
    // with names that exceed system limits. TextFileSaver handles this internally.
    // QFileInfo fileInfo(m_filePath);
    // bool disableSaveProtect = fileInfo.fileName().length() > MAX_FILENAME_LENGTH;

    // if (!disableSaveProtect) {
    //     QSaveFile saveFile(m_filePath);
    //     saveFile.setDirectWriteFallback(true);
    //     if (!saveToFile(saveFile)) {
    //         return false;
    //     }
    //     return saveFile.commit();
    // } else {
    //     qWarning() << "File name too long, disable QSaveFile. path:" << m_filePath;
    QFile file(m_filePath);
    if (!saveToFile(file)) {
        return false;
    }
    return true;
    // }
}

/**
 * @brief Saves the document to a new file path
 * @param newFilePath The target path to save the file to
 * @return true if the file was saved successfully, false otherwise
 * @note Restores original file path if save fails
 */
bool TextFileSaver::saveAs(const QString &newFilePath)
{
    qDebug() << "Saving as new file:" << newFilePath;
    QString oldPath = m_filePath;
    m_filePath = newFilePath;
    bool result = save();
    if (!result) {
        qWarning() << "SaveAs failed, reverting to original path:" << oldPath;
        m_filePath = oldPath;
    } else {
        qDebug() << "SaveAs completed successfully";
    }
    return result;
}

/**
 * @brief Gets the last error message
 * @return The description of the last error that occurred
 */
QString TextFileSaver::errorString() const
{
    qDebug() << "errorString" << m_errorString;
    return m_errorString;
}

/**
 * @brief Internal implementation of file writing
 * @param file The QFileDevice to write to
 * @return true if the write was successful, false otherwise
 */
bool TextFileSaver::saveToFile(QFileDevice &file)
{
    qDebug() << "Entering saveToFile";
    try {
        qDebug() << "Attempting to open file for writing:" << m_filePath;
        if (!file.open(QIODevice::WriteOnly | QIODevice::Truncate)) {
            m_errorString = file.errorString();
            qWarning() << "Failed to open file:" << m_errorString;
            return false;
        }
        qDebug() << "file opened";
        auto characterCount = m_document->characterCount();
        // Check memory for document content (QChar is 2 bytes)
        qlonglong docMemoryNeeded = characterCount * 2;
        if (!Utils::isMemorySufficientForOperation(Utils::OperationType::RawOperation, docMemoryNeeded, characterCount)) {
            m_errorString = QObject::tr("Insufficient memory to load document content");
            qWarning() << m_errorString << "- needed:" << docMemoryNeeded << "bytes";
            return false;
        }
        qDebug() << "memory sufficient";
        const QString content = m_document->toPlainText();
        const ushort *data = content.utf16();
        const int length = content.length();

        // Dynamically calculate chunk size (10MB or 1/8 of total length, whichever is larger)
        const int chunkSize = qMax(10 * 1024 * 1024, length / 8);
        qDebug() << "chunkSize" << chunkSize;
        qDebug() << "Saving document in chunks, total size:" << length << "chars, chunk size:" << chunkSize;
        for (int i = 0; i < length; i += chunkSize) {
            int currentChunkSize = qMin(chunkSize, length - i);
            qDebug() << "Processing chunk" << i << "-" << i+currentChunkSize-1 << "of" << length;
            QByteArray input(reinterpret_cast<const char *>(data + i), currentChunkSize * sizeof(ushort));
            qDebug() << "input" << input;
            // Check memory for encoding conversion (input + estimated output size)
            qlonglong conversionMemoryNeeded = input.size() * 2;  // Estimate 2x for worst case
            if (!Utils::isMemorySufficientForOperation(
                    Utils::OperationType::RawOperation, conversionMemoryNeeded, characterCount)) {
                m_errorString = QObject::tr("Insufficient memory for encoding conversion");
                return false;
            }
            qDebug() << "memory sufficient";
            QByteArray outData;
            if (!convertEncoding(input, outData)) {
                m_errorString = QObject::tr("Encoding conversion failed");
                qWarning() << m_errorString << "from" << m_fromEncode << "to" << m_toEncode;
                return false;
            }
            qDebug() << "outData" << outData;
            if (outData.isEmpty()) {
                m_errorString = QObject::tr("Converted content is empty");
                return false;
            }
            qDebug() << "outData is not empty";
            QApplication::processEvents();
            qDebug() << "processEvents";
            qint64 written = file.write(outData);
            if (written != outData.size()) {
                m_errorString = file.errorString();
                qWarning() << "Failed to write full chunk. Expected:" << outData.size()
                             << "bytes, wrote:" << written << "bytes. Error:" << m_errorString;
                return false;
            }
        }

        qDebug() << "File saved successfully:" << m_filePath;
        return true;
    } catch (const std::bad_alloc &) {
        m_errorString = QObject::tr("Memory allocation failed");
        qDebug() << "Memory allocation failed";
        return false;
    } catch (const std::exception &e) {
        m_errorString = QObject::tr("Error occurred: %1").arg(e.what());
        qDebug() << "Error occurred: " << e.what();
        return false;
    } catch (...) {
        m_errorString = QObject::tr("Unknown error occurred");
        qDebug() << "Unknown error occurred";
        return false;
    }
    qDebug() << "saveToFile failed";
}

/**
 * @brief Converts between character encodings
 * @param input The input byte array to convert
 * @param output The converted output byte array
 * @return true if conversion was successful, false otherwise
 * @note Uses DetectCode first, falls back to QTextCodec if needed
 */
bool TextFileSaver::convertEncoding(QByteArray &input, QByteArray &output) const
{
    qDebug() << "Entering convertEncoding";
    if (m_fromEncode == m_toEncode) {
        qDebug() << "No encoding conversion needed (same encoding)";
        output = input;
        return true;
    }

    qDebug() << "Converting encoding from" << m_fromEncode << "to" << m_toEncode;
    if (!DetectCode::ChangeFileEncodingFormat(input, output, m_fromEncode, m_toEncode)) {
        qWarning() << "Encoding conversion failed using DetectCode";
        return false;
    }
    qDebug() << "convertEncoding success";
    return true;
}
