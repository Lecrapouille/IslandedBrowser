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

#include "Erosion.hpp"
#include <algorithm>
#include <cmath>
#include <functional>
#include <queue>
#include <utility>

namespace {

//! \brief How easily the rock is dug (per year) and how fast slopes creep
//! (m2 per year).
float const ERODIBILITY = 2.0e-5f;
float const CREEP = 0.05f;
//! \brief Duration of one step (years).
float const YEARS = 4.0e4f;

//------------------------------------------------------------------------------
//! The rivers dig, from the sea upward. A cell only needs the new height of
//! its receiver, already computed, so each step is solved exactly:
//!   h = (h + U dt + F h_receiver) / (1 + F),  F = K dt sqrt(A) / distance.
//------------------------------------------------------------------------------
void incise(VoronoiGrid const& grid, std::vector<float>& heights,
            std::vector<float> const& uplift, std::vector<char> const& outlet,
            Drainage const& drainage)
{
    for (int c: drainage.order)
    {
        int const receiver = drainage.receivers[std::size_t(c)];
        if (receiver < 0 || outlet[std::size_t(c)] != 0)
            continue;
        sf::Vector2f const d = grid.site(c) - grid.site(receiver);
        float const distance = std::sqrt(d.x * d.x + d.y * d.y);
        float const factor = ERODIBILITY * YEARS *
                             std::sqrt(drainage.areas[std::size_t(c)]) / distance;
        float const raised = heights[std::size_t(c)] + uplift[std::size_t(c)] * YEARS;
        float const bed = heights[std::size_t(receiver)];
        // A lake (the receiver is higher) is not dug: it fills up.
        heights[std::size_t(c)] = (bed >= raised) ? raised
                                                  : (raised + factor * bed) / (1.0f + factor);
    }
}

//------------------------------------------------------------------------------
//! The slopes creep: each cell moves toward the mean of its 4 neighbours.
//------------------------------------------------------------------------------
void creep(VoronoiGrid const& grid, std::vector<float>& heights,
           std::vector<char> const& outlet)
{
    float const cell = grid.cellSize();
    float const rate = CREEP * YEARS / (cell * cell);
    int const columns = grid.columns();
    std::vector<float> change(heights.size(), 0.0f);
    for (int j = 1; j < grid.rows() - 1; ++j)
    {
        for (int i = 1; i < columns - 1; ++i)
        {
            std::size_t const c = std::size_t(grid.cellAt(i, j));
            if (outlet[c] != 0)
                continue;
            change[c] = rate * (heights[c - 1u] + heights[c + 1u] +
                                heights[c - std::size_t(columns)] +
                                heights[c + std::size_t(columns)] - 4.0f * heights[c]);
        }
    }
    for (std::size_t c = 0u; c < heights.size(); ++c)
        heights[c] += change[c];
}

} // namespace

//------------------------------------------------------------------------------
//! The sea cells are reached first. Then, again and again, the lowest cell
//! reached so far lets the water of its unreached neighbours come to it.
//! The water so always flows toward the sea, over the lowest passes.
//------------------------------------------------------------------------------
Drainage drain(VoronoiGrid const& grid, std::vector<float> const& heights,
               std::vector<char> const& outlet)
{
    std::size_t const count = heights.size();
    Drainage result;
    result.receivers.assign(count, -1);
    result.order.reserve(count);
    std::vector<float> level(heights);
    std::vector<char> reached(count, 0);
    using Entry = std::pair<float, int>;
    std::priority_queue<Entry, std::vector<Entry>, std::greater<Entry>> open;
    for (std::size_t c = 0u; c < count; ++c)
    {
        if (outlet[c] != 0)
        {
            reached[c] = 1;
            open.emplace(heights[c], int(c));
        }
    }
    while (!open.empty())
    {
        int const c = open.top().second;
        open.pop();
        result.order.push_back(c);
        for (int n: grid.neighbours(c))
        {
            if (reached[std::size_t(n)] != 0)
                continue;
            reached[std::size_t(n)] = 1;
            // A hollow is filled up to the level of its pass.
            level[std::size_t(n)] = std::max(heights[std::size_t(n)], level[std::size_t(c)]);
            result.receivers[std::size_t(n)] = c;
            open.emplace(level[std::size_t(n)], n);
        }
    }

    // Each cell gives its water to its receiver, from the sources down.
    float const cell = grid.cellSize();
    result.areas.assign(count, cell * cell);
    for (auto it = result.order.rbegin(); it != result.order.rend(); ++it)
    {
        int const receiver = result.receivers[std::size_t(*it)];
        if (receiver >= 0)
            result.areas[std::size_t(receiver)] += result.areas[std::size_t(*it)];
    }
    return result;
}

//------------------------------------------------------------------------------
Drainage erode(VoronoiGrid const& grid, std::vector<float>& heights,
               std::vector<float> const& uplift, std::vector<char> const& outlet,
               int steps)
{
    for (int step = 0; step < steps; ++step)
    {
        incise(grid, heights, uplift, outlet, drain(grid, heights, outlet));
        creep(grid, heights, outlet);
    }
    return drain(grid, heights, outlet);
}
