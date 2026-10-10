#ifndef POD_FS_UTILS_H
#define POD_FS_UTILS_H

#include <filesystem>
#include <string>
#include <vector>

namespace pod::FsUtils
{
    /// Returns all file paths in \p dir (non-recursive).
    /// Throws std::runtime_error if \p dir does not exist.
    std::vector<std::filesystem::path> getAllFilesInDir(const std::filesystem::path& dir);

    /// Returns file paths in \p dir whose extension matches \p ext exactly
    /// (e.g. ".obs", ".rnx").  Case-sensitive.
    std::vector<std::filesystem::path> getFilesByExtension(const std::filesystem::path& dir,
                                                           const std::string& ext);

    /// Returns file paths in \p dir whose extension matches the regex \p pattern
    /// (e.g. R"(\.\d{2}[oOdD])").
    std::vector<std::filesystem::path> getFilesByExtensionRegex(const std::filesystem::path& dir,
                                                                const std::string& pattern);
} // namespace pod::FsUtils

#endif // ! POD_FS_UTILS_H
