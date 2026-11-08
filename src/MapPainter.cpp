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

#include "MapPainter.hpp"
#include "Meshes.hpp"
#include "NodeStyle.hpp"
#include <algorithm>
#include <cmath>

//------------------------------------------------------------------------------
MapPainter::MapPainter(sf::RenderTarget& target, IslandedBrowser const& island,
                       Camera const& camera, Palette const& palette,
                       Selection const& selection, SeaRoutes const& routes,
                       TextPainter const& painter)
    : m_target(target), m_island(island), m_camera(camera), m_palette(palette),
      m_selection(selection), m_routes(routes), m_painter(painter)
{}

//------------------------------------------------------------------------------
void MapPainter::drawMap(std::vector<size_t> const& matches)
{
    drawSeaRoutes();
    drawEdges();
    drawNodes(matches);
}

//------------------------------------------------------------------------------
size_t MapPainter::nodeAt(sf::Vector2i pixel) const
{
    sf::Vector2f const world = m_camera.toWorld(pixel);
    float const s = m_camera.scale();
    auto const& nodes = m_island.bookmarks().nodes;
    size_t best = BookmarkNode::none;
    float best_score = 0.0f;
    for (ForceDirectedGraph::Vertex const& v: m_island.layout().vertices())
    {
        // Distance in pixels between the mouse and the center of the disc.
        sf::Vector2f const d = (v.position - world) / s;
        float const distance = std::sqrt(d.x * d.x + d.y * d.y);
        float const radius = nodeRadius(nodes[v.node]);
        if (distance > std::max(radius + 3.0f, PICK_DISTANCE))
            continue;
        // Negative inside the disc, more negative for a bigger disc.
        float const score = distance - radius;
        if (best == BookmarkNode::none || score < best_score)
        {
            best = v.node;
            best_score = score;
        }
    }
    return best;
}

//------------------------------------------------------------------------------
sf::Color MapPainter::colorOf(ForceDirectedGraph::Vertex const& vertex) const
{
    sf::Color color = m_palette.island(vertex.island);
    if (m_island.bookmarks().nodes[vertex.node].isBookmark())
        color = blend(color, sf::Color::White, 0.5f);
    if (vertex.node == m_selection.hovered())
        color = sf::Color::White;
    else if (m_selection.dimmed(vertex.node))
        color = blend(color, SEA_COLOR, DIM_FACTOR);
    return color;
}

//------------------------------------------------------------------------------
//! A sea route is wider and more opaque when the two islands share more similar
//! bookmarks. Its width is given in pixels, so it is converted into world
//! units with the current zoom.
//------------------------------------------------------------------------------
void MapPainter::drawSeaRoutes()
{
    if (!m_show_links)
        return;
    float const s = m_camera.scale();
    auto const& vertices = m_island.layout().vertices();
    sf::VertexArray triangles(sf::Triangles);
    for (SeaRoutes::Route const& route: m_routes.list())
    {
        sf::Color color = SEA_ROUTE_COLOR;
        color.a = sf::Uint8(40.0f + 120.0f * route.strength);
        if (m_selection.dimmed(vertices[route.a].node) &&
            m_selection.dimmed(vertices[route.b].node))
            color.a = sf::Uint8(color.a / 4);
        mesh::band(triangles, vertices[route.a].position, vertices[route.b].position,
                   (1.0f + 7.0f * route.strength) * s, color);
    }
    m_target.draw(triangles);
}

//------------------------------------------------------------------------------
void MapPainter::drawEdges()
{
    ForceDirectedGraph const& layout = m_island.layout();
    auto const& vertices = layout.vertices();
    sf::VertexArray tree(sf::Lines);
    sf::VertexArray links(sf::Lines);
    for (ForceDirectedGraph::Spring const& spring: layout.springs())
    {
        sf::Vector2f const a = vertices[spring.a].position;
        sf::Vector2f const b = vertices[spring.b].position;
        bool const faded = m_selection.dimmed(vertices[spring.a].node) ||
                           m_selection.dimmed(vertices[spring.b].node);
        if (spring.tree)
        {
            // Out of the focus: almost invisible. In the focus: stronger.
            sf::Color color = TREE_EDGE_COLOR;
            if (faded)
                color.a = sf::Uint8(color.a / 4);
            else if (m_selection.hasFocus())
                color.a = sf::Uint8(std::min(255, 2 * int(color.a)));
            mesh::line(tree, a, b, color);
        }
        else if (m_show_links && vertices[spring.a].island == vertices[spring.b].island)
        {
            // Links inside an island, tinted by the island. Links between
            // islands are summarized by the sea routes.
            sf::Color color = colorOf(vertices[spring.a]);
            color.a = faded ? 8 : 25;
            mesh::line(links, a, b, color);
        }
    }
    m_target.draw(links);
    m_target.draw(tree);
}

