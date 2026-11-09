#include "ProjectConfig.hpp"

#include "CivilTime.hpp"

#include <format>
#include <toml++/toml.hpp>

namespace pod::config
{
    namespace
    {
        using namespace std::string_literals;

        [[noreturn]] void fail(std::string_view section, std::string_view key, std::string_view what)
        {
            throw ConfigError(std::format("Config error in [{}] key '{}': {}", section, key, what));
        }

        const toml::table& requireTable(const toml::table& parent, std::string_view name)
        {
            const auto* t = parent[name].as_table();
            if (!t)
                fail("", name, "table is missing");
            return *t;
        }

        template <typename T>
        T require(const toml::table& t, std::string_view section, std::string_view key)
        {
            const auto node = t[key];
            if (!node)
                fail(section, key, "required value is missing");
            const auto v = node.value<T>();
            if (!v)
                fail(section, key, "value has wrong type");
            return *v;
        }

        template <typename T>
        T optional(const toml::table& t, std::string_view section, std::string_view key, T defaultValue)
        {
            if (!t.contains(key))
                return defaultValue;
            return require<T>(t, section, key);
        }

        template <typename T>
        std::vector<T> requireArray(const toml::table& t, std::string_view section, std::string_view key)
        {
            const auto* arr = t[key].as_array();
            if (!arr)
                fail(section, key, "required array is missing");
            std::vector<T> result;
            for (const auto& el : *arr)
            {
                const auto v = el.value<T>();
                if (!v)
                    fail(section, key, "array element has wrong type");
                result.push_back(*v);
            }
            return result;
        }

        gnsstk::Triple requireTriple(const toml::table& t, std::string_view section, std::string_view key)
        {
            const auto v = requireArray<double>(t, section, key);
            if (v.size() != 3)
                fail(section, key, "array must contain exactly three numbers");
            return gnsstk::Triple(v[0], v[1], v[2]);
        }

        gnsstk::CommonTime toCommonTime(const toml::date_time& dt)
        {
            const auto& d = dt.date;
            const auto& tm = dt.time;
            gnsstk::CivilTime ct(static_cast<int>(d.year),
                                 static_cast<int>(d.month),
                                 static_cast<int>(d.day),
                                 static_cast<int>(tm.hour),
                                 static_cast<int>(tm.minute),
                                 tm.second + tm.nanosecond * 1e-9,
                                 gnsstk::TimeSystem::GPS);
            auto result = ct.convertToCommonTime();
            if (dt.offset)
                result.addSeconds(-60.0 * dt.offset->minutes);
            return result;
        }

        std::optional<gnsstk::CommonTime> optionalTime(const toml::table& t,
                                                       std::string_view section,
                                                       std::string_view key)
        {
            const auto node = t[key];
            if (!node)
                return std::nullopt;
            const auto* dt = node.as_date_time();
            if (!dt)
                fail(section, key, "value must be a TOML date-time");
            return toCommonTime(dt->get());
        }

        template <typename E>
        E requireEnum(const toml::table& t,
                      std::string_view section,
                      std::string_view key,
                      std::initializer_list<E> allowed)
        {
            const int v = require<int>(t, section, key);
            for (const auto e : allowed)
                if (static_cast<int>(e) == v)
                    return e;
            fail(section, key, "unsupported value " + std::to_string(v));
        }

        StationConfig parseStation(const toml::table& t, const std::string& id)
        {
            const std::string section = "stations." + id;
            StationConfig s;
            s.nominalPositionEcefM = requireTriple(t, section, "nominal_position_ecef_m");
            s.markerToArpUenM = requireTriple(t, section, "marker_to_arp_uen_m");
            s.antennaModel = optional<std::string>(t, section, "antenna_model", "");
            s.usePcPatterns = optional<bool>(t, section, "use_pc_patterns", true);
            s.useAzimuth = optional<bool>(t, section, "use_azimuth", false);
            return s;
        }

        GenericFilesConfig parseGenericFiles(const toml::table& t)
        {
            constexpr auto sec = "generic_files";
            GenericFilesConfig g;
            g.directory = require<std::string>(t, sec, "directory");
            g.satelliteDataFile = require<std::string>(t, sec, "satellite_data_file");
            g.antexFile = require<std::string>(t, sec, "antex_file");
            g.oceanLoadingFile = require<std::string>(t, sec, "ocean_loading_file");
            return g;
        }

