
#pragma once

#include "FilterParameter.h"

#include <algorithm>
#include <stdexcept>
#include <unordered_map>
#include <vector>

namespace pod
{
    class StateLayout
    {
      public:
        // build layout from vector (order matters!)
        void build(const std::vector<FilterParameter>& params);

        // get Index of parameter p, throw if not found
        int index(const FilterParameter& p) const;

        // Safe check
        bool contains(const FilterParameter& p) const;

        // get parameter by index, throw if index is out of range
        const FilterParameter& param(int idx) const;

        size_t size() const;

      private:
        std::unordered_map<FilterParameter, int> indexMap_;
        std::vector<FilterParameter> indexToParam_;
    };
} // namespace pod
