#include "ApprPosProvider.hpp"

#include "Bancroft.hpp"
#include "PRSolution2.hpp"
#include "StringUtils.h"
#include "sqlite3.h"

#include <filesystem>

using namespace gnsstk;
namespace fs = std::filesystem;

namespace pod
{
    std::string getPosSourceString(ApprPositionSource source)
    {
        static const std::map<ApprPositionSource, std::string> posSource2Str{
            {ApprPositionSource::FromConfig, "ini file"},
            {ApprPositionSource::ComputeForEachEpoch, "Statndalone for each epoch"},
            {ApprPositionSource::ComputeForFirstEpoch, "Statndalone for first epoch"},
            {ApprPositionSource::LoadFromFile, "*.pos file"},
        };
        const auto it = posSource2Str.find(source);
        if (it != posSource2Str.end())
            return it->second;

        GNSSTK_ASSERT(false);
        return "Unknown";
    }

    int IApprPosProvider::ComputeApprSol(const gnsstk::IRinex& rin_epoch,
                                         NavLibrary& ephem,
                                         gnsstk::Vector<double>& solution)
    {
        auto svs = rin_epoch.getBody().getVectorOfSatID().toStdVector();
        auto meas = rin_epoch.getBody().getVectorOfTypeID(TypeID::C1).toStdVector();

        Matrix<double> svp;
        if (PRSolution2::PrepareAutonomousSolution(rin_epoch.getHeader().epoch, svs, meas, ephem, svp))
            return -1;

        Bancroft ban;

        if (ban.Compute(svp, solution))
            return -2;

        return 0;
    }

    int ComputeApprPos::getPosition(const gnsstk::IRinex& rin_epoch, gnsstk::Position& pos)
    {
        auto t = rin_epoch.getHeader().epoch;
        auto it = pvtStore.find(t);
        if (it == pvtStore.end())
        {
            Vector<double> vect;
            int rc = IApprPosProvider::ComputeApprSol(rin_epoch, ephStore, vect);
            if (!rc)
            {
                pos = Position(vect[0], vect[1], vect[2]);
                Xvt xvt;
                xvt.frame = RefFrame(RefFrameSys::WGS84, t);
                xvt.x = pos;
                xvt.clkbias = vect[3];
                pvtStore[t] = xvt;
            }
            else
                return rc;
        }
        else
            pos = Position(it->second.x[0], it->second.x[1], it->second.x[2]);

        return 0;
    }

    int ComputeOnePos::getPosition(const gnsstk::IRinex& rin_epoch, gnsstk::Position& pos)
    {
        if (isFirstTime)
        {
            Vector<double> vect;
            int rc = IApprPosProvider::ComputeApprSol(rin_epoch, ephStore, vect);
            if (!rc)
            {
                apprPos = Position(vect[0], vect[1], vect[2]);
                isFirstTime = false;
            }
            else
                return rc;
        }
        pos = apprPos;
        return 0;
    }

    bool PositionFromFile::loadApprPos(const std::filesystem::path& path)
    {
        pvtStore.clear();
        try
        {
            std::ifstream file(path);
            if (file.is_open())
            {
                std::string line;
                while (std::getline(file, line))
                {
                    auto words = StringUtils::split(line, ";");
                    if (words.size() > 5)
                    {
                        int sType = stoi(words[5]);
                        CommonTime ct;
                        if (sType && StringUtils::tryParseTime(words[0], ct))
                        {
                            Xvt xvt;
                            xvt.x = Triple(stod(words[1]), stod(words[2]), stod(words[3]));
                            xvt.clkbias = stod(words[4]);
                            pvtStore.insert(std::make_pair(ct, xvt));
                        }
                    }
                }
            }
            else
            {
                auto mess = "Can't load data from file: " + path.string();
                std::exception e(mess.c_str());
                throw e;
            }
        }
        catch (const std::exception& e)
        {
            std::cout << e.what() << std::endl;
            throw e;
        }
        return true;
    }

    int PositionFromFile::getPosition(const gnsstk::IRinex& rin_epoch, gnsstk::Position& pos)
    {
        auto it = pvtStore.find(rin_epoch.getHeader().epoch);
        if (it == pvtStore.end())
            return -1;
        pos = Position(it->second.x[0], it->second.x[1], it->second.x[2]);
        return 0;
    }
} // namespace pod