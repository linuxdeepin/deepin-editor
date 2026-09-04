// SPDX-FileCopyrightText: 2026 UnionTech Software Technology Co., Ltd.
// SPDX-License-Identifier: GPL-3.0-or-later

// Utils（daemon/src/utils.h）单元测试
//
// 方法与分支清单：
// Utils::fileExists(path)
//   B1: check_file.exists() == true 且 isFile() == true  → true
//   B2: 存在但是目录（isFile() == false）               → false
//   B3: 不存在（exists() == false）                     → false
//   B4: 空路径                                           → false
// Utils::fileIsWritable(path)
//   B5: permissions 含 WriteUser                         → true
//   B6: permissions 不含 WriteUser（只读）               → false
//   B7: 文件不存在（permissions 为 0）                   → false
//
// 用例映射：
// - FileExists_Parameters_ExpectMatchingResult   /* TEST_P */ → B1/B2/B3/B4
// - FileExists_UnicodePath_ExistingFile_ReturnsTrue           → B1（unicode 维度）
// - FileIsWritable_ReadWriteFile_ReturnsTrue                  → B5
// - FileIsWritable_ReadOnlyFile_ReturnsFalse                  → B6
// - FileIsWritable_NonexistentFile_ReturnsFalse               → B7
//
// 环境隔离：全部路径来自 QTemporaryDir/QTemporaryFile（RAII 成员，TearDown 自动释放），
// 权限断言基于 QFileDevice 权限位本身（与运行用户是否 root 无关，确定性成立）。

#include <gtest/gtest.h>

#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QTemporaryDir>
#include <QTemporaryFile>

#include "utils.h"

namespace {

struct FileExistsCase {
    QString scenario;   // 场景描述（用于失败定位）
    QString path;       // 待测路径（相对 tmpDir 前缀填充）
    bool createAsFile;  // 是否先创建普通文件
    bool createAsDir;   // 是否先创建目录
    bool expected;      // 期望返回值
};

class UtilsFileExistsTest : public ::testing::TestWithParam<FileExistsCase> {
protected:
    void SetUp() override
    {
        ASSERT_TRUE(tmpDir.isValid());
    }

    void TearDown() override { }

    QTemporaryDir tmpDir;
};

// B1/B2/B3/B4：等价类 + 边界（存在文件 / 目录 / 缺失 / 空串）
TEST_P(UtilsFileExistsTest, FileExists_Parameters_ExpectMatchingResult)
{
    const FileExistsCase &c = GetParam();

    // Arrange
    QString path = c.path;
    if (!c.path.isEmpty() && !c.path.startsWith('/'))
        path = tmpDir.path() + '/' + c.path;
    if (c.createAsFile) {
        QFile f(path);
        ASSERT_TRUE(f.open(QIODevice::WriteOnly));
        f.close();
    }
    if (c.createAsDir)
        ASSERT_TRUE(QDir(tmpDir.path()).mkdir(c.path));

    // Act
    bool actual = Utils::fileExists(path);

    // Assert
    EXPECT_EQ(actual, c.expected) << "scenario: " << c.scenario.toStdString();
    EXPECT_EQ(QFile::exists(path), c.createAsFile || c.createAsDir)
        << "现场状态与预期创建物一致: " << c.scenario.toStdString();
}

INSTANTIATE_TEST_SUITE_P(FileExistsCases, UtilsFileExistsTest,
    ::testing::Values(
        FileExistsCase{"existing regular file", "plain.txt", true, false, true},   // B1
        FileExistsCase{"existing directory", "subdir", false, true, false},        // B2
        FileExistsCase{"missing path", "missing.txt", false, false, false},        // B3
        FileExistsCase{"empty path", "", false, false, false}));                   // B4

// B1：unicode 路径维度
TEST(UtilsTest, FileExists_UnicodePath_ExistingFile_ReturnsTrue)
{
    // Arrange
    QTemporaryDir tmpDir;
    ASSERT_TRUE(tmpDir.isValid());
    const QString path = tmpDir.path() + "/目录_文件.txt";
    QFile f(path);
    ASSERT_TRUE(f.open(QIODevice::WriteOnly));
    f.close();

    // Act
    bool ret = Utils::fileExists(path);

    // Assert
    EXPECT_TRUE(ret);                       // 期望：unicode 路径文件可被识别（B1）
    EXPECT_EQ(QFileInfo(path).isFile(), true);  // 现场复核确为普通文件
}

// B5：含 WriteUser 权限
TEST(UtilsTest, FileIsWritable_ReadWriteFile_ReturnsTrue)
{
    // Arrange
    QTemporaryDir tmpDir;
    ASSERT_TRUE(tmpDir.isValid());
    const QString path = tmpDir.path() + "/writable.txt";
    QFile f(path);
    ASSERT_TRUE(f.open(QIODevice::WriteOnly));
    f.close();
    ASSERT_TRUE(QFile::setPermissions(path,
        QFileDevice::ReadOwner | QFileDevice::WriteOwner | QFileDevice::ReadUser | QFileDevice::WriteUser));

    // Act
    bool ret = Utils::fileIsWritable(path);

    // Assert
    EXPECT_TRUE(ret);  // 期望：WriteUser 置位（B5）
    EXPECT_TRUE(QFile::permissions(path) & QFileDevice::WriteUser);  // 权限位现场复核
}

// B6：只读文件（无 WriteUser）
TEST(UtilsTest, FileIsWritable_ReadOnlyFile_ReturnsFalse)
{
    // Arrange
    QTemporaryDir tmpDir;
    ASSERT_TRUE(tmpDir.isValid());
    const QString path = tmpDir.path() + "/readonly.txt";
    QFile f(path);
    ASSERT_TRUE(f.open(QIODevice::WriteOnly));
    f.close();
    ASSERT_TRUE(QFile::setPermissions(path,
        QFileDevice::ReadOwner | QFileDevice::ReadUser));

    // Act
    bool ret = Utils::fileIsWritable(path);

    // Assert
    EXPECT_FALSE(ret);  // 期望：WriteUser 未置位（B6）
    EXPECT_FALSE(QFile::permissions(path) & QFileDevice::WriteUser);
}

// B7：不存在 → permissions 为 0
TEST(UtilsTest, FileIsWritable_NonexistentFile_ReturnsFalse)
{
    // Arrange
    QTemporaryDir tmpDir;
    ASSERT_TRUE(tmpDir.isValid());
    const QString path = tmpDir.path() + "/no-such-file.txt";

    // Act
    bool ret = Utils::fileIsWritable(path);

    // Assert
    EXPECT_FALSE(ret);  // 期望：缺失文件 permissions==0，无 WriteUser（B7）
    EXPECT_FALSE(QFile::exists(path));  // 现场复核：文件确实不存在
}

} // namespace
