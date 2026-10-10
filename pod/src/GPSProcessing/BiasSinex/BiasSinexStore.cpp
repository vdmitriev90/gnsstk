#include "BiasSinexStore.hpp"

#include "Exception.hpp"
#include "StringUtils.hpp"
#include "YDSTime.hpp"

#include <algorithm>
#include <cctype>
#include <charconv>
#include <cmath>
#include <cstdio>
#include <fstream>
#include <stdexcept>
#include <string>
#include <vector>

using namespace gnsstk;

namespace
{
	using gnsstk::StringUtils::asDouble;

	std::string_view trimView(std::string_view sv)
	{
		while (!sv.empty() && std::isspace(static_cast<unsigned char>(sv.front())))
			sv.remove_prefix(1);
		while (!sv.empty() && std::isspace(static_cast<unsigned char>(sv.back())))
			sv.remove_suffix(1);
		return sv;
	}

	std::string trimCopy(std::string_view sv)
	{
		sv = trimView(sv);
		return std::string(sv.begin(), sv.end());
	}

	std::vector<std::string> splitWhitespace(std::string_view sv)
	{
		std::vector<std::string> out;
		while (!sv.empty())
		{
			while (!sv.empty() && std::isspace(static_cast<unsigned char>(sv.front())))
				sv.remove_prefix(1);
			if (sv.empty())
				break;

			size_t len = 0;
			while (len < sv.size() && !std::isspace(static_cast<unsigned char>(sv[len])))
				++len;

			out.emplace_back(sv.substr(0, len));
			sv.remove_prefix(len);
		}
		return out;
	}

	double parseDoubleToken(std::string_view token, const std::string& ctx)
	{
		token = trimView(token);
		if (token.empty())
			throw InvalidRequest("Missing numeric value in " + ctx);

		double value = 0.0;
		const char* begin = token.data();
		const char* end = token.data() + token.size();
		const auto [ptr, ec] = std::from_chars(begin, end, value);
		if (ec == std::errc() && ptr == end)
			return value;

		return asDouble(std::string(token));
	}

	TimeSystem decodeTimeSystem(char code)
	{
		switch (code)
		{
		case 'G': return TimeSystem::GPS;
		case 'R': return TimeSystem::GLO;
		case 'E': return TimeSystem::GAL;
		case 'C': return TimeSystem::BDT;
		case 'J': return TimeSystem::QZS;
		case 'I': return TimeSystem::IRN;
		case 'U': return TimeSystem::UTC;
		default:  return TimeSystem::Unknown;
		}
	}

	SatelliteSystem decodeSatelliteSystem(char code)
	{
		switch (code)
		{
		case 'G': return SatelliteSystem::GPS;
		case 'R': return SatelliteSystem::Glonass;
		case 'E': return SatelliteSystem::Galileo;
		case 'C': return SatelliteSystem::BeiDou;
		case 'J': return SatelliteSystem::QZSS;
		case 'S': return SatelliteSystem::Geosync;
		case 'I': return SatelliteSystem::IRNSS;
		default:  return SatelliteSystem::Unknown;
		}
	}

	CommonTime parseBiasEpoch(const std::string& token, TimeSystem ts)
	{
		std::vector<std::string> parts = splitWhitespace(token);
		if (parts.size() != 1)
			throw InvalidRequest("Invalid BSX epoch token: '" + token + "'");

		const std::string& t = parts.front();
		int year = 0;
		int doy = 0;
		int sod = 0;
		if (std::sscanf(t.c_str(), "%d:%d:%d", &year, &doy, &sod) != 3)
			throw InvalidRequest("Invalid BSX epoch token: '" + token + "'");

		if (year <= 0 || doy <= 0 || sod < 0)
			throw InvalidRequest("Invalid BSX epoch values: '" + token + "'");

		return YDSTime(year, doy, static_cast<double>(sod), ts).convertToCommonTime();
	}

	std::optional<SatID> parseSatId(const std::string& token)
	{
		if (token.empty() || token == "----")
			return std::nullopt;
		if (token.size() < 2)
			throw InvalidRequest("Invalid PRN token: '" + token + "'");

		const SatelliteSystem system = decodeSatelliteSystem(token.front());
		if (system == SatelliteSystem::Unknown)
			throw InvalidRequest("Unknown satellite system in PRN token: '" + token + "'");

		int prn = 0;
		const auto digits = std::string_view(token).substr(1);
		const auto [ptr, ec] = std::from_chars(digits.data(), digits.data() + digits.size(), prn);
		if (ec != std::errc() || ptr != digits.data() + digits.size())
			throw InvalidRequest("Invalid PRN token: '" + token + "'");

		return SatID(prn, system);
	}

	pod::BiasSinexEntryType parseEntryType(const std::string& token)
	{
		if (token == "OSB")
			return pod::BiasSinexEntryType::OSB;
		if (token == "DSB")
			return pod::BiasSinexEntryType::DSB;
		return pod::BiasSinexEntryType::Unknown;
	}

