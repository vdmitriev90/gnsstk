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
        static GnssSolutionUPtr Factory(GnssDataStorePtr dataStore);

      public:
        CustomSolution();

        CustomSolution(GnssDataStorePtr dataStore);

        virtual ~CustomSolution();

#pragma region Methods

        virtual std::string fileName() const override
        {
            return ptr->fileName();
        }

        virtual SlnType desiredSlnType() const override
        {
            return ptr->desiredSlnType();
        }

        virtual GnssSolution& setConfigData(GnssDataStorePtr dataStore)
        {
            ptr = Factory(dataStore);
            return (*this);
        };

        virtual void process() override
        {
            ptr->process();
        };
        virtual GnssEpochMap& getData() override
        {
            return ptr->getData();
        };

        virtual const GnssEpochMap& getData() const override
        {
            return ptr->getData();
        };

      protected:
        virtual GnssDataStore::ProcessOpts& opts() override
        {
            return data_->opts;
        };

        virtual void updateRequaredObs() override {}

#pragma endregion

      private:
        GnssSolutionUPtr ptr;
    };
} // namespace pod
#endif // !POD_CUSTOM_SOLUTION_H
