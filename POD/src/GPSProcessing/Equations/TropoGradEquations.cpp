#include "TropoGradEquations.h"
#include "StateLayout.h"

using namespace gnsstk;
namespace pod
{
	TropoGradEquations::TropoGradEquations(double qPrimeZ, double qPrimeN, double qPrimeE)
		: types({FilterParameter(gnsstk::TypeID::wetMap),
				 FilterParameter(gnsstk::TypeID::wetMapNorth),
				 FilterParameter(gnsstk::TypeID::wetMapEast)})
		, pStochModelZ(std::make_unique<gnsstk::RandomWalkModel>(qPrimeZ))
		, pStochModelNorth(std::make_unique<gnsstk::RandomWalkModel>(qPrimeN))
		, pStochModelEast(std::make_unique<gnsstk::RandomWalkModel>(qPrimeE))
	{
	}

	void TropoGradEquations::prepare(gnsstk::IRinex& gData)
	{
		pStochModelZ->Prepare(SatID::dummy, gData);
		pStochModelNorth->Prepare(SatID::dummy, gData);
		pStochModelEast->Prepare(SatID::dummy, gData);
	}

	void TropoGradEquations::contributeDesignMatrix(const gnsstk::IRinex& gData,
									 const gnsstk::TypeIDSet& obsTypes,
									 gnsstk::Matrix<double>& H,
									 const StateLayout& layout)
	{
		for (auto&& type : types)
		{
			int col = layout.index(type);
			int row(0);
			for (const auto& t : obsTypes)
				for (const auto& it : gData.getBody())
					H(row++, col) = it.second->get_value().at(type.type);
		}
	}

	void TropoGradEquations::contributeTransitionMartix(gnsstk::Matrix<double>& Phi, const StateLayout& layout) const
	{
		FilterParameter pZ(gnsstk::TypeID::wetMap);
		FilterParameter pN(gnsstk::TypeID::wetMapNorth);
		FilterParameter pE(gnsstk::TypeID::wetMapEast);

		Phi(layout.index(pZ), layout.index(pZ)) = pStochModelZ->getPhi();
		Phi(layout.index(pN), layout.index(pN)) = pStochModelNorth->getPhi();
		Phi(layout.index(pE), layout.index(pE)) = pStochModelEast->getPhi();
	}

	void TropoGradEquations::contributeProcessNoiseMatrix(gnsstk::Matrix<double>& Q, const StateLayout& layout) const
	{
		FilterParameter pZ(gnsstk::TypeID::wetMap);
		FilterParameter pN(gnsstk::TypeID::wetMapNorth);
		FilterParameter pE(gnsstk::TypeID::wetMapEast);

		Q(layout.index(pZ), layout.index(pZ)) = pStochModelZ->getQ();
		Q(layout.index(pN), layout.index(pN)) = pStochModelNorth->getQ();
		Q(layout.index(pE), layout.index(pE)) = pStochModelEast->getQ();
	}

	void TropoGradEquations::defStateAndCovariance(gnsstk::Vector<double>& x,
												   gnsstk::Matrix<double>& P,
												   const StateLayout& layout) const
	{
		FilterParameter pZ(gnsstk::TypeID::wetMap);
		FilterParameter pN(gnsstk::TypeID::wetMapNorth);
		FilterParameter pE(gnsstk::TypeID::wetMapEast);

		int colZ = layout.index(pZ);
		x(colZ) = 5.000e-02;
		P(colZ, colZ) = 5e-5;

		int colN = layout.index(pN);
		x(colN) = 0.000e-03;
		P(colN, colN) = 1.667e-06;

		int colE = layout.index(pE);
		x(colE) = 0.000e-03;
		P(colE, colE) = 1.667e-06;
	}

	int TropoGradEquations::getNumUnknowns() const
	{
		return types.size();
	}
} // namespace pod
