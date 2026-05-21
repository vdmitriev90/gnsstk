#ifndef POD_PPP_SOLVER_LEO_H
#define POD_PPP_SOLVER_LEO_H

#include "CodeKalmanSolver.hpp"
#include "ComputeDOP.hpp"
#include "GnssEpochMap.h"
#include "PhaseAmbiguityModel.hpp"
#include "Position.hpp"
#include "PowerSum.hpp"

#include <iostream>

/**
 * This class computes the Precise Point Positioning (PPP) solution
 * GPS reseiver located on  LEO satellite  using a Kalman solver that
 * combines ionosphere-free code and phase   measurements.
 *
 * This class is based on gnsstk::SolverPPP class
 */
namespace pod
{

    class PPPSolverLEO : public gnsstk::CodeKalmanSolver
    {
      public:
        /** Common constructor.
         *
         * @param useNEU   If true, will compute dLat, dLon, dH coordinates;
         *                 if false (the default), will compute dx, dy, dz.
         */
        PPPSolverLEO(bool useNEU = false);

        /** Compute the PPP Solution of the given equations set.
         *
         * @param prefitResiduals   Vector of prefit residuals
         * @param designMatrix      Design matrix for the equation system
         * @param weightMatrix      Matrix of weights
         *
         * \warning A typical Kalman filter works with the measurements noise
         * covariance matrix, instead of the matrix of weights. Beware of this
         * detail, because this method uses the later.
         *
         * @return
         *  0 if OK
         *  -1 if problems arose
         */
        virtual int Compute(const gnsstk::Vector<double>& prefitResiduals,
                            const gnsstk::Matrix<double>& designMatrix,
                            const gnsstk::Matrix<double>& weightMatrix);

        /** Compute the PPP Solution of the given equations set.
         *
         * @param prefitResiduals   Vector of prefit residuals
         * @param designMatrix      Design matrix for the equation system
         * @param weightVector      Vector of weights assigned to each
         *                          satellite.
         *
         * \warning A typical Kalman filter works with the measurements noise
         * covariance matrix, instead of the vector of weights. Beware of this
         * detail, because this method uses the later.
         *
         * @return
         *  0 if OK
         *  -1 if problems arose
         */
        virtual int Compute(const gnsstk::Vector<double>& prefitResiduals,
                            const gnsstk::Matrix<double>& designMatrix,
                            const gnsstk::Vector<double>& weightVector);

        /** Returns a reference to a gnnsRinex object after solving
         *  the previously defined equation system.
         *
         * @param gData    Data object holding the data.
         */
        virtual gnsstk::IRinex& Process(gnsstk::IRinex& gData);

        /** Resets the PPP internal Kalman filter.
         *
         * @param newState         System state vector
         * @param newErrorCov      Error covariance matrix
         *
         * \warning Take care of dimensions: In this case newState must be 6x1
         * and newErrorCov must be 6x6.
         *
         */
        virtual PPPSolverLEO& Reset(const gnsstk::Vector<double>& newState,
                                    const gnsstk::Matrix<double>& newErrorCov)
        {
            kFilter.Reset(newState, newErrorCov);
            return (*this);
        };

        /** Sets if a NEU system will be used.
         *
         * @param useNEU  Boolean value indicating if a NEU system will
         *                be used
         *
         */
        virtual PPPSolverLEO& setNEU(bool useNEU);

        /** Get the weight factor multiplying the phase measurements sigmas.
         *  This factor is the code_sigma/phase_sigma ratio.
         */
        virtual double getWeightFactor(void) const
        {
            return std::sqrt(weightFactor);
        };

        /** Set the weight factor multiplying the phase measurement sigma
         *
         * @param factor      Factor multiplying the phase measurement sigma
         *
         * \warning This factor should be the code_sigma/phase_sigma ratio.
         * For instance, if we assign a code sigma of 1 m and a phase sigma
         * of 10 cm, the ratio is 100, and so should be "factor".
         */
        virtual PPPSolverLEO& setWeightFactor(double factor)
        {
            weightFactor = (factor * factor);
            return (*this);
        };

        /// Get stochastic model pointer for dx (or dLat) coordinate
        gnsstk::IStochasticModel* getXCoordinatesModel() const
        {
            return pCoordXStoModel;
        };

        /** Set coordinates stochastic model for dx (or dLat) coordinate
         *
         * @param pModel      Pointer to gnsstk::IStochasticModel associated with
         *                    dx (or dLat) coordinate.
         */
        PPPSolverLEO& setXCoordinatesModel(gnsstk::IStochasticModel* pModel)
        {
            pCoordXStoModel = pModel;
            return (*this);
        };

