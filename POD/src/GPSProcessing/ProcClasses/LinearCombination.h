#ifndef POD_LINEAR_COMBINATIONS_H
#define POD_LINEAR_COMBINATIONS_H
#include "DataStructures.hpp"

#include <iostream>
#include <memory>

using namespace gnsstk;

namespace pod
{
    class ObservationTypesProvider;
    using ObsTypesProviderPtr = std::shared_ptr<ObservationTypesProvider>;
    TypeID getPrefitObsTypeByObsType(const TypeID& originalType);

    class ObservationTypesProvider
    {
      public:
        static std::shared_ptr<ObservationTypesProvider> instance();

        ObservationTypesProvider() = default;

        TypeID getFirstCodeType(SatelliteSystem ss) const;
        TypeID getSecondCodeType(SatelliteSystem ss) const;
        TypeID getFirstPhaseType(SatelliteSystem ss) const;
        TypeID getSecondPhaseType(SatelliteSystem ss) const;
    };

    class LinearCombination
    {
      public:
        static double getIonoFreeWaveLength(const gnsstk::SatID& sv, int band1, int band2);

        LinearCombination() : obsTypesProvider_(ObservationTypesProvider::instance()) {};
        virtual ~LinearCombination() = default;
        bool getCombination(const SatID& sv, const gnssRinex& rin_epoch, double& value) const
        {
            return getCombination(sv, rin_epoch.body, value);
        }

        bool getCombination(const SatID& sv, const satTypeValueMap& gData, double& value) const
        {
            value = NAN;
            auto tvm = gData.find(sv);
            if (tvm == gData.end())
                return false;

            return getCombination(sv, tvm->second, value);
        }

        virtual bool getCombination(const SatID& sv,
                                    const typeValueMap& tvMap,
                                    double& value) const = 0;

        virtual TypeID getType(SatelliteSystem ss) const = 0;

        ObsTypesProviderPtr getObsTypesProvider() const
        {
            return obsTypesProvider_;
        }

        void setObsTypesProvider(ObsTypesProviderPtr provider)
        {
            obsTypesProvider_ = provider;
        }

      protected:
        std::optional<double> getIonoFreePhaseWaveLength(const SatID& sv) const;
        std::optional<double> getFirstFreqWaveLength(const SatID& sv) const;
        std::optional<double> getSecondFreqWaveLength(const SatID& sv) const;

        ObsTypesProviderPtr obsTypesProvider_;
    };

    class MWoubenna : public LinearCombination
    {
      public:
        MWoubenna() {};
        virtual ~MWoubenna() {};
        virtual bool getCombination(const SatID& sv,
                                    const typeValueMap& tvMap,
                                    double& value) const override;
        virtual TypeID getType(SatelliteSystem ss) const override;
    };

    class PDelta : public LinearCombination
    {
      public:
        PDelta() {};
        virtual ~PDelta() {};
        virtual bool getCombination(const SatID& sv,
                                    const typeValueMap& tvMap,
                                    double& value) const override;
        virtual TypeID getType(SatelliteSystem ss) const override;
    };

    class LDelta : public LinearCombination
    {
      public:
        LDelta() {};
        virtual ~LDelta() {};
        virtual bool getCombination(const SatID& sv,
                                    const typeValueMap& tvMap,
                                    double& value) const override;
        virtual TypeID getType(SatelliteSystem ss) const override;
    };

    class PCCombination : public LinearCombination
    {
      public:
        PCCombination() {};
        virtual ~PCCombination() {};
        virtual bool getCombination(const SatID& sv,
                                    const typeValueMap& tvMap,
                                    double& value) const override;
        virtual TypeID getType(SatelliteSystem ss) const override;
    };

    class LCCombimnation : public LinearCombination
    {
      public:
        LCCombimnation() {};
        virtual ~LCCombimnation() {};
        virtual bool getCombination(const SatID& sv,
                                    const typeValueMap& tvMap,
                                    double& value) const override;
        virtual TypeID getType(SatelliteSystem ss) const override;
    };

    class LICombimnation : public LinearCombination
    {
      public:
        LICombimnation() {};
        virtual ~LICombimnation() {};
        virtual bool getCombination(const SatID& sv,
                                    const typeValueMap& tvMap,
                                    double& value) const override;
        virtual TypeID getType(SatelliteSystem ss) const override;
    };

    class PrefitCode : public LinearCombination
    {
      public:
        PrefitCode() = delete;
        PrefitCode(bool isPreciseModel) : isPrecise(isPreciseModel) {};

      protected:
        bool isPrecise;
    };

    class PrefitC1 : public PrefitCode
    {
      public:
        PrefitC1() = delete;
        PrefitC1(bool isPreciseModel) : PrefitCode(isPreciseModel) {};

        virtual ~PrefitC1() {};
        virtual bool getCombination(const SatID& sv,
                                    const typeValueMap& tvMap,
                                    double& value) const override;
        virtual TypeID getType(SatelliteSystem ss) const override;
    };

    class PrefitC2 : public PrefitCode
    {
      public:
        PrefitC2() = delete;
        PrefitC2(bool isPreciseModel) : PrefitCode(isPreciseModel) {};
        virtual ~PrefitC2() {};
        virtual bool getCombination(const SatID& sv,
                                    const typeValueMap& tvMap,
                                    double& value) const override;
        virtual TypeID getType(SatelliteSystem ss) const override;
    };

    class PrefitPC : public PrefitCode
    {
      public:
        PrefitPC() = delete;
        PrefitPC(bool isPreciseModel) : PrefitCode(isPreciseModel) {};
        virtual ~PrefitPC() {};
        virtual bool getCombination(const SatID& sv,
                                    const typeValueMap& tvMap,
                                    double& value) const override;
        virtual TypeID getType(SatelliteSystem ss) const override;
    };

    class PrefitLC : public LinearCombination
    {
      public:
        PrefitLC() {};
        virtual ~PrefitLC() {};
        virtual bool getCombination(const SatID& sv,
                                    const typeValueMap& tvMap,
                                    double& value) const override;
        virtual TypeID getType(SatelliteSystem ss) const override;
    };

    class PrefitL1 : public LinearCombination
    {
      public:
        PrefitL1() {};
        virtual ~PrefitL1() {};
        virtual bool getCombination(const SatID& sv,
                                    const typeValueMap& tvMap,
                                    double& value) const override;
        virtual TypeID getType(SatelliteSystem ss) const override;
    };

    class PrefitL2 : public LinearCombination
    {
      public:
        PrefitL2() {};
        virtual ~PrefitL2() {};
        virtual bool getCombination(const SatID& sv,
                                    const typeValueMap& tvMap,
                                    double& value) const override;
        virtual TypeID getType(SatelliteSystem ss) const override;
    };

    class CodeIonoDelayL1 : public LinearCombination
    {
      public:
        CodeIonoDelayL1() {};
        virtual ~CodeIonoDelayL1() {};
        virtual bool getCombination(const SatID& sv,
                                    const typeValueMap& tvMap,
                                    double& value) const override;
        virtual TypeID getType(SatelliteSystem ss) const override;
    };

    class PhaseIonoDelayL1 : public LinearCombination
    {
      public:
        PhaseIonoDelayL1() {};
        virtual ~PhaseIonoDelayL1() {};
        virtual bool getCombination(const SatID& sv,
                                    const typeValueMap& tvMap,
                                    double& value) const override;
        virtual TypeID getType(SatelliteSystem ss) const override;
    };
} // namespace pod

#endif // !POD_LINEAR_COMBINATIONS_H