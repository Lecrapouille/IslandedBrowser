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

#ifndef SEA_ROUTES_HPP
#  define SEA_ROUTES_HPP

#  include "IslandedBrowser.hpp"
#  include <vector>

// ****************************************************************************
//! \brief Sea routes are the blue bands across the sea joining two islands
//! that talk about close subjects (the rivers are on the land, see Terrain).
//!
//! organize.py links each bookmark to its most similar bookmarks. Most links
//! stay inside an island, but some cross the sea. For each pair of islands,
//! the strengths of the links crossing between them are added up; only the
//! strongest pairs (about two per island) become sea routes, so the map shows the
//! main relations without turning into spaghetti.
// ****************************************************************************
class SeaRoutes
{
public:

    //-------------------------------------------------------------------------
    //! \brief One sea route between two islands.
    //-------------------------------------------------------------------------
    struct Route
    {
        //! \brief Vertices of the two islands (indices in the layout).
        size_t a;
        size_t b;
        //! \brief 1 for the strongest route, smaller for the others.
        float strength;
    };

public:

    explicit SeaRoutes(IslandedBrowser const& island);

    std::vector<Route> const& list() const { return m_routes; }

private:

    std::vector<Route> m_routes;
};

#endif
