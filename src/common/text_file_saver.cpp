// SPDX-FileCopyrightText: 2025 - 2026 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

/**
 * @file file_saver.cpp
 * @brief Implementation of FileSaver class for safe file saving with encoding conversion
 */

#include "ddlog.h"
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
    qCDebug(appLog) << "TextFileSaver created for document with" << document->characterCount() << "characters";
}

TextFileSaver::~TextFileSaver()
{
    qCDebug(appLog) << "TextFileSaver destructor entry";
}

/**
 * @brief Sets the target file path for saving
 * @param filePath The full path of the file to save to
 */
void TextFileSaver::setFilePath(const QString &filePath)
{
    qCDebug(appLog) << "Setting file path to:" << filePath;
    m_filePath = filePath;
}

/**
 * @brief Sets the target encoding for the saved file
 * @param toEncode The target encoding for the saved file
 */
void TextFileSaver::setEncoding(const QByteArray &toEncode)
{
    qCDebug(appLog) << "Setting target encoding to:" << toEncode;
    m_toEncode = toEncode;
}

void TextFileSaver::setEndlineFormat(bool useCRLF)
{
    m_useCRLF = useCRLF;
}

/**
 * @brief Saves the document to the preset file path
 * @return true if the file was saved successfully, false otherwise
 * @note Uses QSaveFile for atomic writes when filename is not too long
 */
