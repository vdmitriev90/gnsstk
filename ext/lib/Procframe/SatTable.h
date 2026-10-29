#pragma once
#include "Matrix.hpp"
#include "TypeValueRow.h"
#include "satTypeValueMap.hpp"

#include <iterator>
#include <type_traits>
#include <utility>
#include <vector>

namespace gnsstk
{
	/// Owning per-epoch observation storage: rows sorted by SatID, each row is
	/// a TypeValueRow. Iteration order is the same as for
	/// std::map<SatID, typeValueMap>.
	///
	/// Satellites can be removed softly: the row stays in storage but is
	/// hidden from the whole public API (iteration, find, size, bulk
	/// methods). restoreAll() makes all stored rows visible again, compact()
	/// physically drops hidden rows. Soft removal never invalidates
	/// references to rows.
	class SatTable
	{
	  public:
		using value_type = std::pair<SatID, TypeValueRow>;
		using size_type = std::size_t;

		template <bool IsConst>
		class Iter
		{
			using Table = std::conditional_t<IsConst, const SatTable, SatTable>;

		  public:
			using iterator_category = std::bidirectional_iterator_tag;
			using value_type = SatTable::value_type;
			using difference_type = std::ptrdiff_t;
			using reference = std::conditional_t<IsConst, const value_type&, value_type&>;
			using pointer = std::conditional_t<IsConst, const value_type*, value_type*>;

			Iter() = default;
			Iter(Table* table, size_type idx) : table_(table), idx_(idx)
			{
			}

			// iterator -> const_iterator
			template <bool C = IsConst, typename = std::enable_if_t<C>>
			Iter(const Iter<false>& other) : table_(other.table_), idx_(other.idx_)
			{
			}

			reference operator*() const
			{
				return table_->rows_[idx_];
			}
			pointer operator->() const
			{
				return &table_->rows_[idx_];
			}

			Iter& operator++()
			{
				idx_ = table_->nextActive(idx_ + 1);
				return *this;
			}
			Iter operator++(int)
			{
				Iter tmp(*this);
				++(*this);
				return tmp;
			}
			Iter& operator--()
			{
				idx_ = table_->prevActive(idx_);
				return *this;
			}
			Iter operator--(int)
			{
				Iter tmp(*this);
				--(*this);
				return tmp;
			}

			friend bool operator==(const Iter& a, const Iter& b)
			{
				return a.idx_ == b.idx_;
			}
			friend bool operator!=(const Iter& a, const Iter& b)
			{
				return a.idx_ != b.idx_;
			}

			size_type index() const
			{
				return idx_;
			}

		  private:
			template <bool>
			friend class Iter;
			friend class SatTable;

			Table* table_ = nullptr;
			size_type idx_ = 0;
		};

		using iterator = Iter<false>;
		using const_iterator = Iter<true>;

		SatTable() = default;

		explicit SatTable(const satTypeValueMap& src);

		/// Converts active rows to the legacy map type.
		satTypeValueMap toSatTypeValueMap() const;

		// ---- container API (sees active rows only) ----

		iterator begin()
		{
			return iterator(this, nextActive(0));
		}
		const_iterator begin() const
		{
			return const_iterator(this, nextActive(0));
		}
		iterator end()
		{
			return iterator(this, rows_.size());
		}
		const_iterator end() const
		{
			return const_iterator(this, rows_.size());
		}

		size_type size() const
		{
			return activeCount_;
		}
		bool empty() const
		{
			return activeCount_ == 0;
		}
		size_type numSats() const
		{
			return activeCount_;
		}

		iterator find(const SatID& sat);
		const_iterator find(const SatID& sat) const;

		size_type count(const SatID& sat) const
		{
			return find(sat) != end() ? 1 : 0;
		}

		/// Returns the row of the satellite. A missing satellite is inserted
		/// with an empty row; a softly removed one is reactivated with its
		/// values cleared.
		TypeValueRow& operator[](const SatID& sat);

