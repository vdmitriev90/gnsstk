#pragma once
#include "ProcessingClass.hpp"
#include<deque>
#include<map>

namespace pod
{


    class SNRCatcher :
        public gnsstk::ProcessingClass
    {
    public:

        SNRCatcher();
        //
        SNRCatcher(const gnsstk::TypeID & type,
            const gnsstk::TypeID& resType,
            double maxgap, 
            double tresh,
            int maxCount);

        virtual ~SNRCatcher() {};


        /** Returns a gnnsRinex object, adding the new data generated when
        *  calling this object.
        *
        * @param gData    Data object holding the data.
        */
        virtual gnsstk::IRinex& Process(gnsstk::IRinex& gData);

        virtual  gnsstk::SatTypePtrMap& Process(const gnsstk::CommonTime& epoch,
            gnsstk::SatTypePtrMap& gData);

        /** Method that implements the LI cycle slip detection algorithm
        *
        * @param epoch     Time of observations.
        * @param sat       SatID.
        * @param tvMap     Data structure of TypeID and values.
        * @param li        Current LI observation value.
        */
        virtual double getDetection(const gnsstk::CommonTime& epoch,
            const gnsstk::SatID& sat,
            gnsstk::typeValueMap& tvMap,
            double snr);

        /// Returns a string identifying this object.
        virtual std::string getClassName(void) const;

    private:
        /// Type of observable.
        gnsstk::TypeID obsType;

        /// Types of result.
        gnsstk::TypeID resultType1;
        
        /// Maximum buffer size.
        int maxBufferSize;

        /// Minimum size of buffer. It is always set to 5
        static const int minBufferSize;

        /// Maximum interval of time allowed between two successive epochs,
        /// in seconds.
        double deltaTMax;


        /// Saturation threshold to declare cycle slip, in meters.
        double satThreshold;

        struct filterData
        {
            // Default constructor initializing the data in the structure
            filterData()
            {};

            std::deque<gnsstk::CommonTime> epochs; ///< Epochs of previous SNR observables.
            std::deque<double> buffer;  ///< Values of previous SNR observables.
        };
        /// Map holding the information regarding every satellite
        std::map<gnsstk::SatID, filterData> data;

    };
}

