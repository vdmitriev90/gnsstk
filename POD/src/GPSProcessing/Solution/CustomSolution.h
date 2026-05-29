#ifndef POD_CUSTOM_SOLUTION_H
#define POD_CUSTOM_SOLUTION_H

#include "GnssDataStore.hpp"
#include "GnssSolution.h"

#include <memory>

namespace pod
{

    class CustomSolution : public GnssSolution
    {
      private:
        static GnssSolutionUPtr Factory(GnssDataStore_sptr dataStore);

      public:
        CustomSolution();

      public:
        CustomSolution(GnssDataStore_sptr dataStore);

      public:
        virtual ~CustomSolution();

#pragma region Methods

      public:
        virtual std::string fileName() const override
        {
            return ptr->fileName();
        }

      public:
        virtual SlnType desiredSlnType() const override
        {
            return ptr->desiredSlnType();
        }

      public:
        virtual GnssSolution& setConfigData(GnssDataStore_sptr dataStore)
        {
            ptr = Factory(dataStore);
            return (*this);
        };

      public:
        virtual void process() override
        {
            ptr->process();
        };

      public:
        virtual GnssEpochMap& getData() override
        {
            return ptr->getData();
        };

      protected:
        virtual GnssDataStore::ProcessOpts& opts() override
        {
            return data_->opts;
        };

      protected:
        virtual void printSolution(const KalmanSolver& solver,
                                   const gnsstk::CommonTime& time,
                                   GnssEpoch& gEpoch) override {
            // ptr->printSolution(of, solver, time, gEpoch)
        };

      protected:
        virtual void updateRequaredObs() override {}

#pragma endregion

      private:
        GnssSolutionUPtr ptr;
    };
} // namespace pod
#endif // !POD_CUSTOM_SOLUTION_H
