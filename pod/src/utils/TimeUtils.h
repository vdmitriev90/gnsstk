#pragma once

#include <chrono>
#include <cstdint>
#include <string>

namespace pod
{
    /// RAII timer: on destruction stores elapsed time in milliseconds into the referenced variable.
    /// Works on normal exit and during stack unwinding.
    class StopTimer
    {
      public:
        explicit StopTimer(std::int64_t& result);

        StopTimer(const StopTimer&) = delete;
        StopTimer& operator=(const StopTimer&) = delete;

        ~StopTimer();

      private:
        std::int64_t& result_;
        std::chrono::steady_clock::time_point start_;
    };

    /// Current local time formatted as yymmddHHMM.
    std::string currentDateStr();
} // namespace pod
