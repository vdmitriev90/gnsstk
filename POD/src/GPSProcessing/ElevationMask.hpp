#pragma once
#include"ProcessingClass.hpp"

namespace pod
{
	class ElevationMask : public gnsstk::ProcessingClass
	{

	public:

		ElevationMask() :elevation(-90)
		{}

		ElevationMask(double el) :elevation(el)
		{}

		// Inherited via ProcessingClass
		virtual gnsstk::IRinex & Process(gnsstk::IRinex & gData);


		double getElevation() const
		{ return elevation; }

		ElevationMask& setElevation(double value)
		{
			elevation = value;
			return *this;
		}

		virtual std::string getClassName(void) const override
		{ return "pod::ElevationMasks"; }

	private:
		double elevation;

	};
}
