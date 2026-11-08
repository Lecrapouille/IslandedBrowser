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

#ifndef EROSION_HPP
#  define EROSION_HPP

#  include "VoronoiGrid.hpp"
#  include <vector>

// ****************************************************************************
//! \brief How the relief of the islands grows, like real mountains: the
//! ground slowly rises (tectonic uplift) while the rivers dig it.
//!
//! Each cell sends its rain water to its lowest neighbour (its "receiver"),
//! which sends it on, down to the sea. A river digs faster where it carries
//! more water (it drains a larger area) and where it is steeper (stream
//! power law: dh/dt = U - K sqrt(A) S). Slopes also slowly smooth out
//! (creep). Repeated over hundreds of thousands of years, this draws valleys,
//! ridges and river networks that look real (Cordonnier et al. 2016, on a
//! Voronoi diagram as here; Braun and Willett 2013 for the step).
//!
//! A hollow with no way out would trap the water. The "priority flood"
//! (Barnes et al. 2014) fills such hollows up to their lowest pass, as a lake
//! would, so that every drop finds its way to the sea.
// ****************************************************************************

//-----------------------------------------------------------------------------
//! \brief Where the water of each cell goes.
//-----------------------------------------------------------------------------
struct Drainage
{
    //! \brief The cell each cell sends its water to (-1 for the sea).
    std::vector<int> receivers;
    //! \brief The cells from the sea upward: a cell comes after its receiver.
    std::vector<int> order;
    //! \brief Area drained by each cell: its own plus everything upstream (m2).
    std::vector<float> areas;
};

//-----------------------------------------------------------------------------
//! \brief Find where the water of each cell goes.
//! \param[in] heights height of each cell (metres).
//! \param[in] outlet 1 for the cells where the water leaves the land (sea).
//-----------------------------------------------------------------------------
Drainage drain(VoronoiGrid const& grid, std::vector<float> const& heights,
               std::vector<char> const& outlet);

//-----------------------------------------------------------------------------
//! \brief Let the relief grow: uplift, river erosion and creep.
//! \param[in,out] heights height of each cell (metres).
//! \param[in] uplift how fast each cell rises (metres per year).
//! \param[in] steps number of steps of 40 000 years.
//! \return where the water goes on the final relief.
//-----------------------------------------------------------------------------
Drainage erode(VoronoiGrid const& grid, std::vector<float>& heights,
               std::vector<float> const& uplift, std::vector<char> const& outlet,
               int steps);

#endif
