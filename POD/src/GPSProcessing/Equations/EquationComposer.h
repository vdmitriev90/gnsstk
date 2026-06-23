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
        virtual void prepare(gnsstk::IRinex& gData);

        // compose all system matrices in one call
        virtual void updateSystemMatrices(gnsstk::IRinex& gData,
                                          gnsstk::Matrix<double>& H,
                                          gnsstk::Vector<double>& prefitResiduals,
                                          gnsstk::Matrix<double>& W,
                                          gnsstk::Matrix<double>& Phi,
                                          gnsstk::Matrix<double>& Q);

        // compose current state vector and covariance matrix
        virtual void updateKfState(gnsstk::Vector<double>& state, gnsstk::Matrix<double>& cov) const;

        // store current state vector and covariance matrix
        virtual void storeKfState(const gnsstk::Vector<double>& state, const gnsstk::Matrix<double>& cov);

        // compose current state vector and covariance matrix with default values
        virtual void initKfState(gnsstk::Vector<double>& state, gnsstk::Matrix<double>& cov) const;

        // insert current residuals vector into GNSS data structure
        virtual void saveResiduals(gnsstk::IRinex& gData, const gnsstk::Vector<double>& residuals) const;

        // return postfit residuals as gnsstk::satTypeValueMap structure
        virtual std::vector<double> getResiduals(const gnsstk::Vector<double>& residuals,
                                                 const gnsstk::TypeIDSet& types) const;

        virtual const FilterState& getState() const;
        virtual EquationComposer& setState(const FilterState& newState);

        // get curent number of unknowns
        virtual int getNumUnknowns() const;

        const StateLayout& getLayout() const;

        virtual gnsstk::TypeIDSet& getMeasTypes();
        virtual const gnsstk::TypeIDSet& getMeasTypes() const;

        virtual gnsstk::TypeIDSet& getResidTypes();
        virtual const gnsstk::TypeIDSet& getResidTypes() const;

        virtual const ParametersSet& getCurrentAmb() const;

        virtual SlnType getSlnType() const;

        virtual EquationComposer& setSlnType(SlnType sType);

        /// add new equation to equation list
        virtual EquationComposer& addEquation(std::unique_ptr<EquationBase> eq);

        /// erase equation list
        virtual void clearEquations();
        /// erase stored data
        virtual void clearData();
        /// manage satellite-specific data
        virtual void keepOnlySv(const gnsstk::SatIDSet& svs);
        virtual void clearSvData(const gnsstk::SatIDSet& svs);
        virtual void clearSvData();

      private:
        /// Map holding the information regarding every variable
        FilterState filterData_;

        /// list of equations
        EquationsList equations_;

        /// current set of ambiguities
        ParametersSet currAmb_;

        /// type of measurements
        gnsstk::TypeIDSet measurementsTypes_;

        /// type ID of postfit residuals
        gnsstk::TypeIDSet residualsTypes_;

        StateLayout layout_;

        SatObservationBlocks satBlocks_;

        /// number of measurments
        size_t numMeas_;

        // desired solution type
        SlnType slnType_;
    };

    typedef std::shared_ptr<pod::EquationComposer> EquationComposerPtr;
} // namespace pod