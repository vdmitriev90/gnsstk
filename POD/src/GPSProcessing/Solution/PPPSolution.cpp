
#pragma region gpstk includes
#include "BasicModel.hpp"
#include "ComputeDOP.hpp"
#include "ComputeLinear.hpp"
#include "ComputeSatPCenter.hpp"
#include "ComputeTropModel.hpp"
#include "ComputeWindUp.hpp"
#include "CorrectObservables.hpp"
#include "Decimate.hpp"
#include "EclipsedSatFilter.hpp"
#include "GravitationalDelay.hpp"
#include "LICSDetector2.hpp"
#include "LinearCombinations.hpp"
#include "MWCSDetector.hpp"
#include "NavLibrary.hpp"
#include "OceanLoading.hpp"
#include "PhaseCodeAlignment.hpp"
#include "PoleTides.hpp"
#include "PowerSum.hpp"
#include "ProcessingList.hpp"
#include "ObservablesSets.h"
#include "SatArcMarker.hpp"
#include "ObsRangeFilter.h"
#include "SolidTides.hpp"
#include "XYZ2NEU.hpp"

#pragma endregion

// pod includes
#include "CodeSolver.h"
#include "FsUtils.h"
#include "GnssEpochMap.h"
#include "LinearCombination.h"
#include "PPPSolution.h"
#include "ProcessLinear.h"
#include "SolverPPP.hpp"
#include "SolverPPPFB.hpp"

namespace pod
{
    PPPSolution::PPPSolution(GnssDataStorePtr gnssData) : PPPSolutionBase(gnssData)
    {

        // opts().DoY = confReader().getValueAsInt("dayOfYear");

        // initialize troposhperic model
        // tropModel = NeillTropModel(nominalPos_.getAltitude(),
        // nominalPos_.getGeodeticLatitude(),opts().DoY);

        // solverPR_ = unique_ptr<CodeSolverBase>(new CodeSolver(tropModel, data));
    }

