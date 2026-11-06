#include "MatrixExtensions.h"

namespace pod
{

    void MatrixExtensions::removeColumns(gnsstk::Matrix<double>& m, std::set<int> cols)
    {
        size_t new_cols = m.cols() - cols.size();
        gnsstk::Matrix<double> m1(m.rows(), new_cols, .0);
        size_t k = 0;

        for (size_t i = 0; i < m.cols(); i++)
        {
            if (cols.find(i) != cols.end())
                continue;

            for (size_t j = 0; j < m.rows(); j++)
                m1(j, k) = m(j, i);
            k++;
        }
        m = m1;
    }

    void MatrixExtensions::removeRows(gnsstk::Matrix<double>& m, std::set<int> rows)
    {
        size_t new_rows = m.rows() - rows.size();
        gnsstk::Matrix<double> m1(new_rows, m.cols(), .0);
        size_t k = 0;

        for (size_t i = 0; i < m.rows(); i++)
        {
            if (rows.find(i) != rows.end())
                continue;

            for (size_t j = 0; j < m.cols(); j++)
                m1(k, j) = m(i, j);
            k++;
        }
        m = m1;
    }

    void MatrixExtensions::removeElms(gnsstk::Vector<double>& v, std::set<int> elms)
    {
        size_t new_size = v.size() - elms.size();
        gnsstk::Vector<double> v1(new_size, .0);

        size_t k = 0;
        for (size_t i = 0; i < v.size(); i++)
        {
            if (elms.find(i) != elms.end())
                continue;
            v1(k) = v(i);
            k++;
        }
        v = v1;
    }
} // namespace pod