        /// Get stochastic model pointer for dy (or dLon) coordinate
        gnsstk::IStochasticModel* getYCoordinatesModel() const
        {
            return pCoordYStoModel;
        };

        /** Set coordinates stochastic model for dy (or dLon) coordinate
         *
         * @param pModel      Pointer to gnsstk::IStochasticModel associated with
         *                    dy (or dLon) coordinate.
         */
        PPPSolverLEO& setYCoordinatesModel(gnsstk::IStochasticModel* pModel)
        {
            pCoordYStoModel = pModel;
            return (*this);
        };

        /// Get stochastic model pointer for dz (or dH) coordinate
        gnsstk::IStochasticModel* getZCoordinatesModel() const
        {
            return pCoordZStoModel;
        };

        /** Set coordinates stochastic model for dz (or dH) coordinate
         *
         * @param pModel      Pointer to gnsstk::IStochasticModel associated with
         *                    dz (or dH) coordinate.
         */
        PPPSolverLEO& setZCoordinatesModel(gnsstk::IStochasticModel* pModel)
        {
            pCoordZStoModel = pModel;
            return (*this);
        };

        /** Set a single coordinates stochastic model to ALL coordinates.
         *
         * @param pModel      Pointer to gnsstk::IStochasticModel associated with
         *                    coordinates.
         *
         * @warning Do NOT use this method to set the SAME state-aware
         * stochastic model (like RandomWalkModel, for instance) to ALL
         * coordinates, because the results will certainly be erroneous. Use
         * this method only with non-state-aware stochastic models like
         * 'gnsstk::IStochasticModel' (constant coordinates) or 'WhiteNoiseModel'.
         */
        virtual PPPSolverLEO& setCoordinatesModel(gnsstk::IStochasticModel* pModel);

        /// Get receiver clock stochastic model pointer
        virtual gnsstk::IStochasticModel* getReceiverClockModel(void) const
        {
            return pClockStoModel;
        };

        /** Set receiver clock stochastic model
         *
         * @param pModel      Pointer to gnsstk::IStochasticModel associated with
         *                    receiver clock.
         *
         * \warning Be aware that some stochastic models store their internal
         * state (for instance, 'RandomWalkModel' and 'PhaseAmbiguityModel').
         * If that is your case, you MUST NOT use the SAME model in DIFFERENT
         * solver objects.
         */
        virtual PPPSolverLEO& setReceiverClockModel(gnsstk::IStochasticModel* pModel)
        {
            pClockStoModel = pModel;
            return (*this);
        };

        /// Get phase biases stochastic model pointer
        virtual gnsstk::IStochasticModel* getPhaseBiasesModel(void) const
        {
            return pBiasStoModel;
        };

        /** Set phase biases stochastic model.
         *
         * @param pModel      Pointer to gnsstk::IStochasticModel associated with
         *                    phase biases.
         *
         * \warning Be aware that some stochastic models store their internal
         * state (for instance, 'RandomWalkModel' and 'PhaseAmbiguityModel').
         * If that is your case, you MUST NOT use the SAME model in DIFFERENT
         * solver objects.
         *
         * \warning This method should be used with caution, because model
         * must be of PhaseAmbiguityModel class in order to make sense.
         */
        virtual PPPSolverLEO& setPhaseBiasesModel(gnsstk::IStochasticModel* pModel)
        {
            pBiasStoModel = pModel;
            return (*this);
        };

        /// Get the State Transition Matrix (phiMatrix)
        virtual gnsstk::Matrix<double> getPhiMatrix(void) const
        {
            return phiMatrix;
        };

        /** Set the State Transition Matrix (phiMatrix)
         *
         * @param pMatrix     State Transition matrix.
         *
         * \warning Process() methods set phiMatrix and qMatrix according to
         * the stochastic models already defined. Therefore, you must use
         * the Compute() methods directly if you use this method.
         *
         */
        virtual PPPSolverLEO& setPhiMatrix(const gnsstk::Matrix<double>& pMatrix)
        {
            phiMatrix = pMatrix;
            return (*this);
        };

        /// Get the Noise covariance matrix (QMatrix)
        virtual gnsstk::Matrix<double> getQMatrix(void) const
        {
            return qMatrix;
        };