    bool PPPSolution::processCore()
    {
        nominalPos_ = data_->getNominalPosition(opts().SiteRover);

        updateRequaredObs();

        // This object will check that code observations are within
        // reasonable limits
        ObsRangeFilter PRFilter({ObsRangeType::FirstCode, ObsRangeType::SecondCode});
        ObsRangeFilter SNRFilter(ObsRangeType::Snr, confReader().getValueAsInt("SNRmask"), DBL_MAX);

        ProcessLinear linear1;
        linear1.add(std::make_unique<PDelta>());
        linear1.add(std::make_unique<MWoubenna>());

        linear1.add(std::make_unique<LDelta>());
        linear1.add(std::make_unique<LICombimnation>());

        // Objects to mark cycle slips
        LICSDetector2 markCSLI2; // Checks LI cycle slips
        markCSLI2.setSatThreshold(confReader().getValueAsDouble("LISatThreshold"));
        MWCSDetector markCSMW; // Checks Merbourne-Wubbena cycle slips
        markCSMW.setMaxNumLambdas(confReader().getValueAsDouble("MWNLambdas"));

        // Object to keep track of satellite arcs
        SatArcMarker markArc;
        markArc.setDeleteUnstableSats(true);
        markArc.setUnstablePeriod(31.0);

        // Object to decimate data
        Decimate decimateData(confReader().getValueAsDouble("decimationInterval"),
                              confReader().getValueAsDouble("decimationTolerance"),
                              data_->navLibrary_.getInitialTime());

        // Declare a basic modeler
        // BasicModel basic(Position(0.0, 0.0, 0.0), SP3EphList);
        BasicModel basic(nominalPos_, data_->navLibrary_);
        // Set the minimum elevation
        basic.setMinElev(opts().maskEl);

        basic.setDefaultObservable(TypeID::C1);

        // Object to remove eclipsed satellites
        EclipsedSatFilter eclipsedSV;

        // Object to compute gravitational delay effects
        GravitationalDelay grDelay(nominalPos_);

        // Vector from monument to antenna ARP [UEN], in meters
        const Triple offset_ARP = confReader().getValueListAsTriple("offsetARP", opts().SiteRover);
        
        AntexReader antexReader;
        Antenna receiverAntenna;

        // Feed Antex reader object with Antex file
        std::string afile = opts().genericFilesDirectory;
        afile += confReader().getValue("antexFile");

        antexReader.open(afile);

        // Get receiver antenna parameters
        receiverAntenna =
            antexReader.getAntenna(confReader().getValue("antennaModel", opts().SiteRover));

        // Object to compute satellite antenna phase center effect
        ComputeSatPCenter svPcenter(nominalPos_);

        // Feed 'ComputeSatPCenter' object with 'AntexReader' object
        svPcenter.setAntexReader(antexReader);

        // Declare an object to correct observables to monument
        CorrectObservables corr(data_->navLibrary_);

        corr.setMonument(offset_ARP);

        // Check if we want to use Antex patterns
        bool usepatterns(confReader().getValueAsBoolean("usePCPatterns", opts().SiteRover));
        if (usepatterns)
        {
            corr.setAntenna(receiverAntenna);
            // Should we use elevation/azimuth patterns or just elevation?
            corr.setUseAzimuth(confReader().getValueAsBoolean("useAzim", opts().SiteRover));
        }

        // Object to compute wind-up effect
        ComputeWindUp windup(data_->navLibrary_,
                             nominalPos_,
                             opts().genericFilesDirectory + confReader().getValue("satDataFile"));

        // Object to compute the tropospheric data
        ComputeTropModel computeTropo(tropModel);

        // Object to compute ionosphere-free combinations to be used
        // as observables in the PPP processing
        ProcessLinear linear2;
        linear2.add(std::make_unique<PCCombination>());
        linear2.add(std::make_unique<LCCombimnation>());

        // Add to processing list
        // Declare a simple filter object to screen PC
        ObsRangeFilter pcFilter;
        pcFilter.setFilteredType(ObsRangeType::IonoFreeCode);

        // IMPORTANT NOTE:
        // Like in the "filterCode" case, the "filterPC" option allows you to
        // deactivate the "ObsRangeFilter" object that filters out PC, in case
        // you need to.

        // Object to align phase with code measurements
        PhaseCodeAlignment phaseAlign;

        LinearCombinations comb;
        // Object to compute prefit-residuals
        ComputeLinear linear3(comb.pcPrefit);
        linear3.addLinear(comb.lcPrefit);

        // Declare a base-changing object: From ECEF to North-East-Up (NEU)
        XYZ2NEU baseChange(nominalPos_);

        // Object to compute DOP values
        ComputeDOP cDOP;

        double tropoQ(confReader().getValueAsDouble("tropoQ"));
        double posSigma(confReader().getValueAsDouble("posSigma"));
        double clkSigma(confReader().getValueAsDouble("clkSigma"));
        double weightFactor(confReader().getValueAsDouble("weightFactor"));

        // estimate receiver linear clock drift parameters together with  clock offset?
        bool useAdvClkModel = confReader().getValueAsBoolean("useAdvClkModel");
        basic.useClkDrift(useAdvClkModel);

        // Declare solver objects
        SolverPPP pppSolver(useAdvClkModel, tropoQ, posSigma, clkSigma, weightFactor);
        SolverPPPFB fbpppSolver(useAdvClkModel, tropoQ, posSigma, clkSigma, weightFactor);

        std::vector<double> phaselims = confReader().getValueListAsDouble("phaseLimlist");
        fbpppSolver.setPhaseList(phaselims);
        std::vector<double> codelims = confReader().getValueListAsDouble("codeLimList");
        fbpppSolver.setCodeList(codelims);
        int cycles(std::max<int>(phaselims.size(), codelims.size()));
        std::cout << "cycles " << cycles << std::endl;

        if (useAdvClkModel)
        {
            double q1clk(confReader().getValueAsDouble("q1Clk"));
            double q2clk(confReader().getValueAsDouble("q2Clk"));
            AdvClockModel mod(q1clk, q2clk);
            pppSolver.setAdvClkModel(mod);
            fbpppSolver.setAdvClkModel(mod);
        }

        // White noise stochastic models
        WhiteNoiseModel wnM(100.0);

        if (opts().dynamics == Dynamics::Kinematic)
        {
            fbpppSolver.setCoordinatesModel(&wnM);
            pppSolver.setCoordinatesModel(&wnM);
        }

        // Objects to compute tidal effects
        SolidTides solid;

        // Configure ocean loading model
        OceanLoading ocean;
        ocean.setFilename(opts().genericFilesDirectory + confReader().getValue("oceanLoadingFile"));

        // This is the GNSS data structure that will hold all the
        // GNSS-related information
        RinexEpoch rin_epoch;

#pragma region Output streams

        // Prepare for printing
        int prec(4);

        std::ofstream outfile;
        outfile.open(opts().workingDir / getFileName(), std::ios::out);

#pragma endregion

        int i = 1;
        std::cout << "First forward processing part started." << std::endl;
        for (auto& obsFile : data_->getObsFiles(opts().SiteRover))
        {
            std::cout << obsFile << std::endl;
            // Input observation file stream
            Rinex3ObsStream rin;

            // Open Rinex observations file in read-only mode
            rin.open(obsFile, std::ios::in);

            rin.exceptions(std::ios::failbit);
            Rinex3ObsHeader roh;

            // read the header
            rin >> roh;
            gMap_.header = roh;

            // set def. interval for basic model object
            basic.setDefaultInterval(roh.interval);

            // Let's process all lines of observation data, one by one
            while (rin >> rin_epoch)
            {
                // work around for post header comments
                if (rin_epoch.getBody().size() == 0)
                    continue;
                //
                rin_epoch.keepOnlySatSystems(opts().systems);

                /// update current time and nominal position
                CommonTime time(rin_epoch.getHeader().epoch);
                apprPos().getPosition(rin_epoch, nominalPos_);

                /// compute pole tide displacment
                auto eop = data_->eopStore.getEOP(MJD(time).mjd, IERSConvention::IERS2010);
                PoleTides pole;
                pole.setXY(eop.xp, eop.yp);

                basic.setRxPosition(nominalPos_);
                grDelay.setNominalPosition(nominalPos_);
                svPcenter.setNominalPosition(nominalPos_);
                windup.setNominalPosition(nominalPos_);
                XYZ2NEU baseChange(nominalPos_);
                tropModel.setAllParameters(time, nominalPos_);
                // Compute solid, oceanic and pole tides effects at this epoch
                Triple tides(solid.getSolidTide(time, nominalPos_)
                             + ocean.getOceanLoading(opts().SiteRover, time)
                             + pole.getPoleTide(time, nominalPos_));

                // Update observable correction object with tides information
                corr.setExtraBiases(tides);
                corr.setNominalPosition(nominalPos_);

                try
                {
                    rin_epoch >> requireObs_;
                    rin_epoch >> PRFilter;
                    rin_epoch >> SNRFilter;
                    rin_epoch >> linear1;
                    rin_epoch >> markCSLI2;
                    rin_epoch >> markCSMW;
                    rin_epoch >> markArc;
                    rin_epoch >> decimateData;
                    rin_epoch >> basic;
                    rin_epoch >> eclipsedSV;
                    rin_epoch >> grDelay;
                    rin_epoch >> svPcenter;
                    rin_epoch >> corr;
                    rin_epoch >> windup;
                    rin_epoch >> computeTropo;
                    rin_epoch >> linear2;
                    rin_epoch >> pcFilter;
                    // rin_epoch >> phaseAlign;
                    rin_epoch >> linear3;
                    // rin_epoch >> baseChange;
                    rin_epoch >> cDOP;

                    if (cycles < 1)
                        rin_epoch >> pppSolver;
                    else
                        rin_epoch >> fbpppSolver;
                }
                catch (DecimateEpoch& d)
                {
                    // If we catch a DecimateEpoch exception, just continue.
                    continue;
                }
                ++i;

                // Check what type of solver we are using
                if (cycles < 1)
                {
                    GnssEpoch ep(rin_epoch.getBody());
                    CommonTime time(rin_epoch.getHeader().epoch);

                    // Let's print to output file the results of this epoch
                    printSolution(outfile, pppSolver, time, ep);

                    // add epoch to results
                    gMap_.data.insert(std::pair<CommonTime, GnssEpoch>(time, ep));
                }
            }

            rin.close();
        }
        std::cout << "First forward processing part finished." << std::endl;

        // Now decide what to do: If solver was a 'forwards-only' version,
        // then we are done and should continue with next station.
        if (cycles < 1)
        {
            outfile.close();
            // We are done with this station. Let's show a message
            std::cout << "Processing finished for station: '" << opts().SiteRover << "'."
                      << std::endl;

            return true;
        }

        // Now, let's do 'forwards-backwards' cycles
        try
        {
            std::cout << "fw-bw bart begin..." << std::endl;
            fbpppSolver.ReProcess();
        }
        catch (Exception& e)
        {
            // If problems arose, issue an message and skip receiver
            std::cerr << "Exception at reprocessing phase: " << e << std::endl;
            std::cerr << "Station '" << opts().SiteRover << "'." << std::endl;

            // Close output file for this station
            outfile.close();

            GNSSTK_RETHROW(e);

        } // End of 'try-catch' block

        // Reprocess is over. Let's finish with the last processing
        // Loop over all data epochs, again, and print results
        while (fbpppSolver.LastProcess(rin_epoch))
        {
            // update current time and nominal position
            GnssEpoch ep(rin_epoch.getBody());
            apprPos().getPosition(rin_epoch, nominalPos_);
            printSolution(outfile, fbpppSolver, rin_epoch.getHeader().epoch, ep);
            gMap_.data.insert(std::pair<CommonTime, GnssEpoch>(rin_epoch.getHeader().epoch, ep));

        } // End of 'while( fbpppSolver.LastProcess(rin_epoch) )'

        std::cout << "Processing finished for station: '" << opts().SiteRover << "'." << std::endl;
        std::cout << "Num. of rejected meas. " << fbpppSolver.getRejectedMeasurements()
                  << std::endl;

        outfile.close();

        return true;
    }

