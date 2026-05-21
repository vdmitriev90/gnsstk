#pragma once
#include "DataStructures.hpp"
#include "RinexEpoch.h"
#include "SatID.hpp"
#include "TypeID.hpp"

#include <set>
namespace pod
{
    typedef std::map<gnsstk::SatelliteSystem, gnsstk::SatIDSet> System2SatSetMap;

    class FilterParameter
    {
      public:
        static System2SatSetMap get_sv_by_ss(const std::set<FilterParameter>& parameters);

        static gnsstk::SatIDSet get_all_sv(const std::set<FilterParameter>& parameters);

        static gnsstk::SatSystSet get_all_ss(const std::set<FilterParameter>& parameters);

        static gnsstk::TypeIDSet get_all_types(const std::set<FilterParameter>& parameters);

        FilterParameter() : type(gnsstk::TypeID::Unknown), sv(gnsstk::SatID::dummy) {};

        FilterParameter(const gnsstk::TypeID& obsType, const gnsstk::SatID& sat)
            : type(obsType)
            , sv(sat) {};

        FilterParameter(const gnsstk::TypeID& obsType) : type(obsType), sv(gnsstk::SatID::dummy) {};

        FilterParameter(const FilterParameter& parameter)
            : type(parameter.type)
            , sv(parameter.sv) {};

        ~FilterParameter() {};

        FilterParameter& operator=(const FilterParameter& other)
        {
            // check for self-assignment
            if (&other == this)
                return *this;

            this->sv = other.sv;
            this->type = other.type;

            return *this;
        }

        inline bool operator==(const FilterParameter& parameter) const
        {
            return (this->type == parameter.type && this->sv == parameter.sv);
        }

        std::string toString() const;

        gnsstk::TypeID type;

        gnsstk::SatID sv;
    };

    inline bool operator<(const FilterParameter& parameter1, const FilterParameter& parameter2)
    {
        // first, compare the types
        // then, compare the satellites
        if (parameter1.type == parameter2.type)
            return parameter1.sv < parameter2.sv;
        else
            return parameter1.type < parameter2.type;
    }
    inline std::ostream& operator<<(std::ostream& os, const FilterParameter& par)
    {
        os << par.type;
        if (par.sv.isValid())
            os << ' ' << par.sv;

        return os;
    }

    typedef std::set<FilterParameter> ParametersSet;
} // namespace pod