		/// Inserts the row if the satellite is not active. A softly removed
		/// satellite is reactivated and its row replaced.
		std::pair<iterator, bool> emplace(const SatID& sat, TypeValueRow row = {});
		std::pair<iterator, bool> insert(const value_type& value)
		{
			return emplace(value.first, value.second);
		}

		/// Soft removal; returns the next active row.
		iterator erase(const_iterator pos);
		size_type erase(const SatID& sat);

		/// Drops all rows, including hidden ones.
		void clear();

		void reserve(size_type n)
		{
			rows_.reserve(n);
			active_.reserve(n);
		}

		// ---- soft removal management ----

		/// Makes all stored rows visible again.
		void restoreAll();

		/// Physically removes hidden rows. Invalidates iterators and references.
		void compact();

		/// Number of stored rows, including hidden ones.
		size_type storedSize() const
		{
			return rows_.size();
		}

		// ---- SatTypePtrMap compatible API ----

		size_type numElements() const;
		SatIDSet getSatID() const;
		SatSystSet getSatSystems() const;
		Vector<SatID> getVectorOfSatID() const;
		TypeIDSet getTypeID() const;

		/// Returned tables contain copies of active rows only.
		SatTable extractSatID(const SatID& satellite) const;
		SatTable extractSatID(const int& p, SatelliteSystem s) const;
		SatTable extractSatID(const SatIDSet& satSet) const;
		SatTable extractSatSyst(const SatSystSet& systSet) const;
		SatTable extractSatSyst(SatelliteSystem s) const;

		SatTable& keepOnlySatSyst(const SatSystSet& systSet);
		SatTable& keepOnlySatSyst(SatelliteSystem s);
		SatTable& keepOnlySatID(const SatID& satellite);
		SatTable& keepOnlySatID(const int& p, SatelliteSystem s);
		SatTable& keepOnlySatID(const SatIDSet& satSet);

		satTypeValueMap extractTypeID(const TypeID& type) const;
		satTypeValueMap extractTypeID(const TypeIDSet& typeSet) const;

		SatTable& keepOnlyTypeID(const TypeID& type);
		SatTable& keepOnlyTypeID(const TypeIDSet& typeSet);

		SatTable& removeSatID(int id, SatelliteSystem system);
		SatTable& removeSatID(const SatID& satellite);
		SatTable& removeSatID(const SatIDSet& satSet);

		SatTable& removeSatSyst(const SatSystSet& systSet);
		SatTable& removeSatSyst(SatelliteSystem syst);

		SatTable& removeTypeID(const TypeID& type);
		SatTable& removeTypeID(const TypeIDSet& typeSet);

		/// Missing values are returned as zero.
		Vector<double> getVectorOfTypeID(const TypeID& type) const;

		/// Missing values are returned as zero.
		Matrix<double> getMatrixOfTypes(const TypeIDSet& typeSet) const;

		SatTable& insertTypeIDVector(const TypeID& type, const Vector<double>& dataVector);
		SatTable& insertMatrix(const TypeIDSet& typeSet, const Matrix<double>& dataMatrix);

		/// Throws SatIDNotFound / TypeIDNotFound.
		double getValue(const SatID& satellite, const TypeID& type) const;

		/// Throws SatIDNotFound.
		TypeValueRow& operator()(const SatID& satellite);

		std::ostream& dump(std::ostream& s, int mode = 0) const;

	  private:
		size_type lowerBound(const SatID& sat) const;
		size_type nextActive(size_type idx) const;
		size_type prevActive(size_type idx) const;
		void deactivate(size_type idx);
		template <typename Pred>
		void deactivateIf(Pred pred);

		std::vector<value_type> rows_;
		std::vector<unsigned char> active_;
		size_type activeCount_ = 0;
	};

	std::ostream& operator<<(std::ostream& s, const SatTable& obj);

} // namespace gnsstk
