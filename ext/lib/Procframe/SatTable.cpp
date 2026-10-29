#include "SatTable.h"

#include <algorithm>

namespace gnsstk
{
	SatTable::SatTable(const satTypeValueMap& src)
	{
		reserve(src.size());
		for (const auto& kv : src)
		{
			rows_.emplace_back(kv.first, TypeValueRow(kv.second));
			active_.push_back(1);
		}
		activeCount_ = rows_.size();
	}

	satTypeValueMap SatTable::toSatTypeValueMap() const
	{
		satTypeValueMap res;
		for (const auto& kv : *this)
			res.emplace_hint(res.end(), kv.first, kv.second.toTypeValueMap());
		return res;
	}

	SatTable::size_type SatTable::lowerBound(const SatID& sat) const
	{
		auto it = std::lower_bound(rows_.begin(), rows_.end(), sat,
								   [](const value_type& v, const SatID& s) { return v.first < s; });
		return static_cast<size_type>(it - rows_.begin());
	}

	SatTable::size_type SatTable::nextActive(size_type idx) const
	{
		while (idx < rows_.size() && !active_[idx])
			++idx;
		return idx;
	}

	SatTable::size_type SatTable::prevActive(size_type idx) const
	{
		while (idx > 0)
		{
			--idx;
			if (active_[idx])
				return idx;
		}
		return rows_.size();
	}

	void SatTable::deactivate(size_type idx)
	{
		if (active_[idx])
		{
			active_[idx] = 0;
			--activeCount_;
		}
	}

	template <typename Pred>
	void SatTable::deactivateIf(Pred pred)
	{
		for (size_type i = 0; i < rows_.size(); ++i)
			if (active_[i] && pred(rows_[i]))
				deactivate(i);
	}

	SatTable::iterator SatTable::find(const SatID& sat)
	{
		size_type i = lowerBound(sat);
		if (i < rows_.size() && active_[i] && rows_[i].first == sat)
			return iterator(this, i);
		return end();
	}

	SatTable::const_iterator SatTable::find(const SatID& sat) const
	{
		size_type i = lowerBound(sat);
		if (i < rows_.size() && active_[i] && rows_[i].first == sat)
			return const_iterator(this, i);
		return end();
	}

	TypeValueRow& SatTable::operator[](const SatID& sat)
	{
		size_type i = lowerBound(sat);
		if (i < rows_.size() && rows_[i].first == sat)
		{
			if (!active_[i])
			{
				rows_[i].second.clear();
				active_[i] = 1;
				++activeCount_;
			}
			return rows_[i].second;
		}
		rows_.emplace(rows_.begin() + i, sat, TypeValueRow{});
		active_.insert(active_.begin() + i, 1);
		++activeCount_;
		return rows_[i].second;
	}

	std::pair<SatTable::iterator, bool> SatTable::emplace(const SatID& sat, TypeValueRow row)
	{
		size_type i = lowerBound(sat);
		if (i < rows_.size() && rows_[i].first == sat)
		{
			if (active_[i])
				return {iterator(this, i), false};
			rows_[i].second = std::move(row);
			active_[i] = 1;
			++activeCount_;
			return {iterator(this, i), true};
		}
		rows_.emplace(rows_.begin() + i, sat, std::move(row));
		active_.insert(active_.begin() + i, 1);
		++activeCount_;
		return {iterator(this, i), true};
	}

	SatTable::iterator SatTable::erase(const_iterator pos)
	{
		size_type i = pos.idx_;
		if (i < rows_.size())
			deactivate(i);
		return iterator(this, nextActive(i + 1));
	}

	SatTable::size_type SatTable::erase(const SatID& sat)
	{
		auto it = find(sat);
		if (it == end())
			return 0;
		deactivate(it.idx_);
		return 1;
	}

	void SatTable::clear()
	{
		rows_.clear();
		active_.clear();
		activeCount_ = 0;
	}

	void SatTable::restoreAll()
	{
		std::fill(active_.begin(), active_.end(), static_cast<unsigned char>(1));
		activeCount_ = rows_.size();
	}

