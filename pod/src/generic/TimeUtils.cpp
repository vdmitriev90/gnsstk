#include "TimeUtils.h"

#include <ctime>
#include <iomanip>
#include <sstream>

namespace pod
{
    StopTimer::StopTimer(std::int64_t& result) : result_(result), start_(std::chrono::steady_clock::now()) {}

    StopTimer::~StopTimer()
    {
        using namespace std::chrono;
        result_ = duration_cast<milliseconds>(steady_clock::now() - start_).count();
    }

	std::string currentDateStr()
	{
		const std::time_t t = std::time(nullptr);
		std::ostringstream oss;
		oss << std::put_time(std::localtime(&t), "%y%m%d%H%M%S");
		return oss.str();
	}
} // namespace pod
