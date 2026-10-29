//============================================================================
//
//  Unit tests for TypeValueRow and SatTable.
//
//  The reference behaviour is std::map<SatID, std::map<TypeID, double>>:
//  iteration order and content must match it exactly.
//
//============================================================================

#include "SatTable.h"
#include "TestUtil.hpp"

#include <algorithm>
#include <iostream>
#include <vector>

using namespace gnsstk;

namespace
{
	const SatID G01(1, SatelliteSystem::GPS);
	const SatID G05(5, SatelliteSystem::GPS);
	const SatID G12(12, SatelliteSystem::GPS);
	const SatID R03(3, SatelliteSystem::Glonass);
	const SatID E07(7, SatelliteSystem::Galileo);

	satTypeValueMap makeReference()
	{
		satTypeValueMap ref;
		// inserted out of order on purpose
		ref[G12][TypeID::L1] = 12.1;
		ref[G12][TypeID::C1] = 12.0;
		ref[R03][TypeID::C1] = 3.0;
		ref[G01][TypeID::C1] = 1.0;
		ref[G01][TypeID::L1] = 1.1;
		ref[G01][TypeID::prefitC] = 1.2;
		ref[E07][TypeID::C1] = 7.0;
		ref[G05][TypeID::L1] = 5.1;
		return ref;
	}

	template <typename A, typename B>
	bool sameContent(const A& a, const B& b)
	{
		if (a.size() != b.size())
			return false;
		auto ia = a.begin();
		auto ib = b.begin();
		for (; ia != a.end(); ++ia, ++ib)
		{
			if (!(ia->first == ib->first) || ia->second.size() != ib->second.size())
				return false;
			auto ta = ia->second.begin();
			auto tb = ib->second.begin();
			for (; ta != ia->second.end(); ++ta, ++tb)
				if (!(ta->first == tb->first) || ta->second != tb->second)
					return false;
		}
		return true;
	}

	int testTypeValueRow()
	{
		TUDEF("TypeValueRow", "operator[]");

		TypeValueRow row;
		row[TypeID::L1] = 2.0;
		row[TypeID::C1] = 1.0;
		row[TypeID::prefitC] = 3.0;
		row[TypeID::C1] = 10.0;

		typeValueMap ref;
		ref[TypeID::L1] = 2.0;
		ref[TypeID::C1] = 10.0;
		ref[TypeID::prefitC] = 3.0;

		TUASSERTE(size_t, ref.size(), row.size());
		auto itRef = ref.begin();
		bool sameOrder = true;
		for (const auto& kv : row)
		{
			sameOrder = sameOrder && kv.first == itRef->first && kv.second == itRef->second;
			++itRef;
		}
		TUASSERT(sameOrder);

		TUCSM("find/count/erase");
		TUASSERT(row.find(TypeID::CSL1) == row.end());
		TUASSERTE(size_t, 1, row.count(TypeID::L1));
		TUASSERTE(size_t, 1, row.erase(TypeID::L1));
		TUASSERTE(size_t, 0, row.erase(TypeID::L1));
		TUASSERTE(size_t, 0, row.count(TypeID::L1));

		TUCSM("operator-> compatibility");
		TypeValueRow& r = row;
		auto it = r->find(TypeID::C1);
		TUASSERT(it != r->end());
		TUASSERTFE(10.0, it->second);

		TUCSM("getValue");
		TUASSERTFE(3.0, row.getValue(TypeID::prefitC));
		try
		{
			row.getValue(TypeID::weight);
			TUFAIL("TypeIDNotFound expected");
		}
		catch (const TypeIDNotFound&)
		{
			TUPASS("TypeIDNotFound thrown");
		}

		TUCSM("keepOnlyTypeID/removeTypeID");
		TypeValueRow copy(row);
		copy.keepOnlyTypeID(TypeID::C1);
		TUASSERTE(size_t, 1, copy.size());
		copy = row;
		copy.removeTypeID(TypeIDSet{TypeID::C1, TypeID::prefitC});
		TUASSERT(copy.empty());

		TUCSM("typeValueMap conversion");
		typeValueMap back = row.toTypeValueMap();
		TUASSERT(TypeValueRow(back) == row);

		TURETURN();
	}

	int testOrderAndConversion()
	{
		TUDEF("SatTable", "SatTable(satTypeValueMap)");

		const satTypeValueMap ref = makeReference();
		SatTable table(ref);

		TUASSERTE(size_t, ref.size(), table.size());
		TUASSERT(sameContent(ref, table));

		TUCSM("operator[] insertion order");
		SatTable built;
		built[G12][TypeID::L1] = 12.1;
		built[G12][TypeID::C1] = 12.0;
		built[R03][TypeID::C1] = 3.0;
		built[G01][TypeID::C1] = 1.0;
		built[G01][TypeID::L1] = 1.1;
		built[G01][TypeID::prefitC] = 1.2;
		built[E07][TypeID::C1] = 7.0;
		built[G05][TypeID::L1] = 5.1;
		TUASSERT(sameContent(ref, built));

		TUCSM("toSatTypeValueMap");
		TUASSERT(sameContent(ref, table.toSatTypeValueMap()));

		TUCSM("structured bindings and legacy syntax");
		size_t n = 0;
		for (auto& [sat, tv] : table)
		{
			auto it = tv->find(TypeID::C1);
			if (it != tv->end())
				++n;
		}
		TUASSERTE(size_t, 4, n);

		TURETURN();
	}