//------------------------------------------------------------------------------
void MapPainter::drawNodes(std::vector<size_t> const& matches)
{
    // Radii are in pixels: multiplied by the scale, the discs keep the same
    // size on the screen whatever the zoom.
    float const s = m_camera.scale();
    sf::VertexArray discs(sf::Triangles);
    sf::VertexArray rings(sf::Triangles);
    ForceDirectedGraph const& layout = m_island.layout();
    auto const& nodes = m_island.bookmarks().nodes;
    for (ForceDirectedGraph::Vertex const& v: layout.vertices())
    {
        float radius = nodeRadius(nodes[v.node]);
        if (v.node == m_selection.hovered() || v.node == m_selection.hoveredRow())
            radius *= 1.4f;
        mesh::disc(discs, v.position, radius * s, colorOf(v));
    }

    // Search results in blue, then the selection in yellow.
    for (size_t node: matches)
    {
        size_t const vertex = layout.vertexOf(node);
        if (vertex == BookmarkNode::none)
            continue;
        float const r = nodeRadius(nodes[node]);
        mesh::ring(rings, layout.vertices()[vertex].position, (r + 2.0f) * s,
                   (r + 4.0f) * s, URL_COLOR);
    }
    size_t const selected = m_selection.selected();
    size_t const vertex = layout.vertexOf(selected);
    if (selected != BookmarkNode::none && vertex != BookmarkNode::none)
    {
        float const r = nodeRadius(nodes[selected]);
        mesh::ring(rings, layout.vertices()[vertex].position, (r + 3.0f) * s,
                   (r + 6.0f) * s, SELECTION_COLOR);
    }
    m_target.draw(discs);
    m_target.draw(rings);
}

//------------------------------------------------------------------------------
void MapPainter::writeName(sf::Text& label, size_t node, sf::Vector2i pixel,
                           unsigned size, sf::Color color, bool centered)
{
    BookmarkNode const& n = m_island.bookmarks().nodes[node];
    sf::String title = utf8(n.title);
    if (title.getSize() > 40u)
        title = title.substring(0u, 39u) + sf::String(L"…");
    label.setString(title);
    label.setCharacterSize(size);
    // A black outline keeps the letters readable over the lines and discs.
    label.setOutlineThickness(size >= CHARACTER_SIZE ? 2.0f : 1.5f);
    label.setFillColor(color);
    sf::FloatRect const box = label.getLocalBounds();
    float const radius = nodeRadius(n);
    if (centered)
        label.setPosition(std::round(float(pixel.x) - box.width / 2.0f),
                          std::round(float(pixel.y) - box.height - radius - 6.0f));
    else
        label.setPosition(std::round(float(pixel.x) + radius + 4.0f),
                          std::round(float(pixel.y) - float(size) * 0.65f));
    m_target.draw(label);
}

//------------------------------------------------------------------------------
//! Island names are always written. Community names, then bookmark titles,
//! only appear once zoomed in enough to have room for them: written at every
//! zoom, they would cover the whole map.
//------------------------------------------------------------------------------
void MapPainter::drawNames()
{
    if (!m_painter.hasFont() || !m_show_names)
        return;
    Bookmarks const& bookmarks = m_island.bookmarks();
    ForceDirectedGraph const& layout = m_island.layout();
    sf::Vector2f const window = m_painter.windowSize();
    float const zoom = 1.0f / m_camera.scale();
    size_t const selected = m_selection.selected();
    sf::Text label;
    label.setFont(m_painter.font());
    label.setOutlineColor(sf::Color::Black);

    // Names far outside the window are skipped (faster).
    auto visible = [&](sf::Vector2i pixel) {
        return pixel.x > -200 && pixel.y > -50 && float(pixel.x) < window.x + 50.0f &&
               float(pixel.y) < window.y + 50.0f;
    };

    // Small names first: the island names are written over them.
    bool const communities = (zoom >= COMMUNITY_LABEL_ZOOM);
    bool const titles = (zoom >= BOOKMARK_LABEL_ZOOM);
    if (communities)
    {
        for (ForceDirectedGraph::Vertex const& v: layout.vertices())
        {
            BookmarkNode const& node = bookmarks.nodes[v.node];
            bool const wanted = node.isBookmark() ? titles : (node.depth >= 2u);
            if (!wanted || m_selection.dimmed(v.node) || v.node == selected)
                continue;
            sf::Vector2i const pixel = m_camera.toPixel(v.position);
            if (!visible(pixel))
                continue;
            sf::Color const color = node.isBookmark() ? TEXT_COLOR
                : blend(m_palette.island(v.island), sf::Color::White, 0.4f);
            writeName(label, v.node, pixel, node.isBookmark() ? 13u : 15u, color,
                      !node.isBookmark());
        }
    }

    for (size_t island: bookmarks.islands)
    {
        size_t const vertex = layout.vertexOf(island);
        if (vertex == BookmarkNode::none)
            continue;
        sf::Vector2i const pixel = m_camera.toPixel(layout.vertices()[vertex].position);
        sf::Color color = m_palette.island(layout.vertices()[vertex].island);
        if (m_selection.dimmed(island))
            color = blend(color, SEA_COLOR, 0.6f);
        writeName(label, island, pixel, CHARACTER_SIZE, color, true);
    }

    // The selection is always named, so it can be found on the map.
    size_t const vertex = layout.vertexOf(selected);
    if (selected != BookmarkNode::none && vertex != BookmarkNode::none &&
        bookmarks.nodes[selected].depth >= 2u)
    {
        sf::Vector2i const pixel = m_camera.toPixel(layout.vertices()[vertex].position);
        writeName(label, selected, pixel, 15u, SELECTION_COLOR,
                  !bookmarks.nodes[selected].isBookmark());
    }
}
