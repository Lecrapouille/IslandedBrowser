/* *****************************************************************************
** MIT License
**
** Copyright (c) 2022 -- 2026 Quentin Quadrat
**
** Permission is hereby granted, free of charge, to any person obtaining a copy
** of this software and associated documentation files (the "Software"), to deal
** in the Software without restriction, including without limitation the rights
** to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
** copies of the Software, and to permit persons to whom the Software is
** furnished to do so, subject to the following conditions:
**
** The above copyright notice and this permission notice shall be included in all
** copies or substantial portions of the Software.
**
** THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
** IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
** FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
** AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
** LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
** OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
** SOFTWARE.
********************************************************************************
*/

#include "VoronoiGrid.hpp"
#include "Maths.hpp"
#include <algorithm>
#include <cmath>
#include <random>

namespace {

//! \brief How far a site moves from the center of its square (in squares).
//! Below 0.3, the closest site of a point is always among the 3 x 3 squares
//! around it.
float const JITTER = 0.3f;

} // namespace

//------------------------------------------------------------------------------
VoronoiGrid::VoronoiGrid(int columns, int rows, float cell, std::uint32_t seed)
    : m_columns(columns), m_rows(rows), m_cell(cell)
{
    std::mt19937 random(seed);
    std::uniform_real_distribution<float> unit(-JITTER, JITTER);
    m_sites.resize(std::size_t(columns) * std::size_t(rows));
    for (int j = 0; j < rows; ++j)
    {
        for (int i = 0; i < columns; ++i)
        {
            float const x = float(i) + 0.5f + unit(random);
            float const y = float(j) + 0.5f + unit(random);
            m_sites[std::size_t(cellAt(i, j))] = sf::Vector2f(x * cell, y * cell);
        }
    }
}

//------------------------------------------------------------------------------
bool VoronoiGrid::onBorder(int cell) const
{
    int const i = column(cell);
    int const j = row(cell);
    return i == 0 || j == 0 || i == m_columns - 1 || j == m_rows - 1;
}

//------------------------------------------------------------------------------
int VoronoiGrid::squareOf(float coordinate, int count) const
{
    return clamp(int(std::floor(coordinate / m_cell)), 0, count - 1);
}

//------------------------------------------------------------------------------
VoronoiGrid::Around VoronoiGrid::neighbours(int cell) const
{
    Around around;
    int const ci = column(cell);
    int const cj = row(cell);
    for (int j = std::max(0, cj - 1); j <= std::min(m_rows - 1, cj + 1); ++j)
    {
        for (int i = std::max(0, ci - 1); i <= std::min(m_columns - 1, ci + 1); ++i)
        {
            if (i != ci || j != cj)
                around.cells[around.size++] = cellAt(i, j);
        }
    }
    return around;
}

//------------------------------------------------------------------------------
float VoronoiGrid::bisector(sf::Vector2f own, sf::Vector2f other, sf::Vector2f point)
{
    sf::Vector2f const across = other - own;
    sf::Vector2f const middle = (own + other) * 0.5f;
    return ((middle.x - point.x) * across.x + (middle.y - point.y) * across.y) /
           std::sqrt(across.x * across.x + across.y * across.y);
}

//------------------------------------------------------------------------------
VoronoiGrid::Hit VoronoiGrid::hit(sf::Vector2f point) const
{
    int const gi = squareOf(point.x, m_columns);
    int const gj = squareOf(point.y, m_rows);
    Hit result;
    float best = 1.0e30f;
    for (int j = std::max(0, gj - 1); j <= std::min(m_rows - 1, gj + 1); ++j)
    {
        for (int i = std::max(0, gi - 1); i <= std::min(m_columns - 1, gi + 1); ++i)
        {
            sf::Vector2f const d = point - m_sites[std::size_t(cellAt(i, j))];
            float const d2 = d.x * d.x + d.y * d.y;
            if (d2 < best)
            {
                best = d2;
                result.cell = cellAt(i, j);
            }
        }
    }
    // The closest border is the closest bisector with a neighbour.
    sf::Vector2f const own = site(result.cell);
    for (int other: neighbours(result.cell))
    {
        float const edge = bisector(own, site(other), point);
        if (edge < result.edge)
        {
            result.edge = edge;
            result.across = other;
        }
    }
    return result;
}