	pod::BiasSinexUnit parseUnit(const std::string& token)
	{
		if (token == "ns")
			return pod::BiasSinexUnit::Ns;
		if (token == "cyc")
			return pod::BiasSinexUnit::Cycles;
		return pod::BiasSinexUnit::Unknown;
	}

	pod::BiasSinexHeader parseHeaderLine(const std::string& line)
	{
		if (line.rfind("%=BIA", 0) != 0)
			throw InvalidRequest("Invalid BSX header. Expected '%=BIA'.");

		const auto tokens = splitWhitespace(line);
		if (tokens.size() < 8)
			throw InvalidRequest("Invalid BSX header: insufficient fields.");

		pod::BiasSinexHeader header;
		header.version = parseDoubleToken(tokens[1], "BSX header version");
		if (std::fabs(header.version - 1.0) > 1e-9)
			throw InvalidRequest("Unsupported BSX version: '" + tokens[1] + "'");

		header.creationAgency = tokens[2];
		header.creationEpoch = tokens[3];
		header.dataAgency = tokens[4];
		header.startEpoch = tokens[5];
		header.endEpoch = tokens[6];
		header.observationCode = tokens[7].empty() ? ' ' : tokens[7].front();
		if (tokens.size() > 8)
			header.serial = tokens[8];

		return header;
	}

	bool isStationSolutionRecord(const std::string& line)
	{
		if (line.empty())
			return false;
		if (line.size() <= 15)
			return false;

		const std::size_t len = std::min<std::size_t>(9, line.size() - 15);
		return !trimCopy(std::string_view(line).substr(15, len)).empty();
	}

	pod::BiasSinexSolutionRecord parseSolutionRecord(const std::string& line,
													 std::size_t lineNumber,
													 TimeSystem ts)
	{
		if (line.empty() || line.front() != ' ')
			throw InvalidRequest("Invalid BIAS/SOLUTION line at " + std::to_string(lineNumber));

		const auto safeSubstr = [&](std::size_t pos, std::size_t len) -> std::string
		{
			if (pos >= line.size())
				return {};
			return line.substr(pos, std::min(len, line.size() - pos));
		};

		pod::BiasSinexSolutionRecord record;
		record.lineNumber = lineNumber;
		record.rawLine = line;
		record.entryType = parseEntryType(trimCopy(safeSubstr(1, 3)));
		record.svn = trimCopy(safeSubstr(6, 4));
		record.sat = parseSatId(trimCopy(safeSubstr(11, 3)));
		record.station = trimCopy(safeSubstr(15, 9));
		record.obs1 = trimCopy(safeSubstr(25, 4));
		record.obs2 = trimCopy(safeSubstr(30, 4));
		record.start = parseBiasEpoch(trimCopy(safeSubstr(35, 14)), ts);
		record.end = parseBiasEpoch(trimCopy(safeSubstr(50, 14)), ts);
		record.unit = parseUnit(trimCopy(safeSubstr(65, 3)));

		const auto tail = splitWhitespace(safeSubstr(68, line.size() > 68 ? line.size() - 68 : 0));
		if (tail.size() < 2)
			throw InvalidRequest("Incomplete BIAS/SOLUTION numeric tail at line " + std::to_string(lineNumber));

		record.estimatedValue = parseDoubleToken(tail[0], "estimated value");
		record.stdDev = parseDoubleToken(tail[1], "standard deviation");
		if (tail.size() > 2)
			record.estimatedSlope = parseDoubleToken(tail[2], "estimated slope");
		if (tail.size() > 3)
			record.slopeStdDev = parseDoubleToken(tail[3], "slope standard deviation");

		if (record.start >= record.end)
			throw InvalidRequest("Invalid BSX interval at line " + std::to_string(lineNumber));
		if (record.obs1.empty())
			throw InvalidRequest("Missing OBS1 at line " + std::to_string(lineNumber));
		if (!record.sat && record.station.empty())
			throw InvalidRequest("Record without satellite or station target at line " + std::to_string(lineNumber));
		if (record.unit == pod::BiasSinexUnit::Unknown)
			throw InvalidRequest("Unsupported BSX unit at line " + std::to_string(lineNumber));

		record.target = record.station.empty() ? pod::BiasSinexTarget::Satellite
											   : pod::BiasSinexTarget::Station;
		return record;
	}
}

namespace pod
{
	bool BiasSinexSolutionRecord::isSatelliteOsb() const noexcept
	{
		return entryType == BiasSinexEntryType::OSB
			&& target == BiasSinexTarget::Satellite
			&& sat.has_value();
	}

	bool BiasSinexSolutionRecord::contains(const CommonTime& epoch) const noexcept
	{
		return start <= epoch && epoch < end;
	}

