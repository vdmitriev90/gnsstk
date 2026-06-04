#include "PPPSolutionBase.h"

#include "FsUtils.h"
#include "GnssDataStore.hpp"
#include "GnssEpochMap.h"
#include "GnssSolution.h"
#include "PODSolution.h"
#include "PPPSolution.h"
#include "Rinex3NavData.hpp"
#include "Rinex3NavHeader.hpp"
#include "Rinex3NavStream.hpp"

#include <list>
#include <regex>
#include <set>

using namespace gnsstk;

namespace pod
{

    PPPSolutionBase::PPPSolutionBase(GnssDataStorePtr procData) : GnssSolution(procData, 1.0) {}
    PPPSolutionBase::~PPPSolutionBase()
    {
        solverPR_.release();
    }

    void PPPSolutionBase::mapSNR(IRinex& rin_epoch)
    {
        for (auto& it1 : rin_epoch.getBody())
        {
            auto ts1 = TypeID(TypeID::S1);
            auto ts2 = TypeID(TypeID::S2);

            auto s1 = it1.second->get_value().find(ts1);
            if (s1 != it1.second->get_value().end())
                s1->second = mapSNR(s1->second);

            auto s2 = it1.second->get_value().find(ts1);
            if (s2 != it1.second->get_value().end())
                s2->second = mapSNR(s2->second);
        }
    }

    void PPPSolutionBase::PRProcess()
    {
        NeillTropModel NeillModel = solverPR_->initTropoModel(nominalPos_, opts().DoY);

        std::cout << "solverType " << solverPR_->getName() << std::endl;

        solverPR_->maskEl = 5;
        solverPR_->ionoType = data_->ionoCorrector.getType();

        std::ofstream os;
        const auto outPath = opts().workingDir / data_->apprPosFile;
        os.open(outPath);

        // decimation
        int sampl(1);
        double tol(0.1);

        for (auto obsFile : data_->getObsFiles(opts().SiteRover))
        {
            int badSol(0);
            std::cout << obsFile << std::endl;
            try
            {
                // Input observation file stream
                Rinex3ObsStream rin;
                // Open Rinex observations file in read-only mode
                rin.open(obsFile, std::ios::in);

                rin.exceptions(std::ios::failbit);
                Rinex3ObsHeader roh;
                Rinex3ObsData rod;

                // read the header
                rin >> roh;
                CommonTime ct0 = roh.firstObs;
                CommonTime Tpre(CommonTime::BEGINNING_OF_TIME);
                Tpre.setTimeSystem(TimeSystem::Any);
                // Let's process all lines of observation data, one by one
                while (rin >> rod)
                {
                    // work around for post header comments
                    if (std::abs(rod.time - Tpre) <= CommonTime::eps)
                        continue;
                    Tpre = rod.time;
                    double dt = rod.time - ct0;
                    const GPSWeekSecond gpst = static_cast<GPSWeekSecond>(rod.time);

                    if (fmod(gpst.getSOW(), sampl) > tol)
                        continue;

                    int GoodSats = 0;
                    int res = 0;
                    CodeProcSvData svData;
                    ///
                    solverPR_->selectObservables(
                        rod, roh, opts().systems, CodeProcSvData::obsTypes, svData);

                    for (auto& it : svData.data)
                        it.second.snr = mapSNR(it.second.snr);

                    svData.applyCNoMask(solverPR_->maskSNR);
                    GoodSats = svData.getNumUsedSv();

                    os << std::setprecision(6);
                    os << CivilTime(gpst).printf("%02Y %02m %02d %02H %02M %02S %P") << " " << dt
                       << " ";

                    if (GoodSats >= 4)
                    {
                        try
                        {
                            solverPR_->prepare(rod.time, data_->navLibrary_, svData);
                            res = solverPR_->solve(rod.time, data_->bceIonoStore, svData);
                            os << res;
                        }
                        catch (Exception& e)
                        {
                            GNSSTK_RETHROW(e)
                        }
                    }
                    else
                        res = -1;

                    os << *solverPR_ << " " << svData << std::endl;

                    if (res == 0)
                    {
                        Xvt xvt;
                        xvt.x = Triple(solverPR_->Sol(0), solverPR_->Sol(1), solverPR_->Sol(2));
                        xvt.clkbias = solverPR_->Sol(3);
                        // data->apprPos.insert(pair<CommonTime, Xvt>(rod.time, xvt));
                    }
                    else
                    {
                        solverPR_->Sol = 0.0;
                        badSol++;
                    }
                }
                rin.close();
            }
            catch (Exception& e)
            {
                std::cerr << e << std::endl;
                GNSSTK_RETHROW(e);
            }
            catch (...)
            {
                std::cerr << "Caught an unexpected exception." << std::endl;
            }
            std::cout << "Number of bad solutions for file " << badSol << std::endl;
        }
    }

} // namespace pod