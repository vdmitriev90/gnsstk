#ifndef POD_STRING_UTILS_H
#define POD_STRING_UTILS_H

#include "CivilTime.hpp"

#include <string>

namespace pod::StringUtils
{
    std::string formatTime(const gnsstk::CommonTime& t);
    bool tryParseTime(const std::string& str, gnsstk::CommonTime& time);
    std::vector<std::string> split(const std::string& str, const std::string& delim);

} // namespace pod::StringUtils

#endif // ! POD_STRING_UTILS_H
