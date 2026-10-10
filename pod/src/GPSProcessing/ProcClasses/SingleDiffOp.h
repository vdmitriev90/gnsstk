#pragma once

#include "ObservationResolver.h"
#include "ProcessingClass.hpp"
#include "SatObservationBlocks.h"

namespace pod
{
    /**
     * Applies single-difference operator to GNSS data.
     *
     * Uses ObservationResolver to resolve which TypeIDs to difference
     * instead of a hardcoded TypeIDSet.
     *
     * Belongs to pod:: namespace to avoid inverse dependency
     * (gnsstk:: should not depend on pod::).
     */
    class SingleDifferenceOp : public gnsstk::ProcessingClass
    {
      public:
        SingleDifferenceOp() = default;

        explicit SingleDifferenceOp(const std::vector<ObsSlot>& slots)
            : slots_(slots)
        {
        }

        explicit SingleDifferenceOp(const gnsstk::SatTypePtrMap& refData,
                                    bool deleteMissingSats = true,
                                    const std::vector<ObsSlot>& slots = {})
            : slots_(slots)
            , refData_(refData)
            , deleteMissingSats_(deleteMissingSats)
        {
        }

        explicit SingleDifferenceOp(const gnsstk::IRinex& gData,
                                    bool deleteMissingSats = true,
                                    const std::vector<ObsSlot>& slots = {})
            : slots_(slots)
            , refData_(gData.getBody())
            , deleteMissingSats_(deleteMissingSats)
        {
        }

        SingleDifferenceOp& setSlots(const std::vector<ObsSlot>& slots)
        {
            slots_ = slots;
            return *this;
        }

        SingleDifferenceOp& setRefData(const gnsstk::SatTypePtrMap& gData)
        {
            refData_ = gData;
            return *this;
        }

        SingleDifferenceOp& setRefData(const gnsstk::IRinex& gData)
        {
            refData_ = gData.getBody();
            return *this;
        }

        SingleDifferenceOp& setDeleteMissingSats(bool v)
        {
            deleteMissingSats_ = v;
            return *this;
        }
        bool getDeleteMissingSats() const
        {
            return deleteMissingSats_;
        }

        SingleDifferenceOp& setUpdateCSFlag(bool v)
        {
            updateCSFlag_ = v;
            return *this;
        }
        bool getUpdateCSFlag() const
        {
            return updateCSFlag_;
        }

        gnsstk::SatTypePtrMap& Process(gnsstk::SatTypePtrMap& gData);

        gnsstk::IRinex& Process(gnsstk::IRinex& gData) override
        {
            Process(gData.getBody());
            return gData;
        }

        std::string getClassName() const override
        {
            return "pod::SingleDifferenceOp";
        }

      private:
        ObservationResolver resolver_;
        std::vector<ObsSlot> slots_;
        gnsstk::SatTypePtrMap refData_;
        bool deleteMissingSats_ = true;
        bool updateCSFlag_ = true;
    };

} // namespace pod
