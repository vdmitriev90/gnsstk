#pragma once

namespace pod {
    //
    enum class CarrierBand
    {
        cbUndefined = 0,
        L1,
        L2,
        L5
    };

    enum class TropoModelType
    {
        Undefined = 0,
        Simple = 1,
        SimpleWithGradients,
        Advanced
    };

    // Desired type of GNSS solution
    enum class SlnType
    {
        None = 0,
        Standalone = 1,
        CodeDiff = 2,
        PdFloat = 3,
        PdFixed = 4,
        PppFloat = 16,
        PppFixed = 17,
    };

    // Receiver dynamic mode
    enum class Dynamics
    {
        Undefined = 0,
        Kinematic = 1,
        Static,
        RandomWalk,
        Spaceborne,
    };

    enum class ObsSlot
    {
        Undefined = 0,
        FirstBandCode,
        FirstBandPhase,
        SecondBandCode,
        SecondBandPhase,
        CodeIonoFree, // iono-free code
        PhaseIonoFree // iono-free phase
    };
}