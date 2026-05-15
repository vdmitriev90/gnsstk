#pragma once
#include "NavLibrary.hpp"
#include "Position.hpp"
#include "RinexEpoch.h"

namespace pod
{
    enum ApprPositionSource
    {
        FromConfig = 1,
        ComputeForEachEpoch,
        ComputeForFirstEpoch,
        LoadFromFile,
    };

    std::string getPosSourceString(ApprPositionSource source);

    class IApprPosProvider
    {
      public:
        IApprPosProvider() = default;
        virtual ~IApprPosProvider() = default;

        virtual int getPosition(const gnsstk::IRinex& gRin, gnsstk::Position& pos) = 0;
        virtual ApprPositionSource getSource() const = 0;

      protected:
        static int ComputeApprSol(const gnsstk::IRinex& gRin,
                                  gnsstk::NavLibrary& ephem,
                                  gnsstk::Vector<double>& pos);
    };

    typedef std::unique_ptr<IApprPosProvider> PosProviderUPtr;

    class ComputeApprPos : public IApprPosProvider
    {

      public:
        ComputeApprPos(gnsstk::NavLibrary& eStore, const gnsstk::Position& pos)
            : ephStore(eStore)
            , prePos(pos)
        {
        }
        ComputeApprPos(gnsstk::NavLibrary& eStore) : ephStore(eStore) {}

        // Inherited via IApprPosProvider
        virtual IApprPosProvider& setNominalPosition(const gnsstk::Position& pos)
        {
            prePos = pos;
            return *this;
        }

        virtual gnsstk::Position getNominalPosition() const
        {
            return prePos;
        }

        virtual int getPosition(const gnsstk::IRinex& gRin, gnsstk::Position& pos) override;

        virtual ApprPositionSource getSource() const override
        {
            return ApprPositionSource::ComputeForEachEpoch;
        }

      private:
        gnsstk::NavLibrary& ephStore;

        std::map<gnsstk::CommonTime, gnsstk::Xvt> pvtStore;

        gnsstk::Position prePos;
    };

    class ComputeOnePos : public IApprPosProvider
    {
      public:
        ComputeOnePos(gnsstk::NavLibrary& ephem) : ephStore(ephem), isFirstTime(true) {}

        // Inherited via IApprPosProvider
        virtual int getPosition(const gnsstk::IRinex& gRin, gnsstk::Position& pos) override;

        virtual ApprPositionSource getSource() const override
        {
            return ApprPositionSource::ComputeForFirstEpoch;
        }

      private:
        gnsstk::NavLibrary& ephStore;

        bool isFirstTime;

        gnsstk::Position apprPos;
    };

    class PositionFromFile : public IApprPosProvider
    {

      public:
        PositionFromFile(std::string path)
        {
            loadApprPos(path);
        }

        int getPosition(const gnsstk::IRinex& gRin, gnsstk::Position& pos) override;

        int size()
        {
            return pvtStore.size();
        }
        virtual ApprPositionSource getSource() const override
        {
            return ApprPositionSource::LoadFromFile;
        }

      private:
        bool loadApprPos(const std::string& path);

        std::map<gnsstk::CommonTime, gnsstk::Xvt> pvtStore;
    };

    class ApprPosSimple : public IApprPosProvider
    {

      public:
        ApprPosSimple(const gnsstk::Position& pos) : apprPos(pos) {};
        int getPosition(const gnsstk::IRinex& gRin, gnsstk::Position& pos) override
        {
            pos = apprPos;
            return 0;
        }

        virtual ApprPositionSource getSource() const override
        {
            return ApprPositionSource::FromConfig;
        }

      private:
        gnsstk::Position apprPos;
    };

} // namespace pod