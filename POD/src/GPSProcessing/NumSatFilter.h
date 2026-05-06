#pragma once
#include"ProcessingClass.hpp"
#include"GnssDataStore.hpp"

namespace pod
{
    class NumSatFilter
        :public gnsstk::ProcessingClass
    {
    private:
        static std::map<SlnType, int> minSatbySsForSt;
    public:
        NumSatFilter(SlnType slnType) :minSvNum(minSatbySsForSt.at(slnType)) {};
        virtual ~NumSatFilter() {};


        virtual gnsstk::IRinex & Process(gnsstk::IRinex & gData) override;
        
        virtual std::string getClassName(void) const override;

        int minSvNum;

    };

}
