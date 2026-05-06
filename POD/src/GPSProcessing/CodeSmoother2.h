#pragma once
#include "CodeSmoother.hpp"
#include<memory>


namespace pod
{
    typedef std::unique_ptr<gnsstk::ProcessingClass> process_uptr;
   
	//aggregator for 'scMarker' and 'CodeSmoother' objects
    class CodeSmoother2 : public gnsstk::ProcessingClass
    {
    public:

        CodeSmoother2();

        CodeSmoother2(std::list<gnsstk::CodeSmoother>& smoothers, std::list<process_uptr>& markers, int interval = 600);

        virtual ~CodeSmoother2();

        virtual CodeSmoother2& addScMarker(gnsstk::ProcessingClass& scMarker);

        virtual CodeSmoother2& addScMarker(process_uptr scMarker);

        virtual CodeSmoother2& addSmoother(gnsstk::CodeSmoother& smoother);

        virtual gnsstk::IRinex& Process(gnsstk::IRinex& gData) override;
 
        virtual std::string getClassName(void) const override
        {
            return "CodeSmoother2";
        }

        virtual int getInterval() const
        {
            return window;
        }

        virtual CodeSmoother2& setInterval(int interval);
        

    private:
        //
        std::list<gnsstk::CodeSmoother> smoothers;
        std::list<process_uptr> scMarkers;

        // smoothing window in samples
        int window ;
    };
}

