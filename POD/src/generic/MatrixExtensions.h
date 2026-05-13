#pragma once

#include "Matrix.hpp"
#include "Vector.hpp"

#include <set>

namespace pod
{
    //
    class MatrixExtensions
    {
      public:
        static void removeColumns(gnsstk::Matrix<double>& m, std::set<int> cols);

        static void removeRows(gnsstk::Matrix<double>& m, std::set<int> rows);

        static void removeElms(gnsstk::Vector<double>& v, std::set<int> elms);
    };
} // namespace pod