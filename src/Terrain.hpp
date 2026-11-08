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

#ifndef TERRAIN_HPP
#  define TERRAIN_HPP

#  include "Erosion.hpp"
#  include "IslandedBrowser.hpp"
#  include <SFML/Graphics/Rect.hpp>
#  include <cstdint>
#  include <vector>

// ****************************************************************************
//! \brief Turns the graph of bookmarks into real-looking land: coasts,
//! hills, valleys and rivers, under the discs of the map.
//!
//! The method follows A. Patel's "Polygonal Map Generation" and the 3D
//! archipelago of Compages (10e_Archipelago), here in 2D:
//! 1. the map is cut into Voronoi cells (VoronoiGrid);
//! 2. land is laid along the graph: a round patch on each node and a strip
//!    along each edge (island > community > bookmark), with coordinates
//!    bent by noise so the coasts wave. Each cell is land or sea;
//! 3. the sea is the water reached from the edges of the map: water closed
//!    in by land is filled (no lakes);
//! 4. each land cell belongs to the community of the closest node of its
//!    island: the communities become regions, side by side;
//! 5. the relief grows by uplift and river erosion (Erosion.hpp). The ground
//!    rises more inland and on the borders between regions, so each region
//!    becomes a valley between ridges;
//! 6. the cells that drain a large area carry a river.
//!
//! The nodes are not moved: the land is made where the layout put them.
//! Internally the lengths are in metres of the map (a cell is about 170 m),
//! so the sizes of the relief are those of a real island.
// ****************************************************************************
class Terrain
{
public:

    //-------------------------------------------------------------------------
    //! \brief A piece of river, from a cell to the cell downstream.
    //-------------------------------------------------------------------------
    struct Stream
    {
        //! \brief Its ends, in world units (as the layout).
        sf::Vector2f from;
        sf::Vector2f to;
        //! \brief Area drained, counted in cells: the larger, the wider.
        float cells;
    };

    //! \brief Side of a cell (metres): gives the scale of the relief.
    static constexpr float CELL = 167.0f;
    //! \brief Highest ground and deepest sea (metres).
    static constexpr float PEAK = 520.0f;
    static constexpr float SEABED = 90.0f;

public:

    //-------------------------------------------------------------------------
    //! \brief Make the land around the current layout. Takes a fraction of a
    //! second; the layout should have stopped moving.
    //-------------------------------------------------------------------------
    void build(IslandedBrowser const& island);

    //! \brief Forget the land (the layout moves again).
    void clear() { m_ready = false; }
    bool ready() const { return m_ready; }

    //! \brief Rectangle of the world covered by the land and its sea.
    sf::FloatRect area() const { return m_area; }
    //! \brief Smallest rectangle of the world holding all the land.
    sf::FloatRect landBounds() const { return m_land; }
    //! \brief Metres of the map in one world unit.
    float metresPerUnit() const { return m_scale; }

    //-------------------------------------------------------------------------
    //! \brief Height of the ground at a world point (metres, below 0 in the
    //! sea): the heights of the cells blended smoothly, plus a little
    //! roughness on the slopes.
    //-------------------------------------------------------------------------
    float heightAt(sf::Vector2f world) const;

    //! \brief Cell under a world point and its closest border (metres).
    VoronoiGrid::Hit cellAt(sf::Vector2f world) const;

    //! \brief Rank of the island of a cell (-1 for the sea).
    int islandOf(int cell) const { return m_islands_of[std::size_t(cell)]; }
    //! \brief Community (node index) whose region holds the cell, or -1.
    int regionOf(int cell) const { return m_regions_of[std::size_t(cell)]; }
    //! \brief True for the cell where a bookmark stands.
    bool holdsBookmark(int cell) const { return m_parts_of[std::size_t(cell)] >= 0; }
    std::uint32_t seed() const { return m_seed; }

    std::vector<Stream> const& streams() const { return m_streams; }

private:

    //! \brief World units to metres from the top left corner of the area.
    sf::Vector2f toLocal(sf::Vector2f world) const;
    sf::Vector2f toWorld(sf::Vector2f local) const;

    //! \brief Middle of each island and how far its land may grow.
    void measureIslands(ForceDirectedGraph const& layout, size_t islands);
    //! \brief Choose the area, the size of the cells and make the grid.
    void placeGrid(ForceDirectedGraph const& layout);
    //! \brief Steps 2 to 6 of the class description.
    void shapeIslands(IslandedBrowser const& island);
    void floodSea();
    void divideRegions(IslandedBrowser const& island);
    void raiseRelief();
    void traceStreams();
    //! \brief Compute landBounds().
    void findLand();

    //! \brief Blend of the heights of the cells around a local point.
    float smoothHeight(sf::Vector2f local) const;

private:

    bool m_ready = false;
    std::uint32_t m_seed = 7u;
    VoronoiGrid m_grid;
    sf::FloatRect m_area;
    sf::FloatRect m_land;
    float m_scale = 1.0f;
    //! \brief Ideal distance between two linked nodes, in metres.
    float m_spacing = 1.0f;

    //! \brief Per cell (see the accessors above).
    std::vector<int> m_islands_of;
    std::vector<int> m_regions_of;
    std::vector<int> m_parts_of;
    //! \brief Per cell: how its position is bent by noise (-1 to 1 on each
    //! axis), for the coasts and the borders.
    std::vector<sf::Vector2f> m_bends;
    //! \brief Per cell: land where positive, deeper sea where more negative.
    std::vector<float> m_shape;
    //! \brief Per cell: 1 when the water leaves the land there (the sea).
    std::vector<char> m_outlet;
    //! \brief Per cell: final height (metres).
    std::vector<float> m_elevations;
    Drainage m_drainage;
    //! \brief Per island rank: its middle, and how far its land reaches
    //! (world units until the grid is placed, then metres).
    std::vector<sf::Vector2f> m_middles;
    std::vector<float> m_island_sizes;

    std::vector<Stream> m_streams;
};

#endif
