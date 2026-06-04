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

#define CATCH_TIME(t, Y, m, d, hh, mm, ss, flag)                                                   \
    static auto desiredTime =                                                                      \
        (gnsstk::CommonTime)gnsstk::CivilTime(Y, m, d, hh, mm, ss, gnsstk::TimeSystem::Any);       \
    flag = t == desiredTime;

namespace pod
{
    // base class for all GNSS post processing  classes
    class GnssSolution
    {
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

        virtual std::string fileName() const = 0;

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

        // L1 code measurements used for position computation (typical C1 and P1)
        gnsstk::TypeID codeL1_;

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

#pragma endregion
    };
    typedef std::unique_ptr<GnssSolution> GnssSolutionUPtr;
} // namespace pod
#endif // !POD_GNSS_SOLUTION_H
