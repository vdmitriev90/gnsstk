#pragma once
#include"ProcessingClass.hpp"
namespace pod
{
    class PrefitResCatcher
        :public gnsstk::ProcessingClass
    {
    public:
        PrefitResCatcher() {};
        PrefitResCatcher(const gnsstk::TypeIDSet& types) :resTypes(types)
        {};
        virtual ~PrefitResCatcher() {};

        virtual gnsstk::IRinex & Process(gnsstk::IRinex & gData) override;

        virtual std::string getClassName(void) const override;

        gnsstk::TypeIDSet resTypes;

    };
}