    void PPPSolution::updateRequaredObs()
    {
        requireObs_ = RequireObservablesBuilder(opts().systems).build();
    }

    void PPPSolution::printSolution(std::ofstream& outfile,
                                    const SolverLMS& solver,
                                    const CommonTime& time,
                                    GnssEpoch& gEpoch)
    {
        // Prepare for printing
        outfile << std::fixed << std::setprecision(outputCoordsPrec);

        // Print results
        outfile << static_cast<YDSTime>(time).year << "-"; // Year           - #1
        outfile << static_cast<YDSTime>(time).doy << "-";  // DayOfYear      - #2
        outfile << static_cast<YDSTime>(time).sod << "  "; // SecondsOfDay   - #3
        outfile << std::setprecision(6)
                << (static_cast<YDSTime>(time).doy + static_cast<YDSTime>(time).sod / 86400.0)
                << "  " << std::setprecision(outputCoordsPrec);

        // We add 0.1 meters to 'wetMap' because 'NeillTropModel' sets a
        // nominal value of 0.1 m. Also to get the total we have to add the
        // dry tropospheric delay value
        // ztd - #7
        double wetMap =
            solver.getSolution(TypeID::wetMap) + 0.1 + this->tropModel.dry_zenith_delay();

        gEpoch.slnData.insert(std::pair<TypeID, double>(TypeID::recZTropo, wetMap));

        double x = nominalPos_.X() + solver.getSolution(TypeID::dx); // dx    - #4
        double y = nominalPos_.Y() + solver.getSolution(TypeID::dy); // dy    - #5
        double z = nominalPos_.Z() + solver.getSolution(TypeID::dz); // dz    - #6

        gEpoch.slnData.insert(std::pair<TypeID, double>(TypeID::recX, x));
        gEpoch.slnData.insert(std::pair<TypeID, double>(TypeID::recY, y));
        gEpoch.slnData.insert(std::pair<TypeID, double>(TypeID::recZ, z));

        double varX = solver.getVariance(TypeID::dx); // Cov dx    - #8
        double varY = solver.getVariance(TypeID::dy); // Cov dy    - #9
        double varZ = solver.getVariance(TypeID::dz); // Cov dz    - #10
        double sigma = sqrt(varX + varY + varZ);

        double cdt = solver.getSolution(TypeID::cdt);
        gEpoch.slnData.insert(std::pair<TypeID, double>(TypeID::recCdt, cdt));

        //
        outfile << x << "  " << y << "  " << z << "  " << cdt << " ";

        auto defeq = solver.getDefaultEqDefinition();

        auto itcdtGLO = defeq.body.find(TypeID::recISB_GLN);
        if (defeq.body.find(TypeID::recISB_GLN) != defeq.body.end())
        {
            double cdtGLO = solver.getSolution(TypeID::recISB_GLN);
            gEpoch.slnData.insert(std::pair<TypeID, double>(TypeID::recISB_GLN, cdtGLO));

            outfile << cdtGLO << " ";
        }

        if (defeq.body.find(TypeID::recCdtdot) != defeq.body.end())
        {
            double recCdtdot = solver.getSolution(TypeID::recCdtdot);
            gEpoch.slnData.insert(std::pair<TypeID, double>(TypeID::recCdtdot, recCdtdot));

            outfile << std::setprecision(12) << recCdtdot << " ";
        }

        gEpoch.slnData.insert(std::pair<TypeID, double>(TypeID::sigma, sigma));
        outfile << std::setprecision(6) << wetMap << "  " << sigma << "  ";

        gEpoch.slnData.insert(std::pair<TypeID, double>(TypeID::recSlnType, static_cast<int>(desiredSlnType())));

        outfile << gEpoch.satData.size() << std::endl;
    }

    void PPPSolution::process()
    {
        try
        {
            processCore();
            gMap_.updateMetadata();
        }
        catch (ConfigurationException& conf_exp)
        {
            std::cerr << conf_exp.what() << std::endl;
            throw;
        }
        catch (Exception& gpstk_e)
        {
            GNSSTK_RETHROW(gpstk_e);
        }
        catch (std::exception& std_e)
        {
            std::cerr << std_e.what() << std::endl;
            throw;
        }
    }
} // namespace pod