	void SatTable::compact()
	{
		if (activeCount_ == rows_.size())
			return;

		size_type out = 0;
		for (size_type i = 0; i < rows_.size(); ++i)
		{
			if (!active_[i])
				continue;
			if (out != i)
				rows_[out] = std::move(rows_[i]);
			++out;
		}
		rows_.resize(out);
		active_.assign(out, 1);
	}

	SatTable::size_type SatTable::numElements() const
	{
		size_type n = 0;
		for (const auto& kv : *this)
			n += kv.second.size();
		return n;
	}

	SatIDSet SatTable::getSatID() const
	{
		SatIDSet res;
		for (const auto& kv : *this)
			res.insert(res.end(), kv.first);
		return res;
	}

	SatSystSet SatTable::getSatSystems() const
	{
		SatSystSet res;
		for (const auto& kv : *this)
			res.insert(kv.first.system);
		return res;
	}

	Vector<SatID> SatTable::getVectorOfSatID() const
	{
		std::vector<SatID> temp;
		temp.reserve(activeCount_);
		for (const auto& kv : *this)
			temp.push_back(kv.first);

		Vector<SatID> result;
		result = temp;
		return result;
	}

	TypeIDSet SatTable::getTypeID() const
	{
		TypeIDSet res;
		for (const auto& kv : *this)
			for (const auto& tv : kv.second)
				res.insert(tv.first);
		return res;
	}

	SatTable SatTable::extractSatID(const SatID& satellite) const
	{
		return extractSatID(SatIDSet{satellite});
	}

	SatTable SatTable::extractSatID(const int& p, SatelliteSystem s) const
	{
		return extractSatID(SatID(p, s));
	}

	SatTable SatTable::extractSatID(const SatIDSet& satSet) const
	{
		SatTable res;
		for (const auto& kv : *this)
			if (satSet.count(kv.first))
				res.emplace(kv.first, kv.second);
		return res;
	}

	SatTable SatTable::extractSatSyst(const SatSystSet& systSet) const
	{
		SatTable res;
		for (const auto& kv : *this)
			if (systSet.count(kv.first.system))
				res.emplace(kv.first, kv.second);
		return res;
	}

	SatTable SatTable::extractSatSyst(SatelliteSystem s) const
	{
		return extractSatSyst(SatSystSet{s});
	}

	SatTable& SatTable::keepOnlySatSyst(const SatSystSet& systSet)
	{
		deactivateIf([&](const value_type& kv) { return !systSet.count(kv.first.system); });
		return *this;
	}

	SatTable& SatTable::keepOnlySatSyst(SatelliteSystem s)
	{
		return keepOnlySatSyst(SatSystSet{s});
	}

	SatTable& SatTable::keepOnlySatID(const SatID& satellite)
	{
		return keepOnlySatID(SatIDSet{satellite});
	}

	SatTable& SatTable::keepOnlySatID(const int& p, SatelliteSystem s)
	{
		return keepOnlySatID(SatID(p, s));
	}

	SatTable& SatTable::keepOnlySatID(const SatIDSet& satSet)
	{
		deactivateIf([&](const value_type& kv) { return !satSet.count(kv.first); });
		return *this;
	}

	satTypeValueMap SatTable::extractTypeID(const TypeID& type) const
	{
		return extractTypeID(TypeIDSet{type});
	}

	satTypeValueMap SatTable::extractTypeID(const TypeIDSet& typeSet) const
	{
		satTypeValueMap res;
		for (const auto& kv : *this)
		{
			auto row = kv.second.extractTypeID(typeSet);
			if (!row.empty())
				res[kv.first] = row.toTypeValueMap();
		}
		return res;
	}

	SatTable& SatTable::keepOnlyTypeID(const TypeID& type)
	{
		return keepOnlyTypeID(TypeIDSet{type});
	}

	SatTable& SatTable::keepOnlyTypeID(const TypeIDSet& typeSet)
	{
		for (auto& kv : *this)
			kv.second.keepOnlyTypeID(typeSet);
		return *this;
	}