	bool BiasSinexStore::SatelliteObservableKey::operator<(const SatelliteObservableKey& rhs) const noexcept
	{
		if (sat < rhs.sat)
			return true;
		if (rhs.sat < sat)
			return false;
		return observable < rhs.observable;
	}

	void BiasSinexStore::clear()
	{
		header_ = {};
		description_.values.clear();
		records_.clear();
		satelliteOsbIndex_.clear();
	}

	void BiasSinexStore::loadFile(const std::string& filePath)
	{
		std::ifstream input(filePath);
		if (!input.is_open())
			throw FileMissingException("Unable to open BSX file: " + filePath);

		std::string line;
		std::size_t lineNumber = 0;
		bool inDescription = false;
		bool inSolution = false;
		bool headerRead = false;
		TimeSystem fileTimeSystem = TimeSystem::Unknown;

		while (std::getline(input, line))
		{
			++lineNumber;
			if (!line.empty() && line.back() == '\r')
				line.pop_back();
			if (line.empty())
				continue;

			if (!headerRead)
			{
				header_ = parseHeaderLine(line);
				headerRead = true;
				fileTimeSystem = decodeTimeSystem(header_.observationCode);
				continue;
			}

			if (line == "%ENDBIA")
				break;

			if (line == "+BIAS/DESCRIPTION")
			{
				inDescription = true;
				inSolution = false;
				continue;
			}
			if (line == "-BIAS/DESCRIPTION")
			{
				inDescription = false;
				continue;
			}
			if (line == "+BIAS/SOLUTION")
			{
				inSolution = true;
				inDescription = false;
				continue;
			}
			if (line == "-BIAS/SOLUTION")
			{
				inSolution = false;
				continue;
			}

			if (line.front() == '+' || line.front() == '-')
			{
				inDescription = false;
				inSolution = false;
				continue;
			}

			if (line.front() == '*')
			{
				if (inDescription)
				{
					const std::string payload = trimCopy(std::string_view(line).substr(1));
					if (!payload.empty())
						description_.values["*"].push_back(payload);
				}
				continue;
			}

			if (inDescription)
			{
				const std::string key = trimCopy(std::string_view(line).substr(0, std::min<std::size_t>(40, line.size())));
				const std::string value = line.size() > 40 ? trimCopy(std::string_view(line).substr(40)) : std::string{};
				if (!key.empty())
				{
					description_.values[key].push_back(value);
					if (key == "TIME_SYSTEM" && !value.empty())
						fileTimeSystem = decodeTimeSystem(value.front());
				}
				continue;
			}

			if (inSolution)
			{
				if (isStationSolutionRecord(line))
					continue;

				records_.push_back(parseSolutionRecord(line, lineNumber, fileTimeSystem));
			}
		}

		if (!headerRead)
			throw InvalidRequest("Empty BSX file: " + filePath);

		rebuildIndex();
	}

	const BiasSinexHeader& BiasSinexStore::header() const noexcept
	{
		return header_;
	}

	const BiasSinexDescription& BiasSinexStore::description() const noexcept
	{
		return description_;
	}

	const std::vector<BiasSinexSolutionRecord>& BiasSinexStore::records() const noexcept
	{
		return records_;
	}

	const BiasSinexSolutionRecord* BiasSinexStore::findSatelliteOsb(const SatID& sat,
																	std::string_view observable,
																	const CommonTime& epoch) const noexcept
	{
		const SatelliteObservableKey key{sat, std::string(observable)};
		const auto it = satelliteOsbIndex_.find(key);
		if (it == satelliteOsbIndex_.end())
			return nullptr;

		const auto& indexes = it->second;
		auto ub = std::upper_bound(indexes.begin(), indexes.end(), epoch,
								   [this](const CommonTime& time, std::size_t idx) {
									   return time < records_[idx].start;
								   });

		while (ub != indexes.begin())
		{
			--ub;
			const auto& rec = records_[*ub];
			if (rec.contains(epoch))
				return &rec;
			if (rec.end <= epoch)
				break;
		}

		return nullptr;
	}

	void BiasSinexStore::rebuildIndex()
	{
		satelliteOsbIndex_.clear();
		for (std::size_t i = 0; i < records_.size(); ++i)
		{
			const auto& rec = records_[i];
			if (!rec.isSatelliteOsb())
				continue;

			satelliteOsbIndex_[{*rec.sat, rec.obs1}].push_back(i);
		}

		for (auto& [key, indexes] : satelliteOsbIndex_)
		{
			std::sort(indexes.begin(), indexes.end(), [this](std::size_t lhs, std::size_t rhs) {
				const auto& a = records_[lhs];
				const auto& b = records_[rhs];
				if (a.start < b.start)
					return true;
				if (b.start < a.start)
					return false;
				return a.end < b.end;
			});
		}
	}
} // namespace pod
