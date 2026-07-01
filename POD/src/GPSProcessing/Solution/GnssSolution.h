#ifndef POD_GNSS_SOLUTION_H
#define POD_GNSS_SOLUTION_H

#include "ComputeLinear.hpp"
#include "EquationComposer.h"
#include "GnssDataStore.hpp"
#include "GnssEpochMap.h"
#include "KalmanSolver.h"
#include "ProcessLinear.h"
#include "RequireObservables.hpp"
#include "SQLiteAdapter.h"
#include "SatelliteSystem.hpp"
#include "CommonEnums.h"

#define CATCH_TIME(t, Y, m, d, hh, mm, ss, flag)                                                   \
    static auto desiredTime =                                                                      \
        (gnsstk::CommonTime)gnsstk::CivilTime(Y, m, d, hh, mm, ss, gnsstk::TimeSystem::Any);       \
    flag = t == desiredTime;

namespace pod
{
    class GnssSolution;

    // Helper class to build file names for GnssSolution and its descendants
    class FileNameBuilder
    {
      public:
        explicit FileNameBuilder(const GnssSolution& solution) : solution_(solution) {}

        std::string getFileName() const;

      private:
        // Format: SiteBase-SiteRover_SlnType_System1_System2...
        std::string buildWithBaseAndRover() const;

        // Format: SiteRover_SlnType System1 System2...
        std::string buildRoverOnly() const;

        const GnssSolution& solution_;
    };

    // base class for all GNSS post processing  classes
    class GnssSolution
    {
        friend class FileNameBuilder;

      public:
        static std::ostream& printMsg(const gnsstk::CommonTime& time, const char* msg);

#pragma region Constructors

        GnssSolution(GnssDataStorePtr dataStore, double maxsigma);

        virtual ~GnssSolution();

#pragma endregion

#pragma region Methods

        virtual void process() = 0;

        virtual GnssEpochMap& getData()
        {
            return gMap_;
        };

        virtual const GnssEpochMap& getData() const
        {
            return gMap_;
        };

        virtual std::string getFileName() const
        {
            return FileNameBuilder(*this).getFileName();
        }

        virtual SlnType desiredSlnType() const = 0;

        virtual double getMaxSigma() const
        {
            return maxSigma_;
        }

        virtual GnssSolution& setMaxSigma(double sigma)
        {
            maxSigma_ = sigma;
            return (*this);
        }

        virtual GnssSolution& setConfigData(GnssDataStorePtr dataStore)
        {
            data_ = dataStore;
            return (*this);
        };

      protected:
        virtual gnsstk::ConfDataReader& confReader()
        {
            return *(data_->confReader);
        }

        virtual GnssDataStore::ProcessOpts& opts()
        {
            return data_->opts;
        }

        virtual IApprPosProvider& apprPos()
        {
            return *data_->apprPos;
        }

        virtual GnssDataStore::ProcessOpts& opts() const
        {
            return data_->opts;
        }

        virtual void updateRequaredObs() = 0;

        void printSolution(const KalmanSolver& slr,
                           const gnsstk::CommonTime& t,
                           GnssEpoch& ep) const;

        virtual void storeReceiverParams(const KalmanSolver& solver,
                                         const FilterParameter& param,
                                         GnssEpoch& ep) const;

        virtual void storeSatelliteParams(const KalmanSolver& solver,
                                          const FilterParameter& param,
                                          GnssEpoch& ep) const;

      private:
        void computeAndStoreSolution(const KalmanSolver& solver, GnssEpoch& gEpoch) const;

#pragma endregion

#pragma region Fields
      protected:
        // Input processing data and configuration
        GnssDataStorePtr data_;

        // Nominal position
        gnsstk::Position nominalPos_;

        // Processing result
        GnssEpochMap gMap_;

        // This object will filter out satellites, which doesn't meet  predefined required
        // observables set
        gnsstk::RequireObservables requireObs_;

        // object to compute prefit residuals
        ProcessLinear oMinusC_;

        // equation System composer
        EquationComposerPtr equations_;

        // number of forward-backward cycles
        int forwardBackwardCycles_;

        // object to compute linear combinations
        ProcessLinear computeLinear_;

        // max sigma
        double maxSigma_;

        ProcessingConfig config_;

#pragma endregion
    };
    typedef std::unique_ptr<GnssSolution> GnssSolutionUPtr;
} // namespace pod
#endif // !POD_GNSS_SOLUTION_H