	int testSoftRemoval()
	{
		TUDEF("SatTable", "removeSatID");

		SatTable table(makeReference());
		TypeValueRow* g12 = &table[G12];

		table.removeSatID(SatIDSet{G05, R03});
		TUASSERTE(size_t, 3, table.size());
		TUASSERTE(size_t, 5, table.storedSize());
		TUASSERT(table.find(G05) == table.end());
		TUASSERTE(size_t, 0, table.count(R03));
		TUASSERT(g12 == &table[G12]);

		satTypeValueMap expected = makeReference();
		expected.erase(G05);
		expected.erase(R03);
		TUASSERT(sameContent(expected, table));

		TUCSM("erase(iterator) during iteration");
		for (auto it = table.begin(); it != table.end();)
		{
			if (it->first.system == SatelliteSystem::Galileo)
				it = table.erase(it);
			else
				++it;
		}
		expected.erase(E07);
		TUASSERT(sameContent(expected, table));

		TUCSM("values written while hidden survive restoreAll");
		table[G01][TypeID::postfitC] = 0.5;
		table.restoreAll();
		satTypeValueMap restored = makeReference();
		restored[G01][TypeID::postfitC] = 0.5;
		TUASSERT(sameContent(restored, table));

		TUCSM("operator[] on hidden satellite clears its row");
		table.removeSatID(G05);
		table[G05][TypeID::C1] = 55.0;
		TUASSERTE(size_t, 1, table[G05].size());

		TUCSM("compact");
		table.keepOnlySatSyst(SatelliteSystem::GPS);
		TUASSERTE(size_t, 3, table.size());
		table.compact();
		TUASSERTE(size_t, 3, table.storedSize());
		table.restoreAll();
		TUASSERTE(size_t, 3, table.size());

		TUCSM("copy keeps the mask");
		SatTable src(makeReference());
		src.removeSatID(G01);
		SatTable copy(src);
		TUASSERTE(size_t, 4, copy.size());
		copy.restoreAll();
		TUASSERTE(size_t, 5, copy.size());

		TUCSM("reverse iteration skips hidden rows");
		src.removeSatID(E07);
		std::vector<SatID> rev;
		for (auto it = src.end(); it != src.begin();)
		{
			--it;
			rev.push_back(it->first);
		}
		std::vector<SatID> fwd;
		for (const auto& kv : src)
			fwd.push_back(kv.first);
		std::reverse(rev.begin(), rev.end());
		TUASSERT(rev == fwd);

		TURETURN();
	}

	int testBulkOperations()
	{
		TUDEF("SatTable", "getVectorOfTypeID");

		SatTable table(makeReference());
		table.removeSatID(E07);

		Vector<double> c1 = table.getVectorOfTypeID(TypeID::C1);
		TUASSERTE(size_t, 4, c1.size());
		// order: G01, G05, G12, R03 (SatID order)
		TUASSERTFE(1.0, c1[0]);
		TUASSERTFE(0.0, c1[1]);
		TUASSERTFE(12.0, c1[2]);
		TUASSERTFE(3.0, c1[3]);

		TUCSM("getMatrixOfTypes");
		Matrix<double> m = table.getMatrixOfTypes(TypeIDSet{TypeID::C1, TypeID::L1});
		TUASSERTE(size_t, 4, m.rows());
		TUASSERTE(size_t, 2, m.cols());
		TUASSERTFE(1.1, m(0, 1));
		TUASSERTFE(5.1, m(1, 1));
		TUASSERTFE(0.0, m(3, 1));

		TUCSM("insertTypeIDVector");
		Vector<double> w(4);
		for (size_t i = 0; i < 4; ++i)
			w[i] = double(i + 1);
		table.insertTypeIDVector(TypeID::weight, w);
		TUASSERTFE(4.0, table.getValue(R03, TypeID::weight));
		table.restoreAll();
		TUASSERTE(size_t, 0, table[E07].count(TypeID::weight));

		TUCSM("insertTypeIDVector size mismatch");
		try
		{
			table.insertTypeIDVector(TypeID::weight, w);
			TUFAIL("NumberOfSatsMismatch expected");
		}
		catch (const NumberOfSatsMismatch&)
		{
			TUPASS("NumberOfSatsMismatch thrown");
		}

		TUCSM("getSatID/getTypeID/numElements");
		table.removeSatID(G01);
		TUASSERT(table.getSatID() == (SatIDSet{G05, G12, R03, E07}));
		TUASSERT(table.getTypeID() == (TypeIDSet{TypeID::C1, TypeID::L1, TypeID::weight}));
		// G05: L1, weight; G12: C1, L1, weight; R03: C1, weight; E07: C1
		TUASSERTE(size_t, 8, table.numElements());

		TUCSM("extractSatSyst");
		SatTable gps = table.extractSatSyst(SatelliteSystem::GPS);
		TUASSERTE(size_t, 2, gps.size());
		TUASSERTE(size_t, 2, gps.storedSize());

		TUCSM("getValue on hidden satellite");
		try
		{
			table.getValue(G01, TypeID::C1);
			TUFAIL("SatIDNotFound expected");
		}
		catch (const SatIDNotFound&)
		{
			TUPASS("SatIDNotFound thrown");
		}

		TURETURN();
	}
} // namespace

int main()
{
	int errorTotal = 0;

	errorTotal += testTypeValueRow();
	errorTotal += testOrderAndConversion();
	errorTotal += testSoftRemoval();
	errorTotal += testBulkOperations();

	std::cout << "Total Failures for " << __FILE__ << ": " << errorTotal << std::endl;

	return errorTotal;
}