        /** Set the Noise Covariance Matrix (QMatrix)
         *
         * @param pMatrix     Noise Covariance matrix.
         *
         * \warning Process() methods set phiMatrix and qMatrix according to
         * the stochastic models already defined. Therefore, you must use
         * the Compute() methods directly if you use this method.
         *
         */
        virtual PPPSolverLEO& setQMatrix(const gnsstk::Matrix<double>& pMatrix)
        {
            qMatrix = pMatrix;
            return (*this);
        };

        /** Set the positioning mode, kinematic or static.
         */
        virtual PPPSolverLEO& setKinematic(bool kinematicMode = true,
                                           double sigmaX = 100.0,
                                           double sigmaY = 100.0,
                                           double sigmaZ = 100.0);

        void printSolution(std::ofstream& outfile,
                           const gnsstk::CommonTime& time0,
                           const gnsstk::CommonTime& time,
                           const gnsstk::ComputeDOP& cDOP,
                           GnssEpoch& gEpoch,
                           double PCO,
                           std::vector<gnsstk::PowerSum>& stats,
                           const gnsstk::Position& nomXYZ);

        /// Returns a string identifying this object.
        virtual std::string getClassName(void) const;

        /// Destructor.
        virtual ~PPPSolverLEO() {};

      private:
        /// Number of variables
        int numVar;

        /// Number of unknowns
        int numUnknowns;

        /// Number of measurements
        int numMeas;

        /// Weight factor for phase measurements
        double weightFactor;

        /// Pointer to stochastic model for dx (or dLat) coordinate
        gnsstk::IStochasticModel* pCoordXStoModel;

        /// Pointer to stochastic model for dy (or dLon) coordinate
        gnsstk::IStochasticModel* pCoordYStoModel;

        /// Pointer to stochastic model for dz (or dH) coordinate
        gnsstk::IStochasticModel* pCoordZStoModel;

        /// Pointer to stochastic model for receiver clock
        gnsstk::IStochasticModel* pClockStoModel;

        /// Pointer to stochastic model for phase biases
        gnsstk::IStochasticModel* pBiasStoModel;

        /// State Transition Matrix (PhiMatrix)
        gnsstk::Matrix<double> phiMatrix;

        /// Noise covariance matrix (QMatrix)
        gnsstk::Matrix<double> qMatrix;

        /// Geometry matrix
        gnsstk::Matrix<double> hMatrix;

        /// Weights matrix
        gnsstk::Matrix<double> rMatrix;

        /// Measurements vector (Prefit-residuals)
        gnsstk::Vector<double> measVector;

        /// Boolean indicating if this filter was run at least once
        bool firstTime;

        /// Set with all satellites being processed this epoch
        gnsstk::SatIDSet satSet;

        /// A structure used to store Kalman filter data.
        struct coreFilterData
        {
            // Default constructor initializing the data in the structure
            coreFilterData() : ambiguity(0.0) {};

            double ambiguity;                         ///< Ambiguity value.
            std::map<gnsstk::TypeID, double> vCovMap; ///< Variables covariance values.
            std::map<gnsstk::SatID, double> aCovMap;  ///< Ambiguities covariance values.
        };

        /// Map holding the information regarding every satellite
        std::map<gnsstk::SatID, coreFilterData> KalmanData;

        /// General Kalman filter object
        gnsstk::SimpleKalmanFilter kFilter;

        /// Initializing method.
        void Init(void);

        /// Constant stochastic model
        gnsstk::ConstantModel constantModel;

        /// White noise stochastic model for position
        gnsstk::WhiteNoiseModel whitenoiseModelX;
        gnsstk::WhiteNoiseModel whitenoiseModelY;
        gnsstk::WhiteNoiseModel whitenoiseModelZ;

        /// Random Walk stochastic model
        gnsstk::RandomWalkModel rwalkModel;

        /// White noise stochastic model
        gnsstk::WhiteNoiseModel whitenoiseModel;

        /// Phase biases stochastic model (constant + white noise)
        gnsstk::PhaseAmbiguityModel biasModel;

        virtual int Compute(const gnsstk::Vector<double>& prefitResiduals,
                            const gnsstk::Matrix<double>& designMatrix)
        {
            return 0;
        };

        virtual PPPSolverLEO& setDefaultEqDefinition(const gnsstk::gnssEquationDefinition& eqDef)
        {
            return (*this);
        };

    }; // End of class 'SolverPPP'
} // namespace pod
  //@}
#endif // !POD_PPP_SOLVER_LEO_H
