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

#ifndef MAP_PAINTER_HPP
#  define MAP_PAINTER_HPP

#  include "Camera.hpp"
#  include "Colors.hpp"
#  include "Rivers.hpp"
#  include "Selection.hpp"
#  include "TextPainter.hpp"
#  include <vector>

// ****************************************************************************
//! \brief Draws the archipelago itself: rivers, links, discs and names.
//!
//! The drawing goes from the back to the front, like a painter: first the
//! rivers (in the sea), then the lines, then the discs, then the names on
//! top of everything. Nodes out of the focus (see Selection) are tinted with
//! the color of the sea, so they seem to sink.
// ****************************************************************************
class MapPainter
{
public:

    MapPainter(sf::RenderTarget& target, IslandedBrowser const& island,
               Camera const& camera, Palette const& palette,
               Selection const& selection, Rivers const& rivers,
               TextPainter const& painter);

    //-------------------------------------------------------------------------
    //! \brief Draw the rivers, the links and the discs. Must be called with
    //! the world view of the camera.
    //! \param[in] matches search results to circle (empty when not searching).
    //-------------------------------------------------------------------------
    void drawMap(std::vector<size_t> const& matches);

    //-------------------------------------------------------------------------
    //! \brief Write the names. Must be called with the screen view of the
    //! camera, so the letters keep the same size whatever the zoom.
    //-------------------------------------------------------------------------
    void drawNames();

    //-------------------------------------------------------------------------
    //! \brief Node whose disc is under the pixel, or BookmarkNode::none. When
    //! discs overlap, the biggest wins; a small disc can also be pointed from
    //! a few pixels away, so it is not too hard to catch.
    //-------------------------------------------------------------------------
    size_t nodeAt(sf::Vector2i pixel) const;

    //! \brief Key L: show or hide the similarity links and the rivers.
    void toggleLinks() { m_show_links = !m_show_links; }
    //! \brief Key N: show or hide the names.
    void toggleNames() { m_show_names = !m_show_names; }

private:

    void drawRivers();
    //! \brief Tree lines (island > community > bookmark) and the similarity
    //! links inside each island.
    void drawEdges();
    //! \brief Discs, then rings around the search results and the selection.
    void drawNodes(std::vector<size_t> const& matches);

    //-------------------------------------------------------------------------
    //! \brief Write the title of a node next to its disc (or above it when
    //! centered). Long titles are shortened with "…".
    //-------------------------------------------------------------------------
    void writeName(sf::Text& label, size_t node, sf::Vector2i pixel,
                   unsigned size, sf::Color color, bool centered);

    //-------------------------------------------------------------------------
    //! \brief Color of a disc: the color of its island, lighter for a
    //! bookmark (the sand), white under the mouse, faded out of the focus.
    //-------------------------------------------------------------------------
    sf::Color colorOf(ForceDirectedGraph::Vertex const& vertex) const;

private:

    sf::RenderTarget& m_target;
    IslandedBrowser const& m_island;
    Camera const& m_camera;
    Palette const& m_palette;
    Selection const& m_selection;
    Rivers const& m_rivers;
    TextPainter const& m_painter;
    bool m_show_links = true;
    bool m_show_names = true;
};

#endif
