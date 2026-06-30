#pragma once

#include "EquationBase.h"
#include "FilterParameter.h"
#include "GnssDataStore.hpp"
#include "StateLayout.h"
#include "SatObservationBlocks.h"

#include <memory>

namespace pod
{
    typedef std::unique_ptr<EquationBase> eq_uptr;
    typedef std::vector<eq_uptr> EquationsList;

    class EquationComposer
    {
      public:
        /// values and its covariance processed so far
        struct FilterData
        {
            // Default constructor initializing the data in the structure
            FilterData() : value(0.0) {};

            // value
            double value = 0.0;
            std::map<FilterParameter, double> valCov;
        };
        typedef std::map<FilterParameter, FilterData> FilterState;

        EquationComposer() {};

        EquationComposer(SlnType st) : slnType_(st) {};

        virtual ~EquationComposer() = default;

        // prepare equations according current data set 'gData'
        void prepare(gnsstk::IRinex& gData);

        // compose all system matrices in one call
        void updateSystemMatrices(gnsstk::IRinex& gData,
                                  gnsstk::Matrix<double>& H,
                                  gnsstk::Vector<double>& prefitResiduals,
                                  gnsstk::Matrix<double>& W,
                                  gnsstk::Matrix<double>& Phi,
                                  gnsstk::Matrix<double>& Q);

        // compose current state vector and covariance matrix
        void updateKfState(gnsstk::Vector<double>& state, gnsstk::Matrix<double>& cov) const;

        // store current state vector and covariance matrix
        void storeKfState(const gnsstk::Vector<double>& state, const gnsstk::Matrix<double>& cov);

        // compose current state vector and covariance matrix with default values
        void initKfState(gnsstk::Vector<double>& state, gnsstk::Matrix<double>& cov) const;

        // insert current residuals vector into GNSS data structure
        void saveResiduals(gnsstk::IRinex& gData, const gnsstk::Vector<double>& residuals) const;

        // return postfit residuals as gnsstk::satTypeValueMap structure
        std::vector<double> getResiduals(const gnsstk::Vector<double>& residuals,
                                         const gnsstk::TypeIDSet& types) const;

        // find the row with the largest |residual| among the given postfit types
        ResidualInfo findMaxResidual(const gnsstk::Vector<double>& residuals,
                                     const gnsstk::TypeIDSet& postfitTypes) const;

        // collect the residual rows contributed by a single satellite
        std::set<int> getSatRows(const gnsstk::SatID& sat) const;

        const FilterState& getState() const;
        EquationComposer& setState(const FilterState& newState);

        // get curent number of unknowns
        int getNumUnknowns() const;

        const StateLayout& getLayout() const;

        const ParametersSet& getCurrentAmb() const;

        SlnType getSlnType() const;

        EquationComposer& setSlnType(SlnType sType);

        /// add new equation to equation list
        EquationComposer& addEquation(std::unique_ptr<EquationBase> eq);

        /// erase equation list
        void clearEquations();
        /// erase stored data
        void clearData();
        /// manage satellite-specific data
        void keepOnlySv(const gnsstk::SatIDSet& svs);
        void clearSvData(const gnsstk::SatIDSet& svs);
        void clearSvData();

        size_t getNumSv() const
        {
            return satBlocks_.size();
        }
      private:
        /// Map holding the information regarding every variable
        FilterState filterData_;

        /// list of equations
        EquationsList equations_;

        StateLayout layout_;

        SatObservationBlocks satBlocks_;

        /// current set of ambiguities
        ParametersSet currAmb_;

        // desired solution type
        SlnType slnType_;
    };

    typedef std::shared_ptr<pod::EquationComposer> EquationComposerPtr;
} // namespace pod