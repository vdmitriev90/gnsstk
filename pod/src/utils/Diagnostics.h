#ifndef POD_DIAGNOSTICS_H
#define POD_DIAGNOSTICS_H

#include <filesystem>
#include <fstream>
#include <ostream>
#include <sstream>
#include <streambuf>

#ifdef _WIN32
#include <Windows.h>
#endif

namespace pod
{
	// Directory containing the running executable; falls back to the current
	// working directory if it cannot be determined.
	inline std::filesystem::path diagOutputDir()
	{
#ifdef _WIN32
		wchar_t path[MAX_PATH] = {};
		const DWORD len = ::GetModuleFileNameW(nullptr, path, MAX_PATH);
		if (len > 0 && len < MAX_PATH)
		{
			const std::filesystem::path exePath(path);
			return exePath.parent_path();
		}
#else
		std::error_code ec;
		const auto exePath = std::filesystem::canonical("/proc/self/exe", ec);
		if (!ec)
			return exePath.parent_path();
#endif
		return std::filesystem::current_path();
	}

	// General-purpose diagnostic stream used across processing code.
	// Diagnostic file output is opt-in; disabled builds use a discard stream.
	inline std::ostream& diagLog()
	{
#ifdef ENABLE_DBG_OUTPUT
		// The file is truncated at first use; every line is flushed immediately.
		static std::ofstream os = []() {
			std::ofstream f;
			try
			{
				f.open((diagOutputDir() / "PodDiag.log").string(), std::ios::out | std::ios::trunc);
			}
			catch (...)
			{
			}
			f << std::unitbuf;
			return f;
		}();
#else
		class NullBuffer final : public std::streambuf
		{
		  protected:
			int_type overflow(int_type ch) override
			{
				return traits_type::not_eof(ch);
			}
		};
		static NullBuffer buffer;
		static std::ostream os(&buffer);
#endif
		return os;
	}
} // namespace pod

#ifdef ENABLE_DBG_OUTPUT
#define DIAG_LOG(expr) do { ::pod::diagLog() << expr << std::endl; } while (false)
#define DIAG_LOG_BLOCK(...) do { __VA_ARGS__ } while (false)
#else
#define DIAG_LOG(expr) do { } while (false)
#define DIAG_LOG_BLOCK(...) do { } while (false)
#endif

#ifdef _WIN32
/// print to the debugger Output window
#define DBOUT(s)                                                                                   \
    {                                                                                              \
        std::ostringstream os_;                                                                    \
        os_ << s;                                                                                  \
        OutputDebugString(os_.str().c_str());                                                      \
    }
#define DBOUT_LINE(s)                                                                              \
    {                                                                                              \
        std::ostringstream os_;                                                                    \
        os_ << s;                                                                                  \
        OutputDebugString(os_.str().c_str());                                                      \
        OutputDebugString("\r\n");                                                                 \
    }
#else
#define DBOUT(s)
#define DBOUT_LINE(s)
#endif

#endif // POD_DIAGNOSTICS_H
