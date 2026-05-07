#pragma once

#include"TypeID.hpp"

namespace pod
{
    class PrSmoother
    {
    public:
        PrSmoother();
        PrSmoother(const std::list<gnsstk::TypeID>& tList, int l);
        ~PrSmoother();

        void smooth(const char * path);
    private:
        std::list<gnsstk::TypeID>  codes;
        int window;
    };
}
