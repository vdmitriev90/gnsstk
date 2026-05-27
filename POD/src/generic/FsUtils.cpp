#include "FsUtils.h"

#include <iostream>
#include <regex>
#include <stdexcept>

namespace fs = std::filesystem;

namespace pod::FsUtils
{
    std::vector<fs::path> getAllFilesInDir(const fs::path& dir)
    {
        if (!fs::exists(dir))
            throw std::runtime_error("directory: " + dir.string() + " doesn't exist.");

        std::vector<fs::path> result;
        for (const auto& entry : fs::directory_iterator(dir))
        {
            if (entry.is_regular_file())
                result.push_back(entry.path());
        }
        return result;
    }

    std::vector<fs::path> getFilesByExtension(const fs::path& dir, const std::string& ext)
    {
        auto all_files = getAllFilesInDir(dir);

        std::vector<fs::path> result;
        result.reserve(all_files.size());

        for (const auto& p : all_files)
        {
            if (p.extension().string() == ext)
                result.push_back(p);
        }
        return result;
    }

    std::vector<fs::path> getFilesByExtensionRegex(const fs::path& dir,
                                                   const std::string& pattern)
    {
        std::regex rx(pattern);
        auto all_files = getAllFilesInDir(dir);

        std::vector<fs::path> result;
        result.reserve(all_files.size());

        for (const auto& p : all_files)
        {
            if (p.has_extension() && std::regex_match(p.extension().string(), rx))
                result.push_back(p);
        }
        return result;
    }
} // namespace pod::FsUtils