bool TextFileSaver::save()
{
    qCDebug(appLog) << "Entering save";
    if (m_filePath.isEmpty()) {
        m_errorString = QObject::tr("File path is empty");
        qCWarning(appLog) << "Cannot save - file path is empty";
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
    //     qCWarning(appLog) << "File name too long, disable QSaveFile. path:" << m_filePath;
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
    qCDebug(appLog) << "Saving as new file:" << newFilePath;
    QString oldPath = m_filePath;
    m_filePath = newFilePath;
    bool result = save();
    if (!result) {
        qCWarning(appLog) << "SaveAs failed, reverting to original path:" << oldPath;
        m_filePath = oldPath;
    } else {
        qCDebug(appLog) << "SaveAs completed successfully";
    }
    return result;
}

/**
 * @brief Gets the last error message
 * @return The description of the last error that occurred
 */
QString TextFileSaver::errorString() const
{
    qCDebug(appLog) << "errorString" << m_errorString;
    return m_errorString;
}

/**
 * @brief Internal implementation of file writing
 * @param file The QFileDevice to write to
 * @return true if the write was successful, false otherwise
 */
bool TextFileSaver::saveToFile(QFileDevice &file)
{
    qCDebug(appLog) << "Entering saveToFile";
    try {
        // 先完成前置校验（内存检查等），再打开/截断目标文件，
        // 避免校验失败时目标文件已被 Truncate 清空
        auto characterCount = m_document->characterCount();
        // Check memory for document content (QChar is 2 bytes)
        qlonglong docMemoryNeeded = characterCount * 2;
        if (!Utils::isMemorySufficientForOperation(Utils::OperationType::RawOperation, docMemoryNeeded, characterCount)) {
            m_errorString = QObject::tr("Insufficient memory to load document content");
            qCWarning(appLog) << m_errorString << "- needed:" << docMemoryNeeded << "bytes";
            return false;
        }

        qCDebug(appLog) << "Attempting to open file for writing:" << m_filePath;
        if (!file.open(QIODevice::WriteOnly | QIODevice::Truncate)) {
            m_errorString = file.errorString();
            qCWarning(appLog) << "Failed to open file:" << m_errorString;
            return false;
        }
        qCDebug(appLog) << "file opened";
        qCDebug(appLog) << "memory sufficient";
        QString content = m_document->toPlainText();
        if (m_useCRLF) {
            content.replace(QStringLiteral("\n"), QStringLiteral("\r\n"));
        }
        const ushort *data = content.utf16();
        const int length = content.length();

        // Dynamically calculate chunk size (10MB or 1/8 of total length, whichever is larger)
        const int chunkSize = qMax(10 * 1024 * 1024, length / 8);
        qCDebug(appLog) << "chunkSize" << chunkSize;
        qCDebug(appLog) << "Saving document in chunks, total size:" << length << "chars, chunk size:" << chunkSize;
        for (int i = 0; i < length; i += chunkSize) {
            int currentChunkSize = qMin(chunkSize, length - i);
            qCDebug(appLog) << "Processing chunk" << i << "-" << i+currentChunkSize-1 << "of" << length;
            QByteArray input(reinterpret_cast<const char *>(data + i), currentChunkSize * sizeof(ushort));
            qCDebug(appLog) << "input" << input;
            // Check memory for encoding conversion (input + estimated output size)
            qlonglong conversionMemoryNeeded = input.size() * 2;  // Estimate 2x for worst case
            if (!Utils::isMemorySufficientForOperation(
                    Utils::OperationType::RawOperation, conversionMemoryNeeded, characterCount)) {
                m_errorString = QObject::tr("Insufficient memory for encoding conversion");
                return false;
            }
            qCDebug(appLog) << "memory sufficient";
            QByteArray outData;
            if (!convertEncoding(input, outData)) {
                m_errorString = QObject::tr("Encoding conversion failed");
                qCWarning(appLog) << m_errorString << "from" << m_fromEncode << "to" << m_toEncode;
                return false;
            }
            qCDebug(appLog) << "outData" << outData;
            if (outData.isEmpty()) {
                m_errorString = QObject::tr("Converted content is empty");
                return false;
            }
            qCDebug(appLog) << "outData is not empty";
            QApplication::processEvents();
            qCDebug(appLog) << "processEvents";
            // processEvents() may deliver a queued QCloseEvent that deletes the
            // EditWrapper (and thus the QTextDocument) while the save loop is
            // still running.  Abort immediately when the document is gone or the
            // owner has requested quit, mirroring the guard pattern already used
            // in EditWrapper::loadContent().
            // Note: the condition order below is significant — m_quitFlag is a
            // raw pointer to EditWrapper::m_bQuit and may dangle after the owner
            // is destroyed.  The null check (m_quitFlag &&) MUST precede the
            // dereference (*m_quitFlag) so that C++ short-circuit evaluation
            // prevents dereferencing a dangling pointer.  Do not reorder.
            if (!m_document || (m_quitFlag && *m_quitFlag)) {
                m_errorString = QObject::tr("Save aborted: editor is closing");
                qCWarning(appLog) << "Save aborted: document destroyed or quit requested";
                file.close();
                return false;
            }
            qint64 written = file.write(outData);
            if (written != outData.size()) {
                m_errorString = file.errorString();
                qCWarning(appLog) << "Failed to write full chunk. Expected:" << outData.size()
                             << "bytes, wrote:" << written << "bytes. Error:" << m_errorString;
                return false;
            }
        }

        qCDebug(appLog) << "File saved successfully:" << m_filePath;
        // 成功保存后清空历史错误信息，保证 errorString() 不返回上次失败内容
        m_errorString.clear();
        return true;
    } catch (const std::bad_alloc &) {
        m_errorString = QObject::tr("Memory allocation failed");
        qCDebug(appLog) << "Memory allocation failed";
        return false;
    } catch (const std::exception &e) {
        m_errorString = QObject::tr("Error occurred: %1").arg(e.what());
        qCDebug(appLog) << "Error occurred: " << e.what();
        return false;
    } catch (...) {
        m_errorString = QObject::tr("Unknown error occurred");
        qCDebug(appLog) << "Unknown error occurred";
        return false;
    }
    qCDebug(appLog) << "saveToFile failed";
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
    qCDebug(appLog) << "Entering convertEncoding";
    if (m_fromEncode == m_toEncode) {
        qCDebug(appLog) << "No encoding conversion needed (same encoding)";
        output = input;
        return true;
    }

    qCDebug(appLog) << "Converting encoding from" << m_fromEncode << "to" << m_toEncode;
    if (!DetectCode::ChangeFileEncodingFormat(input, output, m_fromEncode, m_toEncode)) {
        qCWarning(appLog) << "Encoding conversion failed using DetectCode";
        return false;
    }
    qCDebug(appLog) << "convertEncoding success";
    return true;
}
