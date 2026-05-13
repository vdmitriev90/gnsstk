#ifndef POD_FS_UTILS_H
#define POD_FS_UTILS_H

#include <filesystem>
#include <list>
#include <string>

namespace pod
{
    class FsUtils
    {
      public:
        static void getAllFilesInDir(const std::string& dir, std::list<std::string>& files);
        static void getAllFilesInDir(const std::string& dir,
                                     std::list<std::filesystem::path>& files);

        static void getAllFilesInDir(const std::string& dir,
                                     const std::string& ext,
                                     std::list<std::string>& files);
        static void getAllFilesInDir(const std::string& dir,
                                     const std::string& ext,
                                     std::list<std::filesystem::path>& files);
    };
} // namespace pod

#endif // ! POD_FS_UTILS_H
