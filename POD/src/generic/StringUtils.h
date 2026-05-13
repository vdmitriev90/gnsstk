#ifndef POD_STRING_UTILS_H
#define POD_STRING_UTILS_H

#include "CivilTime.hpp"

#include <string>

namespace pod
{

    class StringUtils
    {
      public:
        static std::string formatTime(const gnsstk::CommonTime& t);
        static bool tryParseTime(const std::string& str, gnsstk::CommonTime& time);
        static std::vector<std::string> split(const std::string& str, const std::string& delim);
    };
} // namespace pod

#endif // ! POD_STRING_UTILS_H
