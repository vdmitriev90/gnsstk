
#include "StateLayout.h"

namespace pod
{
    void StateLayout::build(const std::vector<FilterParameter>& params)
    {
        indexMap_.clear();
        indexToParam_.clear();

        indexToParam_.reserve(params.size());

        for (const auto& p : params)
        {
            // scipt duplicate parameters
            if (indexMap_.find(p) != indexMap_.end())
            {
                GNSSTK_ASSERT_MSG(false, "StateLayout::build - duplicate parameter found: " + p.toString());
                continue;
            }

            int idx = static_cast<int>(indexToParam_.size());
            indexMap_[p] = idx;
            indexToParam_.push_back(p);
        }
    }

    int StateLayout::index(const FilterParameter& p) const
    {
        auto it = indexMap_.find(p);
        if (it == indexMap_.end())
            throw std::runtime_error("StateLayout::index - parameter not found: " + p.toString());

        return it->second;
    }

    bool StateLayout::contains(const FilterParameter& p) const
    {
        return indexMap_.find(p) != indexMap_.end();
    }

    const FilterParameter& StateLayout::param(int idx) const
    {
        if (idx < 0 || static_cast<size_t>(idx) >= indexToParam_.size())
            throw std::out_of_range("StateLayout::param - index out of range");

        return indexToParam_[idx];
    }

    size_t StateLayout::size() const
    {
        return indexToParam_.size();
    }
} // namespace pod