	SatTable& SatTable::removeSatID(int id, SatelliteSystem system)
	{
		return removeSatID(SatID(id, system));
	}

	SatTable& SatTable::removeSatID(const SatID& satellite)
	{
		erase(satellite);
		return *this;
	}

	SatTable& SatTable::removeSatID(const SatIDSet& satSet)
	{
		for (const auto& sv : satSet)
			erase(sv);
		return *this;
	}

	SatTable& SatTable::removeSatSyst(const SatSystSet& systSet)
	{
		deactivateIf([&](const value_type& kv) { return systSet.count(kv.first.system) > 0; });
		return *this;
	}

	SatTable& SatTable::removeSatSyst(SatelliteSystem syst)
	{
		return removeSatSyst(SatSystSet{syst});
	}

	SatTable& SatTable::removeTypeID(const TypeID& type)
	{
		return removeTypeID(TypeIDSet{type});
	}

	SatTable& SatTable::removeTypeID(const TypeIDSet& typeSet)
	{
		for (auto& kv : *this)
			kv.second.removeTypeID(typeSet);
		return *this;
	}

	Vector<double> SatTable::getVectorOfTypeID(const TypeID& type) const
	{
		Vector<double> result(activeCount_, 0.0);
		size_type i = 0;
		for (const auto& kv : *this)
		{
			auto it = kv.second.find(type);
			if (it != kv.second.end())
				result[i] = it->second;
			++i;
		}
		return result;
	}

	Matrix<double> SatTable::getMatrixOfTypes(const TypeIDSet& typeSet) const
	{
		Matrix<double> result(activeCount_, typeSet.size(), 0.0);
		size_type row = 0;
		for (const auto& kv : *this)
		{
			size_type col = 0;
			for (const auto& type : typeSet)
			{
				auto it = kv.second.find(type);
				if (it != kv.second.end())
					result(row, col) = it->second;
				++col;
			}
			++row;
		}
		return result;
	}

	SatTable& SatTable::insertTypeIDVector(const TypeID& type, const Vector<double>& dataVector)
	{
		if (dataVector.size() != activeCount_)
		{
			GNSSTK_THROW(NumberOfSatsMismatch(
				" Number of data values in vector and number of satellites do not match"));
		}

		size_type pos = 0;
		for (auto& kv : *this)
			kv.second[type] = dataVector[pos++];

		return *this;
	}

	SatTable& SatTable::insertMatrix(const TypeIDSet& typeSet, const Matrix<double>& dataMatrix)
	{
		if (dataMatrix.rows() != activeCount_)
		{
			GNSSTK_THROW(NumberOfSatsMismatch(
				"Number of rows in matrix and number of satellites do not match"));
		}
		if (dataMatrix.cols() != typeSet.size())
		{
			GNSSTK_THROW(NumberOfTypesMismatch(
				"Number of data values per row in matrix and number of types do not match"));
		}

		size_type row = 0;
		for (auto& kv : *this)
		{
			size_type col = 0;
			for (const auto& type : typeSet)
				kv.second[type] = dataMatrix(row, col++);
			++row;
		}
		return *this;
	}

	double SatTable::getValue(const SatID& satellite, const TypeID& type) const
	{
		auto it = find(satellite);
		if (it == end())
			GNSSTK_THROW(SatIDNotFound("SatID not found in map"));
		return it->second.getValue(type);
	}

	TypeValueRow& SatTable::operator()(const SatID& satellite)
	{
		auto it = find(satellite);
		if (it == end())
			GNSSTK_THROW(SatIDNotFound("SatID not found in map"));
		return it->second;
	}

	std::ostream& SatTable::dump(std::ostream& s, int mode) const
	{
		for (const auto& kv : *this)
		{
			s << kv.first << " ";
			for (const auto& tv : kv.second)
			{
				if (mode == 1)
					s << tv.first << " ";
				s << tv.second << " ";
			}
			s << std::endl;
		}
		return s;
	}

	std::ostream& operator<<(std::ostream& s, const SatTable& obj)
	{
		return obj.dump(s, 1);
	}

} // namespace gnsstk
