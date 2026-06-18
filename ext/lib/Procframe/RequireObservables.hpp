//============================================================================
//
//  This file is part of GPSTk, the GPS Toolkit.
//
//  The GPSTk is free software; you can redistribute it and/or modify
//  it under the terms of the GNU Lesser General Public License as published
//  by the Free Software Foundation; either version 3.0 of the License, or
//  any later version.
//
//  The GPSTk is distributed in the hope that it will be useful,
//  but WITHOUT ANY WARRANTY; without even the implied warranty of
//  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
//  GNU Lesser General Public License for more details.
//
//  You should have received a copy of the GNU Lesser General Public
//  License along with GPSTk; if not, write to the Free Software Foundation,
//  Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110, USA
//  
//  Copyright 2004, The University of Texas at Austin
//  Dagoberto Salazar - gAGE ( http://www.gage.es ). 2007, 2008, 2011
//
//============================================================================

//============================================================================
//
//This software developed by Applied Research Laboratories at the University of
//Texas at Austin, under contract to an agency or agencies within the U.S. 
//Department of Defense. The U.S. Government retains all rights to use,
//duplicate, distribute, disclose, or release this software. 
//
//Pursuant to DoD Directive 523024 
//
// DISTRIBUTION STATEMENT A: This software has been approved for public 
//                           release, distribution is unlimited.
//
//=============================================================================

/**
 * @file RequireObservables.hpp
 * This class filters out satellites with observations grossly out of bounds.
 */

#ifndef GPSTK_REQUIREOBSERVABLES_HPP
#define GPSTK_REQUIREOBSERVABLES_HPP

#include "ProcessingClass.hpp"

namespace gnsstk
{

    /// @ingroup DataStructures
    //@{

    /** This class filters out satellites that don't have the required
     *  observations.
     *
     * This class is meant to be used with the GNSS data structures objects
     * found in "DataStructures" class.
     *
     * A typical way to use this class follows:
     *
     * @code
     *   RinexObsStream rin("ebre0300.02o");
     *
     *   gnssRinex gRin;
     *
     *   TypeIDSet requiredSet;
     *   requiredSet.insert(TypeID::P1);
     *   requiredSet.insert(TypeID::P2);
     *
     *   RequireObservables requireTypes(requiredSet);
     *
     *   while(rin >> gRin)
     *   {
     *         // Satellites without P1 and P2 observations will be deleted
     *      gRin >> requireTypes;
     *   }
     * @endcode
     *
     * The "RequireObservables" object will visit every satellite in the
     * GNSS data structure that is "gRin" and will check that the previously
     * given list of observation types is meet by each satellite.
     *
     * Be warned that if a given satellite does not have ALL the observations
     * required, the full satellite record will be summarily deleted from the
     * data structure.
     *
     */
    class RequireObservables : public ProcessingClass
    {
      public:
        /// Default constructor.
        RequireObservables() = default;

        explicit RequireObservables(bool keepOnlyRequiredTypes)
            : keepOnlyRequiredTypes_(keepOnlyRequiredTypes)
        {};

        /** Returns a SatTypePtrMap object, checking the required
         *  observables.
         *
         * @param gData     Data object holding the data.
         */
        virtual SatTypePtrMap& Process(SatTypePtrMap& gData);

        /** Method to add a TypeID to be required for a specific satellite system.
         *
         * @param sys       Satellite system.
         * @param type      TypeID to be required.
         */
        virtual RequireObservables& addRequiredType(SatelliteSystem sys, const TypeID& type)
        {
            requiredTypes_[sys].insert(type);
            return (*this);
        };

        /** Method to add a set of TypeID's to be required for a specific satellite system.
         *
         * @param sys       Satellite system.
         * @param typeSet   Set of TypeID's to be required.
         */
        virtual RequireObservables& addRequiredType(SatelliteSystem sys, const TypeIDSet& typeSet)
        {
            requiredTypes_[sys].insert(typeSet.begin(), typeSet.end());
            return (*this);
        };

        /** Method to set a TypeID to be required for a specific satellite system.
         *  This method will erase previous required types for that system.
         *
         * @param sys       Satellite system.
         * @param type      TypeID to be required.
         */
        virtual RequireObservables& setRequiredType(SatelliteSystem sys, const TypeID& type)
        {
            requiredTypes_[sys].clear();
            requiredTypes_[sys].insert(type);
            return *this;
        };

        /** Method to set the TypeID's to be required for a specific satellite system.
         *  This method will erase previous types for that system.
         *
         * @param sys       Satellite system.
         * @param typeSet   Set of TypeID's to be required.
         */
        virtual RequireObservables& setRequiredType(SatelliteSystem sys, const TypeIDSet& typeSet)
        {
            requiredTypes_[sys] = typeSet;
            return (*this);
        };

        /** Method to get the set of TypeID's required for a specific satellite system.
         *
         * @param sys       Satellite system.
         */
        virtual TypeIDSet getRequiredType(SatelliteSystem sys) const
        {
            const auto it = requiredTypes_.find(sys);
            return (it != requiredTypes_.end()) ? it->second : TypeIDSet{};
        };

        /// Method to get the full map of required TypeID's per satellite system.
        virtual std::map<SatelliteSystem, TypeIDSet> getRequiredType() const
        {
            return requiredTypes_;
        };

        /** Returns a gnnsRinex object, checking the required observables.
         *
         * @param gData    Data object holding the data.
         */
        virtual IRinex& Process(IRinex& gData)
        {
            currTime_ = gData.getHeader().epoch;
            Process(gData.getBody());
            return gData;
        };

        /// Returns a string identifying this object.
        virtual std::string getClassName(void) const;

        /// If true, TypeID's not in the required set for a given system will
        /// be stripped from each satellite's data after filtering.
        virtual RequireObservables& setKeepOnlyRequiredTypes(bool flag)
        { keepOnlyRequiredTypes_ = flag; return *this; };

        virtual bool getKeepOnlyRequiredTypes() const
        { return keepOnlyRequiredTypes_; };

        /// Destructor
        virtual ~RequireObservables() {};

      private:
        CommonTime currTime_;

        bool keepOnlyRequiredTypes_ = false;

        /// Map of required TypeID sets per satellite system
        std::map<SatelliteSystem, TypeIDSet> requiredTypes_;

    }; // End of class 'RequireObservables'

    //@}

} // End of namespace gnsstk

#endif // GPSTK_REQUIREOBSERVABLES_HPP