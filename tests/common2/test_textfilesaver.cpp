// SPDX-FileCopyrightText: 2026 UnionTech Software Technology Co., Ltd.
// SPDX-License-Identifier: GPL-3.0-or-later

/*
 * TextFileSaver 单元测试
 *
 * 分支清单（来源：TextFileSaver::save / saveToFile / convertEncoding / saveAs）
 * B1 : save() m_filePath 为空                → false + errorString "File path is empty"
 * B2 : saveToFile() QFile::open 失败(目录)    → false + errorString = 文件系统错误
 * B3 : 文档内存不足(第 1 次 memcheck false)   → "Insufficient memory to load document content"
 * B4 : 转换内存不足(第 2 次 memcheck false)   → "Insufficient memory for encoding conversion"
 * B5 : convertEncoding 失败(非法目标编码)     → "Encoding conversion failed"
 * B6 : file.write 返回值 != 数据长度          → false（写失败分支）
 * B7 : m_useCRLF = true                      → 内容 \n 替换为 \r\n
 * B8 : 分块循环 chunkSize(10MB) 多次迭代      → 全量写入
 * B9 : convertEncoding from==to(UTF-16)       → 原样写入原始 UTF-16 字节
 * B10: saveAs 成功                            → 新路径写入成功
 * B11: saveAs 失败                            → m_filePath 回退原路径
 * B12: 默认 UTF-8 保存（UTF-16→UTF-8 iconv）  → 文件字节 == toUtf8()
 *
 * 用例映射：
 * - Save_EmptyFilePath_ReturnsFalseWithError            → B1
 * - Save_Utf8Default_WritesExactBytes                   → B12
 * - Save_CrlfEnabled_ConvertsLineEndings                → B7
 * - Save_EmptyDocument_WritesEmptyFileSucceeds          → B12(空内容循环 0 次)
 * - Save_TargetIsDirectory_OpenFailsReturnsFalse        → B2
 * - Save_MemoryInsufficientDocument_ReturnsFalse        → B3
 * - Save_MemoryInsufficientConversion_ReturnsFalse      → B4
 * - Save_InvalidTargetEncoding_ConversionFails          → B5
 * - Save_SameEncodingUtf16_WritesRawUtf16Bytes          → B9
 * - Save_WriteShortCount_ReturnsFalse                   → B6
 * - SaveAs_NewPath_SucceedsAndWritesFile                → B10
 * - SaveAs_FailureRestoresOriginalPath                  → B11
 * - Save_LargeDocumentMultiChunk_WritesAllBytes         → B8
 *
 * 隐式依赖处理：
 * - Utils::isMemorySufficientForOperation：utils.cpp 为重 GUI 模块（DSettings/
 *   DMessageManager/DGuiApplicationHelper 等），与保存逻辑无关，不入编译目标；
 *   本文件提供本地链接定义并用 stub_ext 控制返回值（fixture 成员计数，无全局状态）。
 * - QApplication::processEvents：由 ensureApp() 的 QGuiApplication(offscreen) 承载。
 * - DetectCode::ChangeFileEncodingFormat：真实执行（iconv 行为已在本机验证确定性）。
 */

#include <gtest/gtest.h>
#include "stubext.h"

#include "text_file_saver.h"
#include "utils.h"

#include <QTextDocument>
#include <QTemporaryDir>
#include <QTemporaryFile>
#include <QFile>
#include <QGuiApplication>
#include <QPointer>

#include <memory>

// 本地链接定义：行为由 fixture 内 stub 控制（见 SetUp）
bool Utils::isMemorySufficientForOperation(Utils::OperationType, qlonglong, qlonglong)
{
    return true;
}

namespace {

QCoreApplication *ensureApp()
{
    if (!QCoreApplication::instance()) {
        if (qEnvironmentVariableIsEmpty("QT_QPA_PLATFORM"))
            qputenv("QT_QPA_PLATFORM", "offscreen");
        static int argc = 1;
        static char argv0[] = "test_common2";
        static char *argv[2] = { argv0, nullptr };
        static QGuiApplication app(argc, argv);
        return &app;
    }
    return QCoreApplication::instance();
}

} // namespace

class TextFileSaverTest : public ::testing::Test
{
protected:
    void SetUp() override
    {
        ensureApp();
        stub.clear();
        memResult = true;
        memCallCount = 0;
        writeCallCount = 0;
        doc = new QTextDocument;
        doc->setPlainText(QStringLiteral("hello 中文"));
        obj = new TextFileSaver(doc);
        tmpDir = std::make_unique<QTemporaryDir>();
        ASSERT_TRUE(tmpDir->isValid());
        installMemoryStub();
    }

    void TearDown() override
    {
        stub.clear();
        delete obj;
        obj = nullptr;
        delete doc;
        doc = nullptr;
    }

    void installMemoryStub()
    {
        stub.set_lamda(&Utils::isMemorySufficientForOperation,
                       [this](Utils::OperationType, qlonglong, qlonglong) -> bool {
                           ++memCallCount;
                           return memResult;
                       });
    }

