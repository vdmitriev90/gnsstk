#include "CodeSmoother2.h"

using namespace gnsstk;

namespace pod
{
    CodeSmoother2::CodeSmoother2(std::vector<code_smoother_ptr>&& codeSmoothers,
                                 std::vector<process_uptr>&& csMarkers,
                                 int interval)
        : smoothers_(std::move(codeSmoothers))
        , scMarkers_(std::move(csMarkers))
        , interval_(interval)

    {
    }

    CodeSmoother2::~CodeSmoother2() {}

    CodeSmoother2& CodeSmoother2::addScMarker(process_uptr scMarker)
    {
        scMarkers_.push_back(std::move(scMarker));
        return *this;
    }

    CodeSmoother2& CodeSmoother2::addSmoother(code_smoother_ptr smoother)
    {
        smoothers_.push_back(std::move(smoother));
        return *this;
    }

    gnsstk::IRinex& CodeSmoother2::Process(gnsstk::IRinex& gData)
    {
        // first, mark cycle slips
        for (auto& it : scMarkers_)
            gData >> *it;

        // then, smooth pseudoranges
        for (auto& it : smoothers_)
            gData >> *it;

        return gData;
    }

    std::string CodeSmoother2::getClassName() const
    {
        return "CodeSmoother2";
    }

    int CodeSmoother2::getInterval() const
    {
        return interval_;
    }

    CodeSmoother2& CodeSmoother2::setInterval(int interval)
    {
        interval_ = interval;
        for (auto& it : smoothers_)
            it->setMaxWindowSize(interval_);
        return (*this);
    }
} // namespace pod