        ProcessingConfig parseProcessing(const toml::table& t)
        {
            constexpr auto sec = "processing";
            ProcessingConfig p;
            p.solutionType = requireEnum<SlnType>(
                t,
                sec,
                "solution_type",
                {SlnType::Standalone, SlnType::CodeDiff, SlnType::PdFloat, SlnType::PdFixed, SlnType::PppFloat});
            p.applyCodeBias = require<bool>(t, sec, "apply_code_bias");
            p.useRinexClock = require<bool>(t, sec, "use_rinex_clock");
            p.fullOutput = require<bool>(t, sec, "full_output");
            for (const auto& code : requireArray<std::string>(t, sec, "satellite_systems"))
            {
                const auto sys = gnsstk::convertCodeToSatelliteSystem(code);
                if (sys == gnsstk::SatelliteSystem::Unknown)
                    fail(sec, "satellite_systems", "unknown satellite system code '" + code + "'");
                p.satelliteSystems.insert(sys);
            }
            for (const int band : requireArray<int>(t, sec, "carrier_bands"))
            {
                if (band != static_cast<int>(CarrierBand::L1) && band != static_cast<int>(CarrierBand::L2)
                    && band != static_cast<int>(CarrierBand::L5))
                    fail(sec, "carrier_bands", "unsupported value " + std::to_string(band));
                p.carrierBands.insert(static_cast<CarrierBand>(band));
            }

            p.computeTroposphere = require<bool>(t, sec, "compute_troposphere");
            p.troposphereVerticalProcessNoise = require<double>(t, sec, "troposphere_vertical_process_noise");
            p.troposphereModelType = requireEnum<TropoModelType>(
                t,
                sec,
                "troposphere_model_type",
                {TropoModelType::Simple, TropoModelType::SimpleWithGradients, TropoModelType::Advanced});
            p.troposphereQ1 = require<double>(t, sec, "troposphere_q1");
            p.troposphereQ2 = require<double>(t, sec, "troposphere_q2");

            p.useAdvancedClockModel = require<bool>(t, sec, "use_advanced_clock_model");
            p.clockQ1 = require<double>(t, sec, "clock_q1");
            p.clockQ2 = require<double>(t, sec, "clock_q2");

            p.computeIonosphere = require<bool>(t, sec, "compute_ionosphere");
            p.ionosphereModelType = require<int>(t, sec, "ionosphere_model_type");
            p.ionosphereSigmaM = require<double>(t, sec, "ionosphere_sigma_m");
            p.codeIonosphereCorrectionType = require<int>(t, sec, "code_ionosphere_correction_type");
            p.smoothCode = require<bool>(t, sec, "smooth_code");
            p.spaceborneReceiver = require<bool>(t, sec, "spaceborne_receiver");

            requireEnum<Dynamics>(t, sec, "dynamics", {Dynamics::Kinematic, Dynamics::Static});
            p.elevationMaskDeg = require<double>(t, sec, "elevation_mask_deg");
            p.snrMaskDbHz = require<double>(t, sec, "snr_mask_db_hz");

            p.forwardBackwardCycles = require<int>(t, sec, "forward_backward_cycles");
            p.phaseLimits = requireArray<double>(t, sec, "phase_limits");
            p.codeLimits = requireArray<double>(t, sec, "code_limits");
            p.positionSigmaM = require<double>(t, sec, "position_sigma_m");
            p.clockSigmaM = require<double>(t, sec, "clock_sigma_m");
            p.weightFactor = require<double>(t, sec, "weight_factor");
            p.lossOfLockThreshold = require<double>(t, sec, "loss_of_lock_threshold");
            p.MelbourneWubbenaThreshold = require<double>(t, sec, "melbourne_wubbena_threshold");

            p.decimationIntervalSeconds = require<int>(t, sec, "decimation_interval_s");
            p.decimationToleranceSeconds = require<double>(t, sec, "decimation_tolerance_s");

            if (t.contains("approximate_position_provider"))
            {
                p.approximatePositionProvider = requireEnum<ApprPositionSource>(t,
                                                                                sec,
                                                                                "approximate_position_provider",
                                                                                {ApprPositionSource::FromConfig,
                                                                                 ApprPositionSource::ComputeForEachEpoch,
                                                                                 ApprPositionSource::ComputeForFirstEpoch,
                                                                                 ApprPositionSource::LoadFromFile,
                                                                                 ApprPositionSource::FromRinexHeader});
            }
            else
                p.approximatePositionProvider = ApprPositionSource::FromRinexHeader;
            if (p.approximatePositionProvider == ApprPositionSource::LoadFromFile)
                p.approximatePositionFile = require<std::string>(t, sec, "approximate_position_file");
            else
                p.approximatePositionFile = optional<std::string>(t, sec, "approximate_position_file", "");
            return p;
        }
    } // namespace

    const StationConfig* ProjectConfig::findStation(const std::string& siteId) const
    {
        const auto it = stations.find(siteId);
        return it == stations.end() ? nullptr : &it->second;
    }

    const StationConfig& ProjectConfig::station(const std::string& siteId) const
    {
        const auto it = stations.find(siteId);
        if (it == stations.end())
            throw ConfigError("Config error: no [stations." + siteId + "] table");
        return it->second;
    }

    ProjectConfig ProjectConfig::load(const std::filesystem::path& file)
    {
        toml::table root;
        try
        {
            root = toml::parse_file(file.string());
        }
        catch (const toml::parse_error& e)
        {
            throw ConfigError(std::format("Can't parse configuration file '{}': {}", file.string(), e.description()));
        }

        ProjectConfig cfg;
        cfg.siteRover = require<std::string>(root, "", "site_rover");
        cfg.siteBase = optional<std::string>(root, "", "site_base", "");
        cfg.genericFiles = parseGenericFiles(requireTable(root, "generic_files"));
        cfg.processing = parseProcessing(requireTable(root, "processing"));

        const auto stationsFile = cfg.genericFiles.directory / "stations.toml";
        toml::table stationsRoot;
        try
        {
            stationsRoot = toml::parse_file(stationsFile.string());
        }
        catch (const toml::parse_error& e)
        {
            throw ConfigError(
                std::format("Can't parse stations file '{}': {}", stationsFile.string(), e.description()));
        }

        const toml::table* stations = stationsRoot["stations"].as_table();
        if (!stations)
            stations = &stationsRoot;
        for (const auto& [id, node] : *stations)
        {
            const auto* t = node.as_table();
            if (!t)
                fail("stations", id.str(), "station must be a table");
            cfg.stations.emplace(std::string(id.str()), parseStation(*t, std::string(id.str())));
        }
        return cfg;
    }
} // namespace pod::config
