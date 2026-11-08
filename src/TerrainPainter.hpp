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

#ifndef TERRAIN_PAINTER_HPP
#  define TERRAIN_PAINTER_HPP

#  include "Colors.hpp"
#  include "Terrain.hpp"
#  include <SFML/Graphics.hpp>
#  include <vector>

// ****************************************************************************
//! \brief Paints the Terrain like a relief map, under the discs of the graph.
//!
//! The land is painted once into a picture (a texture) laid on the world:
//! - the sea gets lighter near the coast, with two lines of equal depth as
//!   on a nautical chart;
//! - the land goes from sand to grass, forest, rock and summits with the
//!   height, tinted by the color of its island, a different shade for each
//!   region;
//! - the relief is lit from the top left (hillshading): slopes facing the
//!   light are brighter, the others darker, so hills look raised;
//! - the borders of the Voronoi cells are drawn: faint inside a region,
//!   firm around the cell of a bookmark, bold between two regions.
//! The rivers are drawn on top as blue bands, wider downstream.
// ****************************************************************************
class TerrainPainter
{
public:

    TerrainPainter(Terrain const& terrain, Palette const& palette);

    //-------------------------------------------------------------------------
    //! \brief Paint the picture and the rivers from the terrain (after
    //! Terrain::build()).
    //-------------------------------------------------------------------------
    void update();

    //! \brief Draw the terrain with the world view of the camera.
    void draw(sf::RenderTarget& target) const;

private:

    //! \brief Height of every pixel of the picture (metres).
    std::vector<float> heights(unsigned width, unsigned height, float pixel) const;
    //! \brief Color of the pixel (x, y) knowing the heights of all pixels.
    sf::Color colorAt(std::vector<float> const& heights, unsigned x, unsigned y,
                      unsigned width, unsigned height, float pixel) const;
    //! \brief Blue bands along the rivers of the terrain.
    void traceRivers();

private:

    Terrain const& m_terrain;
    Palette const& m_palette;
    sf::Texture m_texture;
    sf::Sprite m_sprite;
    sf::VertexArray m_rivers;
};

#endif