    QString filePath(const QString &name) const { return tmpDir->path() + QLatin1Char('/') + name; }

    stub_ext::StubExt stub;
    QTextDocument *doc = nullptr;
    TextFileSaver *obj = nullptr;
    std::unique_ptr<QTemporaryDir> tmpDir;
    bool memResult = true;
    int memCallCount = 0;
    int writeCallCount = 0;
};

// B1
TEST_F(TextFileSaverTest, Save_EmptyFilePath_ReturnsFalseWithError)
{
    // Arrange: 不设置文件路径（保持为空）
    // Act
    const bool ret = obj->save();
    // Assert: 返回失败且错误信息为未翻译源文（未加载翻译时 tr 原样返回）
    EXPECT_FALSE(ret);
    EXPECT_EQ(obj->errorString(), QStringLiteral("File path is empty"));
}

// B12
TEST_F(TextFileSaverTest, Save_Utf8Default_WritesExactBytes)
{
    // Arrange
    doc->setPlainText(QStringLiteral("hello 中文\n"));
    obj->setFilePath(filePath(QStringLiteral("utf8.txt")));
    // Act
    const bool ret = obj->save();
    // Assert
    EXPECT_TRUE(ret);
    EXPECT_TRUE(obj->errorString().isEmpty());
    QFile f(filePath(QStringLiteral("utf8.txt")));
    ASSERT_TRUE(f.open(QIODevice::ReadOnly));
    EXPECT_EQ(f.readAll(), QStringLiteral("hello 中文\n").toUtf8());
    f.close();
}

// B7
TEST_F(TextFileSaverTest, Save_CrlfEnabled_ConvertsLineEndings)
{
    // Arrange
    doc->setPlainText(QStringLiteral("a\nb\nc"));
    obj->setEndlineFormat(true);
    obj->setFilePath(filePath(QStringLiteral("crlf.txt")));
    // Act
    const bool ret = obj->save();
    // Assert
    EXPECT_TRUE(ret);
    QFile f(filePath(QStringLiteral("crlf.txt")));
    ASSERT_TRUE(f.open(QIODevice::ReadOnly));
    EXPECT_EQ(f.readAll(), QByteArrayLiteral("a\r\nb\r\nc"));
    f.close();
}

// B12（空内容：分块循环 0 次迭代，仍成功创建空文件）
TEST_F(TextFileSaverTest, Save_EmptyDocument_WritesEmptyFileSucceeds)
{
    // Arrange
    doc->setPlainText(QString());
    obj->setFilePath(filePath(QStringLiteral("empty.txt")));
    // Act
    const bool ret = obj->save();
    // Assert
    EXPECT_TRUE(ret);
    QFile f(filePath(QStringLiteral("empty.txt")));
    ASSERT_TRUE(f.open(QIODevice::ReadOnly));
    EXPECT_EQ(f.size(), 0);
    f.close();
}

// B2
TEST_F(TextFileSaverTest, Save_TargetIsDirectory_OpenFailsReturnsFalse)
{
    // Arrange: 目标路径是已存在目录，QFile::open(WriteOnly) 必然失败
    obj->setFilePath(tmpDir->path());
    // Act
    const bool ret = obj->save();
    // Assert
    EXPECT_FALSE(ret);
    EXPECT_FALSE(obj->errorString().isEmpty());
}

// B3
TEST_F(TextFileSaverTest, Save_MemoryInsufficientDocument_ReturnsFalse)
{
    // Arrange
    memResult = false;
    obj->setFilePath(filePath(QStringLiteral("mem1.txt")));
    // Act
    const bool ret = obj->save();
    // Assert: 首次内存检查即失败
    EXPECT_FALSE(ret);
    EXPECT_EQ(obj->errorString(), QStringLiteral("Insufficient memory to load document content"));
    EXPECT_EQ(memCallCount, 1);
    // 文件已先被打开截断，应为空文件
    QFile f(filePath(QStringLiteral("mem1.txt")));
    ASSERT_TRUE(f.open(QIODevice::ReadOnly));
    EXPECT_EQ(f.size(), 0);
    f.close();
}

// B4
TEST_F(TextFileSaverTest, Save_MemoryInsufficientConversion_ReturnsFalse)
{
    // Arrange: 第 1 次检查(文档)通过、第 2 次检查(转换)失败
    stub.reset(&Utils::isMemorySufficientForOperation);
    stub.set_lamda(&Utils::isMemorySufficientForOperation,
                   [this](Utils::OperationType, qlonglong, qlonglong) -> bool {
                       ++memCallCount;
                       return memCallCount == 1; // 首次 true，后续 false
                   });
    obj->setFilePath(filePath(QStringLiteral("mem2.txt")));
    // Act
    const bool ret = obj->save();
    // Assert
    EXPECT_FALSE(ret);
    EXPECT_EQ(obj->errorString(), QStringLiteral("Insufficient memory for encoding conversion"));
    EXPECT_EQ(memCallCount, 2);
}

