#ifndef POD_LINEAR_COMB_LIST_H
#define POD_LINEAR_COMB_LIST_H
#include "DataStructures.hpp"
#include "LinearCombination.h"
#include "ProcessingClass.hpp"

#include <list>
#include <memory>
typedef std::unique_ptr<pod::LinearCombination> linear_ptr;

namespace pod
{
    class ProcessLinear : public gnsstk::ProcessingClass
    {
      public:
        ProcessLinear() : useC1(false) {};
        virtual ~ProcessLinear() {};
        void setUseC1(bool useC1);

        bool getUseC1() const
        {
            return useC1;
        }

        virtual gnsstk::IRinex& Process(gnsstk::IRinex& gData) override;

        virtual std::string getClassName(void) const override
        {
            return "ProcessLinear";
        }

        ProcessLinear& clear()
        {
            combs.clear();
            return (*this);
        }

        ProcessLinear& add(linear_ptr comb)
        {
            combs.push_back(std::move(comb));
            return (*this);
        }

      protected:
        std::list<linear_ptr> combs;

        gnsstk::SatTypePtrMap& Process(gnsstk::SatTypePtrMap& gData);

        bool useC1;
    };
} // namespace pod

#endif // !POD_LINEAR_COMB_LIST_H