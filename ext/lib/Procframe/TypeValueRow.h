#pragma once
#include "typeValueMap.hpp"

#include <algorithm>
#include <stdexcept>
#include <utility>
#include <vector>

namespace gnsstk
{
	/// Flat replacement for typeValueMap: values of one satellite stored in a
	/// vector sorted by TypeID. Iteration order is the same as for
	/// std::map<TypeID, double>.
	class TypeValueRow
	{
	  public:
		using value_type = std::pair<TypeID, double>;
		using container_type = std::vector<value_type>;
		using iterator = container_type::iterator;
		using const_iterator = container_type::const_iterator;
		using size_type = container_type::size_type;

		TypeValueRow() = default;

		explicit TypeValueRow(const typeValueMap& src);

		/// Converts back to the legacy map type.
		typeValueMap toTypeValueMap() const;

		/// Allows the legacy pointer-like syntax: it->second->find(type)
		TypeValueRow* operator->()
		{
			return this;
		}
		const TypeValueRow* operator->() const
		{
			return this;
		}

		/// Allows the legacy pointer-like syntax: (*it->second)[type]
		TypeValueRow& operator*()
		{
			return *this;
		}
		const TypeValueRow& operator*() const
		{
			return *this;
		}

		iterator begin()
		{
			return data_.begin();
		}
		const_iterator begin() const
		{
			return data_.begin();
		}
		iterator end()
		{
			return data_.end();
		}
		const_iterator end() const
		{
			return data_.end();
		}

		size_type size() const
		{
			return data_.size();
		}
		bool empty() const
		{
			return data_.empty();
		}
		void clear()
		{
			data_.clear();
		}
		void reserve(size_type n)
		{
			data_.reserve(n);
		}

		iterator find(const TypeID& type);
		const_iterator find(const TypeID& type) const;

		size_type count(const TypeID& type) const
		{
			return find(type) != end() ? 1 : 0;
		}

		/// Returns the value for the type, inserting 0.0 if absent.
		double& operator[](const TypeID& type);

		std::pair<iterator, bool> insert(const value_type& value);

		size_type erase(const TypeID& type);
		iterator erase(const_iterator pos)
		{
			return data_.erase(pos);
		}

		// ---- typeValueMap compatible API ----

		size_type numTypes() const
		{
			return data_.size();
		}

		TypeIDSet getTypeID() const;

		TypeValueRow extractTypeID(const TypeID& type) const;
		TypeValueRow extractTypeID(const TypeIDSet& typeSet) const;

		TypeValueRow& keepOnlyTypeID(const TypeID& type);
		TypeValueRow& keepOnlyTypeID(const TypeIDSet& typeSet);

		TypeValueRow& removeTypeID(const TypeID& type)
		{
			erase(type);
			return *this;
		}
		TypeValueRow& removeTypeID(const TypeIDSet& typeSet);

		/// Throws TypeIDNotFound if the type is absent.
		double getValue(const TypeID& type) const;

		/// std::map-compatible access: throws std::out_of_range if absent.
		double& at(const TypeID& type)
		{
			auto it = find(type);
			if (it == end())
				throw std::out_of_range("TypeValueRow::at");
			return it->second;
		}
		double at(const TypeID& type) const
		{
			auto it = find(type);
			if (it == end())
				throw std::out_of_range("TypeValueRow::at");
			return it->second;
		}

		/// Throws TypeIDNotFound if the type is absent.
		double& operator()(const TypeID& type);

		friend bool operator==(const TypeValueRow& a, const TypeValueRow& b)
		{
			return a.data_ == b.data_;
		}

	  private:
		iterator lowerBound(const TypeID& type);
		const_iterator lowerBound(const TypeID& type) const;

		container_type data_;
	};

	std::ostream& operator<<(std::ostream& s, const TypeValueRow& obj);

} // namespace gnsstk