// B5
TEST_F(TextFileSaverTest, Save_InvalidTargetEncoding_ConversionFails)
{
    // Arrange: 非法目标编码 → iconv_open 失败 → QTextCodec 也找不到 → 转换失败
    obj->setEncoding("UT-NOT-EXIST");
    obj->setFilePath(filePath(QStringLiteral("badenc.txt")));
    // Act
    const bool ret = obj->save();
    // Assert
    EXPECT_FALSE(ret);
    EXPECT_EQ(obj->errorString(), QStringLiteral("Encoding conversion failed"));
}

// B9
TEST_F(TextFileSaverTest, Save_SameEncodingUtf16_WritesRawUtf16Bytes)
{
    // Arrange: m_fromEncode(UTF-16) == m_toEncode → 无转换，原样写入 QString::utf16 字节
    const QString text = QStringLiteral("hi 中文");
    doc->setPlainText(text);
    obj->setEncoding("UTF-16");
    obj->setFilePath(filePath(QStringLiteral("utf16.txt")));
    // Act
    const bool ret = obj->save();
    // Assert
    EXPECT_TRUE(ret);
    QFile f(filePath(QStringLiteral("utf16.txt")));
    ASSERT_TRUE(f.open(QIODevice::ReadOnly));
    const QByteArray raw(reinterpret_cast<const char *>(text.utf16()), text.size() * 2);
    EXPECT_EQ(f.readAll(), raw);
    f.close();
}

// B6
TEST_F(TextFileSaverTest, Save_WriteShortCount_ReturnsFalse)
{
    // Arrange: stub QIODevice::write 返回 maxSize-1，触发写入不完整分支
    using WritePtr = qint64 (QIODevice::*)(const char *, qint64);
    stub.set_lamda(static_cast<WritePtr>(&QIODevice::write),
                   [this](QIODevice *, const char *, qint64 maxSize) -> qint64 {
                       ++writeCallCount;
                       return maxSize - 1;
                   });
    obj->setFilePath(filePath(QStringLiteral("wfail.txt")));
    // Act
    const bool ret = obj->save();
    // Assert
    EXPECT_FALSE(ret);
    EXPECT_EQ(writeCallCount, 1);
}

// B10
TEST_F(TextFileSaverTest, SaveAs_NewPath_SucceedsAndWritesFile)
{
    // Arrange
    obj->setFilePath(filePath(QStringLiteral("origin.txt")));
    const QString newPath = filePath(QStringLiteral("saved-as.txt"));
    // Act
    const bool ret = obj->saveAs(newPath);
    // Assert
    EXPECT_TRUE(ret);
    QFile f(newPath);
    ASSERT_TRUE(f.open(QIODevice::ReadOnly));
    EXPECT_EQ(f.readAll(), QStringLiteral("hello 中文").toUtf8());
    f.close();
}

// B11
TEST_F(TextFileSaverTest, SaveAs_FailureRestoresOriginalPath)
{
    // Arrange
    const QString originPath = filePath(QStringLiteral("restore.txt"));
    obj->setFilePath(originPath);
    // Act: saveAs 到目录路径 → 失败并回退
    const bool failed = obj->saveAs(tmpDir->path());
    // Assert: 失败后原路径保持，后续 save() 写入原路径成功
    EXPECT_FALSE(failed);
    EXPECT_TRUE(obj->save());
    QFile f(originPath);
    ASSERT_TRUE(f.open(QIODevice::ReadOnly));
    EXPECT_EQ(f.readAll(), QStringLiteral("hello 中文").toUtf8());
    f.close();
}

// B8
TEST_F(TextFileSaverTest, Save_LargeDocumentMultiChunk_WritesAllBytes)
{
    // Arrange: 11MB 文本 → chunkSize = max(10MB, 11MB/8) = 10MB → 2 个分块
    const int chunkChars = 11 * 1024 * 1024;
    QString big(chunkChars, QChar(QLatin1Char('a')));
    big.append(QStringLiteral("END"));
    doc->setPlainText(big);
    obj->setFilePath(filePath(QStringLiteral("big.txt")));
    // Act
    const bool ret = obj->save();
    // Assert
    EXPECT_TRUE(ret);
    QFile f(filePath(QStringLiteral("big.txt")));
    ASSERT_TRUE(f.open(QIODevice::ReadOnly));
    const QByteArray data = f.readAll();
    f.close();
    EXPECT_EQ(data.size(), big.toUtf8().size());
    EXPECT_EQ(data.left(1), QByteArrayLiteral("a"));
    EXPECT_TRUE(data.endsWith(QByteArrayLiteral("END")));
    EXPECT_GE(memCallCount, 3); // 1 次文档检查 + 每分块 1 次转换检查
}

// 构造/析构与 errorString 取值（每个用例的 SetUp/TearDown 均覆盖构造析构）
TEST_F(TextFileSaverTest, ErrorString_InitiallyEmpty_ReturnsEmptyByDefault)
{
    // Arrange/Act/Assert: 新建对象错误串为空；getter 语义为返回最近一次错误
    EXPECT_TRUE(obj->errorString().isEmpty());
    obj->setFilePath(QString());
    EXPECT_FALSE(obj->save());
    EXPECT_FALSE(obj->errorString().isEmpty());
}
