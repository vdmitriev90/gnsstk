#include "TypeValueRow.h"

namespace gnsstk
{
	namespace
	{
		struct TypeLess
		{
			bool operator()(const TypeValueRow::value_type& v, const TypeID& t) const
			{
				return v.first < t;
			}
		};
	} // namespace

	TypeValueRow::TypeValueRow(const typeValueMap& src)
	{
		data_.reserve(src.size());
		for (const auto& kv : src)
			data_.emplace_back(kv.first, kv.second);
	}

	typeValueMap TypeValueRow::toTypeValueMap() const
	{
		typeValueMap res;
		for (const auto& kv : data_)
			res.emplace_hint(res.end(), kv.first, kv.second);
		return res;
	}

	TypeValueRow::iterator TypeValueRow::lowerBound(const TypeID& type)
	{
		return std::lower_bound(data_.begin(), data_.end(), type, TypeLess{});
	}

	TypeValueRow::const_iterator TypeValueRow::lowerBound(const TypeID& type) const
	{
		return std::lower_bound(data_.begin(), data_.end(), type, TypeLess{});
	}

	TypeValueRow::iterator TypeValueRow::find(const TypeID& type)
	{
		auto it = lowerBound(type);
		return (it != data_.end() && it->first == type) ? it : data_.end();
	}

	TypeValueRow::const_iterator TypeValueRow::find(const TypeID& type) const
	{
		auto it = lowerBound(type);
		return (it != data_.end() && it->first == type) ? it : data_.end();
	}

	double& TypeValueRow::operator[](const TypeID& type)
	{
		auto it = lowerBound(type);
		if (it == data_.end() || !(it->first == type))
			it = data_.emplace(it, type, 0.0);
		return it->second;
	}

	std::pair<TypeValueRow::iterator, bool> TypeValueRow::insert(const value_type& value)
	{
		auto it = lowerBound(value.first);
		if (it != data_.end() && it->first == value.first)
			return {it, false};
		return {data_.insert(it, value), true};
	}

	TypeValueRow::size_type TypeValueRow::erase(const TypeID& type)
	{
		auto it = find(type);
		if (it == data_.end())
			return 0;
		data_.erase(it);
		return 1;
	}

	TypeIDSet TypeValueRow::getTypeID() const
	{
		TypeIDSet res;
		for (const auto& kv : data_)
			res.insert(res.end(), kv.first);
		return res;
	}

	TypeValueRow TypeValueRow::extractTypeID(const TypeID& type) const
	{
		TypeValueRow res;
		auto it = find(type);
		if (it != data_.end())
			res.data_.push_back(*it);
		return res;
	}

	TypeValueRow TypeValueRow::extractTypeID(const TypeIDSet& typeSet) const
	{
		TypeValueRow res;
		for (const auto& kv : data_)
			if (typeSet.count(kv.first))
				res.data_.push_back(kv);
		return res;
	}

	TypeValueRow& TypeValueRow::keepOnlyTypeID(const TypeID& type)
	{
		return keepOnlyTypeID(TypeIDSet{type});
	}

	TypeValueRow& TypeValueRow::keepOnlyTypeID(const TypeIDSet& typeSet)
	{
		data_.erase(std::remove_if(data_.begin(), data_.end(),
								   [&](const value_type& kv) { return !typeSet.count(kv.first); }),
					data_.end());
		return *this;
	}

	TypeValueRow& TypeValueRow::removeTypeID(const TypeIDSet& typeSet)
	{
		data_.erase(std::remove_if(data_.begin(), data_.end(),
								   [&](const value_type& kv) { return typeSet.count(kv.first) > 0; }),
					data_.end());
		return *this;
	}

	double TypeValueRow::getValue(const TypeID& type) const
	{
		auto it = find(type);
		if (it == data_.end())
			GNSSTK_THROW(TypeIDNotFound("TypeID not found in row"));
		return it->second;
	}

	double& TypeValueRow::operator()(const TypeID& type)
	{
		auto it = find(type);
		if (it == data_.end())
			GNSSTK_THROW(TypeIDNotFound("TypeID not found in row"));
		return it->second;
	}

	std::ostream& operator<<(std::ostream& s, const TypeValueRow& obj)
	{
		for (const auto& kv : obj)
			s << kv.first << " " << kv.second << " ";
		return s;
	}

} // namespace gnsstk
