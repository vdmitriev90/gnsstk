//============================================================================
//
//  Unit tests for SolidTides (IERS Conventions 2010, section 7.1.1).
//
//  Test vectors are taken from the IERS routine DEHANTTIDEINEL.F.
//
//============================================================================

#include "CivilTime.hpp"
#include "SolidTides.hpp"
#include "TestUtil.hpp"
#include "TimeSystem.hpp"

#include <iomanip>
#include <iostream>
#include <string>
#include <vector>

using namespace gnsstk;

namespace
{
    struct SolidTideTestCase
    {
        const char* name;

        Triple xsta;
        Triple xsun;
        Triple xmon;

        int year;
        int month;
        int day;
        double fhr;

        Triple expected;
    };

    void printResult(const SolidTideTestCase& testCase, const Triple& computed)
    {
        const Triple difference(
            computed[0] - testCase.expected[0], computed[1] - testCase.expected[1], computed[2] - testCase.expected[2]);

        std::cout << std::scientific << std::setprecision(7);

        std::cout << '\n' << testCase.name << '\n';

        std::cout << "  computed [m] : " << computed[0] << "  " << computed[1] << "  " << computed[2] << '\n';

        std::cout << "  expected [m] : " << testCase.expected[0] << "  " << testCase.expected[1] << "  "
                  << testCase.expected[2] << '\n';

        std::cout << "  difference [m]: " << difference[0] << "  " << difference[1] << "  " << difference[2] << '\n';

        std::cout << "  difference [mm]: " << difference[0] * 1000.0 << "  " << difference[1] * 1000.0 << "  "
                  << difference[2] * 1000.0 << '\n';
    }

    CommonTime makeEpoch(const SolidTideTestCase& testCase)
    {
        const int hour = static_cast<int>(testCase.fhr);

        const double fractionalMinutes = (testCase.fhr - static_cast<double>(hour)) * 60.0;

        const int minute = static_cast<int>(fractionalMinutes);

        const double second = (fractionalMinutes - static_cast<double>(minute)) * 60.0;

        return CivilTime(testCase.year, testCase.month, testCase.day, hour, minute, second, TimeSystem::UTC);
    }

} // anonymous namespace

int main()
{
    TUDEF("SolidTides", "dehantTideInel");

    const std::vector<SolidTideTestCase> testCases = {
        {"IERS DEHANTTIDEINEL 2009-04-13",

         Triple(4075578.385, 931852.890, 4801570.154),

         Triple(137859926952.015, 54228127881.4350, 23509422341.6960),

         Triple(-179996231.920342, -312468450.131567, -169288918.592160),

         2009,
         4,
         13,
         0.0,

         Triple(0.7700420357108125891e-01, 0.6304056321824967613e-01, 0.5516568152597246810e-01)},

        {"IERS DEHANTTIDEINEL 2012-07-13",

         Triple(1112189.660, -4842955.026, 3985352.284),

         Triple(-54537460436.2357, 130244288385.279, 56463429031.5996),

         Triple(300396716.912, 243238281.451, 120548075.939),

         2012,
         7,
         13,
         0.0,

         Triple(-0.2036831479592075833e-01, 0.5658254776225972449e-01, -0.7597679676871742227e-01)},

        {"IERS DEHANTTIDEINEL 2015-07-15",

         Triple(1112200.5696, -4842957.8511, 3985345.9122),

         Triple(100210282451.6279, 103055630398.3160, 56855096480.4475),

         Triple(369817604.4348, 1897917.5258, 120804980.8284),

         2015,
         7,
         15,
         0.0,

         Triple(0.00509570869172363845, 0.0828663025983528700, -0.0636634925404189617)}
    };

    // Tight enough to detect an implementation error, but less sensitive
    // to platform-dependent floating-point differences than 1e-12.
    const double displacementTolerance = 1.0e-11;

    for (const SolidTideTestCase& testCase : testCases)
    {
        const CommonTime epoch(makeEpoch(testCase));

        double T(0.0);
        double fhr(0.0);

        testFramework.changeSourceMethod(std::string("computeTimeArgs: ") + testCase.name);

        SolidTides::computeTimeArgs(epoch, T, fhr);

        TUASSERTFEPS(fhr, testCase.fhr, 1.0e-9);

        testFramework.changeSourceMethod(std::string("dehantTideInel: ") + testCase.name);

        const Triple computed(SolidTides::dehantTideInel(testCase.xsta, testCase.xsun, testCase.xmon, T, fhr));

        printResult(testCase, computed);

        TUASSERTFEPS(computed[0], testCase.expected[0], displacementTolerance);

        TUASSERTFEPS(computed[1], testCase.expected[1], displacementTolerance);

        TUASSERTFEPS(computed[2], testCase.expected[2], displacementTolerance);
    }

    std::cout << "\nTotal Failures for " << __FILE__ << ": " << testFramework.countFails() << std::endl;

    return testFramework.countFails();
}