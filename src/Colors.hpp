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

#ifndef COLORS_HPP
#  define COLORS_HPP

#  include "IslandedBrowser.hpp"
#  include <SFML/Graphics/Color.hpp>
#  include <vector>

//-----------------------------------------------------------------------------
//! \brief Mix two colors like two paints: t = 0 gives only a, t = 1 only b,
//! t = 0.5 half of each. The transparency of a is kept.
//-----------------------------------------------------------------------------
sf::Color blend(sf::Color a, sf::Color b, float t);

//-----------------------------------------------------------------------------
//! \brief Make a color more see-through: alpha = 1 keeps it as it is,
//! alpha = 0 makes it invisible. Used to fade notifications in and out.
//-----------------------------------------------------------------------------
sf::Color fade(sf::Color color, float alpha);

// ****************************************************************************
//! \brief Gives each island its own color, so that the eye groups the dots
//! of the same subject at a glance.
//!
//! The colors are picked around the color wheel by steps of the golden
//! ratio (0.618 of a turn): whatever the number of islands, two islands that
//! come one after the other never get similar colors.
// ****************************************************************************
class Palette
{
public:

    explicit Palette(IslandedBrowser const& island);

    //-------------------------------------------------------------------------
    //! \brief Color of an island given by its rank (0 for the first island,
    //! 1 for the second...), as stored in ForceDirectedGraph::Vertex::island.
    //-------------------------------------------------------------------------
    sf::Color island(size_t rank) const;

    //-------------------------------------------------------------------------
    //! \brief Color of the island a node belongs to, or the plain text color
    //! when the node is not on the map (the hidden root).
    //-------------------------------------------------------------------------
    sf::Color ofNode(size_t node) const;

private:

    //-------------------------------------------------------------------------
    //! \brief The index-th color of the wheel (see the class description).
    //-------------------------------------------------------------------------
    static sf::Color hue(size_t index);

private:

    IslandedBrowser const& m_island;
    //! \brief One color per island, in the order of Bookmarks::islands.
    std::vector<sf::Color> m_colors;
};

#endif
