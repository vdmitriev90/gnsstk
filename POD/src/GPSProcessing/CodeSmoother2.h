#pragma once
#include "CodeSmoother.hpp"

#include <memory>

namespace pod
{
    using process_uptr = std::unique_ptr<gnsstk::ProcessingClass>;
    using code_smoother_ptr = std::unique_ptr<gnsstk::CodeSmoother>;

    // aggregator for 'scMarker' and 'CodeSmoother' objects
    class CodeSmoother2 : public gnsstk::ProcessingClass
    {
      public:
        CodeSmoother2() = default;

        CodeSmoother2(std::vector<code_smoother_ptr>&& codeSmoothers,
                      std::vector<process_uptr>&& csMarkers,
                      int interval = 600);

        virtual ~CodeSmoother2();

        CodeSmoother2& addScMarker(process_uptr scMarker);

        CodeSmoother2& addSmoother(code_smoother_ptr smoother);

        virtual gnsstk::IRinex& Process(gnsstk::IRinex& gData) override;

        virtual std::string getClassName() const override;

        int getInterval() const;

        CodeSmoother2& setInterval(int interval);

      private:
        // Code smoothers
        std::vector<code_smoother_ptr> smoothers_;
        // Cycle slip markers
        std::vector<process_uptr> scMarkers_;

        // smoothing window in samples
        int interval_;
    };
} // namespace pod
