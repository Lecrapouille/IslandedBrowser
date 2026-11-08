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

#include "Terrain.hpp"
#include "Maths.hpp"
#include "Noise.hpp"
#include <algorithm>
#include <cmath>

using noise::fbm;
using noise::smoothstep;

namespace {

//! \brief At most this many cells along the longest side of the map.
float const MAX_CELLS = 260.0f;
//! \brief How far the noise moves the coasts (at most) and the borders
//! between regions (metres).
float const COAST_BEND = 1100.0f;
float const BORDER_BEND = 220.0f;
//! \brief Deep sea kept along the edges of the map (metres).
float const EDGE_SEA = 1200.0f;
//! \brief Steps of erosion (40 000 years each): older land, deeper valleys.
int const EROSION_STEPS = 60;
//! \brief Uplift at the heart of an island: 1 mm a year.
float const UPLIFT = 0.001f;
//! \brief Extra uplift at the summit of an island, and at the summit of the
//! biggest community (a smaller community rises less).
float const ISLAND_SUMMIT = 1.6f;
float const COMMUNITY_SUMMIT = 1.0f;
//! \brief Land cells are at least this high; the sea floor starts at this
//! depth (metres).
float const SHORE = 3.0f;
float const SEA_FLOOR = 6.0f;
//! \brief A river shorter than this many cells is not drawn.
size_t const SHORTEST_RIVER = 3u;

//------------------------------------------------------------------------------
//! \brief A round piece of land: a disc (a == b) or a strip with round ends
//! (a capsule) along an edge of the graph.
//------------------------------------------------------------------------------
struct Shape
{
    sf::Vector2f a;
    sf::Vector2f b;
    float radius;
    int island;
};

//------------------------------------------------------------------------------
//! \brief How much a shape covers a point: 1 on its axis, 0 at its radius.
//------------------------------------------------------------------------------
float cover(Shape const& shape, sf::Vector2f point)
{
    sf::Vector2f const ab = shape.b - shape.a;
    float const ab2 = std::max(ab.x * ab.x + ab.y * ab.y, 1.0e-6f);
    sf::Vector2f const ap = point - shape.a;
    float const t = clamp((ap.x * ab.x + ap.y * ab.y) / ab2, 0.0f, 1.0f);
    sf::Vector2f const d = ap - ab * t;
    float const t2 = (d.x * d.x + d.y * d.y) / (shape.radius * shape.radius);
    return (t2 < 1.0f) ? (1.0f - t2) * (1.0f - t2) : 0.0f;
}

//------------------------------------------------------------------------------
//! \brief The community a node belongs to: its ancestor just below the
//! island (the node itself for a community), -1 for an island.
//------------------------------------------------------------------------------
int regionOfNode(Bookmarks const& bookmarks, size_t node)
{
    if (bookmarks.nodes[node].depth < 2u)
        return -1;
    while (bookmarks.nodes[node].depth > 2u)
        node = bookmarks.nodes[node].parent;
    return int(node);
}

} // namespace

//------------------------------------------------------------------------------
sf::Vector2f Terrain::toLocal(sf::Vector2f world) const
{
    return sf::Vector2f(world.x - m_area.left, world.y - m_area.top) * m_scale;
}

//------------------------------------------------------------------------------
sf::Vector2f Terrain::toWorld(sf::Vector2f local) const
{
    return sf::Vector2f(m_area.left, m_area.top) + local / m_scale;
}

//------------------------------------------------------------------------------
VoronoiGrid::Hit Terrain::cellAt(sf::Vector2f world) const
{
    return m_grid.hit(toLocal(world));
}

//------------------------------------------------------------------------------
void Terrain::build(IslandedBrowser const& island)
{
    m_rivers.clear();
    m_summits.clear();
    measureIslands(island.layout(), island.bookmarks().islands.size());
    placeGrid(island.layout());
    shapeIslands(island);
    floodSea();
    divideRegions(island);
    placeSummits(island);
    raiseRelief();
    traceRivers();
    findLand();
    m_ready = true;
}

//------------------------------------------------------------------------------
//! The layout keeps the islands far apart: land hugging the nodes would only
//! be a few crumbs in the ocean. Each island grows into the free sea around
//! it, up to a bit less than halfway to its closest neighbour, so a strait
//! always remains between two islands.
//------------------------------------------------------------------------------
void Terrain::measureIslands(ForceDirectedGraph const& layout, size_t islands)
{
    float const k = layout.params.ideal_distance;
    auto const& vertices = layout.vertices();

    // Middle and spread (root mean square distance to the middle).
    m_middles.assign(islands, sf::Vector2f(0.0f, 0.0f));
    std::vector<float> counts(islands, 0.0f);
    for (ForceDirectedGraph::Vertex const& v: vertices)
    {
        m_middles[v.island] += v.position;
        counts[v.island] += 1.0f;
    }
    for (size_t i = 0u; i < islands; ++i)
        m_middles[i] /= std::max(1.0f, counts[i]);
    std::vector<float> spreads(islands, 0.0f);
    for (ForceDirectedGraph::Vertex const& v: vertices)
    {
        sf::Vector2f const d = v.position - m_middles[v.island];
        spreads[v.island] += (d.x * d.x + d.y * d.y) / std::max(1.0f, counts[v.island]);
    }

    m_island_sizes.assign(islands, 0.0f);
    for (size_t i = 0u; i < islands; ++i)
    {
        float const spread = std::max(k, std::sqrt(spreads[i]));
        float room = 1.0e30f;
        for (size_t j = 0u; j < islands; ++j)
        {
            sf::Vector2f const d = m_middles[j] - m_middles[i];
            if (j != i)
                room = std::min(room, 0.5f * std::sqrt(d.x * d.x + d.y * d.y));
        }
        m_island_sizes[i] = std::max(0.9f * spread, std::min(0.8f * room, 4.0f * spread));
    }
}

//------------------------------------------------------------------------------
//! The land covers the layout and the grown islands, plus a margin of sea.
//! The cells are small enough to give several cells between two linked
//! nodes, but not more than MAX_CELLS along a side, so a huge collection
//! stays fast.
//------------------------------------------------------------------------------
void Terrain::placeGrid(ForceDirectedGraph const& layout)
{
    float const k = layout.params.ideal_distance;
    sf::FloatRect box = layout.bounds();
    for (size_t i = 0u; i < m_middles.size(); ++i)
    {
        // The coast can wave up to half the size of the island further.
        float const reach = 1.5f * m_island_sizes[i];
        float const right = std::max(box.left + box.width, m_middles[i].x + reach);
        float const bottom = std::max(box.top + box.height, m_middles[i].y + reach);
        box.left = std::min(box.left, m_middles[i].x - reach);
        box.top = std::min(box.top, m_middles[i].y - reach);
        box.width = right - box.left;
        box.height = bottom - box.top;
    }
    float const margin = std::max(4.0f * k, 0.06f * std::max(box.width, box.height));
    float const width = box.width + 2.0f * margin;
    float const height = box.height + 2.0f * margin;
    float const cell = std::max(k / 4.0f, std::max(width, height) / MAX_CELLS);
    int const columns = int(std::ceil(width / cell));
    int const rows = int(std::ceil(height / cell));
    m_area = sf::FloatRect(box.left - margin, box.top - margin,
                           float(columns) * cell, float(rows) * cell);
    m_scale = CELL / cell;
    m_spacing = k * m_scale;
    m_grid = VoronoiGrid(columns, rows, CELL, m_seed);
    // From now on, the islands are measured in metres.
    for (size_t i = 0u; i < m_middles.size(); ++i)
    {
        m_middles[i] = toLocal(m_middles[i]);
        m_island_sizes[i] *= m_scale;
    }
}

//------------------------------------------------------------------------------
//! Shapes on the graph: a core in the middle of each island, a disc on each
//! node, a strip along each edge of the tree. Each cell keeps the shape that
//! covers it most, looked up at a position bent by noise: the coasts wave.
//------------------------------------------------------------------------------
void Terrain::shapeIslands(IslandedBrowser const& island)
{
    Bookmarks const& bookmarks = island.bookmarks();
    ForceDirectedGraph const& layout = island.layout();
    auto const& vertices = layout.vertices();
    float const k = m_spacing;

    std::vector<Shape> shapes;
    for (size_t i = 0u; i < m_middles.size(); ++i)
        shapes.push_back({ m_middles[i], m_middles[i], m_island_sizes[i], int(i) });
    for (ForceDirectedGraph::Vertex const& v: vertices)
    {
        BookmarkNode const& node = bookmarks.nodes[v.node];
        sf::Vector2f const p = toLocal(v.position);
        float const radius = node.isBookmark() ? 0.9f * k : (node.depth <= 1u ? 1.4f * k : 1.3f * k);
        shapes.push_back({ p, p, radius, int(v.island) });
        size_t const parent = layout.vertexOf(node.parent);
        if (parent != BookmarkNode::none)
            shapes.push_back({ toLocal(vertices[parent].position), p, 0.7f * k, int(v.island) });
    }

    std::size_t const count = m_grid.count();
    m_bends.resize(count);
    for (std::size_t c = 0u; c < count; ++c)
    {
        sf::Vector2f const p = m_grid.site(int(c));
        m_bends[c] = sf::Vector2f(2.0f * fbm(p.x / 2600.0f, p.y / 2600.0f, m_seed + 11u, 4) - 1.0f,
                                  2.0f * fbm(p.x / 2600.0f, p.y / 2600.0f, m_seed + 12u, 4) - 1.0f);
    }

    // Each shape only looks at the cells of its bounding box.
    std::vector<float> bumps(count, 0.0f);
    m_islands_of.assign(count, -1);
    for (Shape const& shape: shapes)
    {
        float const bend = std::min(COAST_BEND, 0.4f * m_island_sizes[size_t(shape.island)]);
        float const reach = shape.radius + bend + CELL;
        int const i0 = std::max(0, int((std::min(shape.a.x, shape.b.x) - reach) / CELL));
        int const i1 = std::min(m_grid.columns() - 1, int((std::max(shape.a.x, shape.b.x) + reach) / CELL));
        int const j0 = std::max(0, int((std::min(shape.a.y, shape.b.y) - reach) / CELL));
        int const j1 = std::min(m_grid.rows() - 1, int((std::max(shape.a.y, shape.b.y) + reach) / CELL));
        for (int j = j0; j <= j1; ++j)
        {
            for (int i = i0; i <= i1; ++i)
            {
                std::size_t const c = std::size_t(m_grid.cellAt(i, j));
                float const f = cover(shape, m_grid.site(int(c)) + m_bends[c] * bend);
                if (f > bumps[c])
                {
                    bumps[c] = f;
                    m_islands_of[c] = shape.island;
                }
            }
        }
    }

    // Rough coasts; deep sea along the edges of the map.
    float const width = float(m_grid.columns()) * CELL;
    float const height = float(m_grid.rows()) * CELL;
    m_shape.resize(count);
    for (std::size_t c = 0u; c < count; ++c)
    {
        sf::Vector2f const p = m_grid.site(int(c));
        float const edge = std::min(std::min(p.x, width - p.x), std::min(p.y, height - p.y));
        float const rough = 0.12f + 0.4f * std::min(1.0f, 3.0f * bumps[c]);
        // Far from any island, the open sea is deep: no shallows in the
        // middle of nowhere.
        float const open_sea = 0.12f * (1.0f - std::min(1.0f, 4.0f * bumps[c]));
        m_shape[c] = bumps[c] + rough * (fbm(p.x / 1500.0f, p.y / 1500.0f, m_seed, 5) - 0.5f) -
                     0.12f - open_sea - 0.6f * std::max(0.0f, 1.0f - edge / EDGE_SEA);
    }
}

//------------------------------------------------------------------------------
//! Fill the sea from the edges of the map, cell by cell (like paint poured
//! from the borders). Water that the paint cannot reach is surrounded by
//! land: it would be a lake, it becomes land.
//------------------------------------------------------------------------------
void Terrain::floodSea()
{
    std::size_t const count = m_grid.count();
    std::vector<char> sea(count, 0);
    std::vector<int> open;
    auto reach = [&](int i, int j) {
        if (i < 0 || j < 0 || i >= m_grid.columns() || j >= m_grid.rows())
            return;
        int const c = m_grid.cellAt(i, j);
        if (sea[std::size_t(c)] == 0 && m_shape[std::size_t(c)] <= 0.0f)
        {
            sea[std::size_t(c)] = 1;
            open.push_back(c);
        }
    };
    for (int i = 0; i < m_grid.columns(); ++i)
    {
        reach(i, 0);
        reach(i, m_grid.rows() - 1);
    }
    for (int j = 0; j < m_grid.rows(); ++j)
    {
        reach(0, j);
        reach(m_grid.columns() - 1, j);
    }
    while (!open.empty())
    {
        int const c = open.back();
        open.pop_back();
        int const i = m_grid.column(c);
        int const j = m_grid.row(c);
        reach(i + 1, j);
        reach(i - 1, j);
        reach(i, j + 1);
        reach(i, j - 1);
    }
    m_outlet.resize(count);
    for (std::size_t c = 0u; c < count; ++c)
        m_outlet[c] = (sea[c] != 0 || m_grid.onBorder(int(c))) ? 1 : 0;
}

//------------------------------------------------------------------------------
//! Each land cell joins the community of the closest node of its island.
//! The position is bent a little so the borders between regions wave. The
//! cell under a bookmark is remembered: it is painted a little stronger.
//------------------------------------------------------------------------------
void Terrain::divideRegions(IslandedBrowser const& island)
{
    Bookmarks const& bookmarks = island.bookmarks();
    struct Node
    {
        sf::Vector2f at;
        int region;
    };
    std::vector<std::vector<Node>> nodes(bookmarks.islands.size());
    for (ForceDirectedGraph::Vertex const& v: island.layout().vertices())
    {
        int const region = regionOfNode(bookmarks, v.node);
        if (region >= 0)
            nodes[v.island].push_back({ toLocal(v.position), region });
    }

    std::size_t const count = m_grid.count();
    m_regions_of.assign(count, -1);
    m_parts_of.assign(count, -1);
    for (std::size_t c = 0u; c < count; ++c)
    {
        if (m_outlet[c] != 0 || m_islands_of[c] < 0)
            continue;
        sf::Vector2f const p = m_grid.site(int(c)) + m_bends[c] * BORDER_BEND;
        float best = 1.0e30f;
        for (Node const& node: nodes[std::size_t(m_islands_of[c])])
        {
            sf::Vector2f const d = p - node.at;
            float const d2 = d.x * d.x + d.y * d.y;
            if (d2 < best)
            {
                best = d2;
                m_regions_of[c] = node.region;
            }
        }
    }

    for (ForceDirectedGraph::Vertex const& v: island.layout().vertices())
    {
        if (!bookmarks.nodes[v.node].isBookmark())
            continue;
        int const c = m_grid.hit(toLocal(v.position)).cell;
        if (m_outlet[std::size_t(c)] == 0)
            m_parts_of[std::size_t(c)] = int(v.node);
    }
}

//------------------------------------------------------------------------------
//! The ground rises more far from the coast and on the borders between
//! regions; erosion then digs the valleys. The relief the erosion gives has
//! its own scale: it is stretched so the highest cell reaches the PEAK.
//------------------------------------------------------------------------------
void Terrain::raiseRelief()
{
    std::size_t const count = m_grid.count();

    // Steps of cells from the sea, by a breadth first walk from the outlets.
    std::vector<int> inland(count, -1);
    std::vector<int> queue;
    for (std::size_t c = 0u; c < count; ++c)
    {
        if (m_outlet[c] != 0)
        {
            inland[c] = 0;
            queue.push_back(int(c));
        }
    }
    for (std::size_t head = 0u; head < queue.size(); ++head)
    {
        int const c = queue[head];
        for (int n: m_grid.neighbours(c))
        {
            if (inland[std::size_t(n)] < 0)
            {
                inland[std::size_t(n)] = inland[std::size_t(c)] + 1;
                queue.push_back(n);
            }
        }
    }
    std::vector<int> deepest(m_island_sizes.size(), 1);
    for (std::size_t c = 0u; c < count; ++c)
    {
        if (m_outlet[c] == 0 && m_islands_of[c] >= 0)
        {
            int& d = deepest[std::size_t(m_islands_of[c])];
            d = std::max(d, inland[c]);
        }
    }

    std::vector<float> ground(count, 0.0f);
    std::vector<float> uplift(count, 0.0f);
    for (std::size_t c = 0u; c < count; ++c)
    {
        if (m_outlet[c] != 0)
            continue;
        sf::Vector2f const p = m_grid.site(int(c));
        // Some areas rise more than others: massifs.
        float const massif = 0.4f + 1.2f * fbm(p.x / 2200.0f, p.y / 2200.0f, m_seed + 61u, 3);
        std::size_t const isle = std::size_t(std::max(0, m_islands_of[c]));
        float const heart = smoothstep(0.0f, std::max(2.0f, 0.7f * float(deepest[isle])),
                                       float(inland[c]));
        // The mountains of the summits: a bell curve around each one.
        float mountains = 0.0f;
        for (Summit const& summit: m_summits)
        {
            sf::Vector2f const d = p - summit.at;
            float const d2 = (d.x * d.x + d.y * d.y) / (summit.radius * summit.radius);
            if (d2 < 9.0f)
                mountains = std::max(mountains, summit.strength * std::exp(-d2));
        }
        uplift[c] = UPLIFT * (0.4f * massif * (0.25f + 0.75f * heart) + mountains);
        ground[c] = 4.0f * noise::value(p.x / 400.0f, p.y / 400.0f, m_seed + 71u);
    }
    m_drainage = erode(m_grid, ground, uplift, m_outlet, EROSION_STEPS);

    float const tallest = std::max(1.0f, *std::max_element(ground.begin(), ground.end()));
    m_elevations.resize(count);
    for (std::size_t c = 0u; c < count; ++c)
    {
        if (m_outlet[c] == 0)
        {
            m_elevations[c] = SHORE + ground[c] * (0.92f * PEAK / tallest);
        }
        else
        {
            // The sea deepens away from the coast.
            m_elevations[c] = -SEABED * smoothstep(0.0f, 0.14f, -m_shape[c]) - SEA_FLOOR;
            m_islands_of[c] = -1;
        }
    }
}

//------------------------------------------------------------------------------
//! The island node is the summit of its island, as wide as half the island.
//! Each community node is a smaller summit, higher when the community holds
//! more bookmarks than the others of its island.
//------------------------------------------------------------------------------
void Terrain::placeSummits(IslandedBrowser const& island)
{
    Bookmarks const& bookmarks = island.bookmarks();
    ForceDirectedGraph const& layout = island.layout();
    std::vector<size_t> biggest(bookmarks.islands.size(), 1u);
    for (ForceDirectedGraph::Vertex const& v: layout.vertices())
    {
        BookmarkNode const& node = bookmarks.nodes[v.node];
        if (!node.isBookmark() && node.depth == 2u)
            biggest[v.island] = std::max(biggest[v.island], island.bookmarkCount(v.node));
    }

    std::vector<Summit> communities;
    for (ForceDirectedGraph::Vertex const& v: layout.vertices())
    {
        BookmarkNode const& node = bookmarks.nodes[v.node];
        sf::Vector2f const at = toLocal(v.position);
        int const cell = m_grid.hit(at).cell;
        if (m_outlet[std::size_t(cell)] != 0 || node.isBookmark() || node.depth > 2u)
            continue;
        if (node.depth <= 1u)
        {
            m_summits.push_back({ cell, at, ISLAND_SUMMIT, 0.55f * m_island_sizes[v.island] });
        }
        else
        {
            float const share = float(island.bookmarkCount(v.node)) / float(biggest[v.island]);
            communities.push_back({ cell, at, COMMUNITY_SUMMIT * (0.5f + 0.5f * std::sqrt(share)),
                                    2.2f * m_spacing });
        }
    }
    std::stable_sort(communities.begin(), communities.end(), [](Summit const& a, Summit const& b) {
        return a.strength > b.strength;
    });
    m_summits.insert(m_summits.end(), communities.begin(), communities.end());
}

//------------------------------------------------------------------------------
//! Among the neighbours lower than the cell, the one reached by the steepest
//! slope (height lost per metre). A cell lower than all its neighbours is
//! the bottom of a hollow: its water overflows where the priority flood
//! found the lowest pass (Erosion.hpp).
//------------------------------------------------------------------------------
int Terrain::downhill(int cell) const
{
    int best = -1;
    float steepest = 0.0f;
    float const here = m_elevations[std::size_t(cell)];
    for (int n: m_grid.neighbours(cell))
    {
        sf::Vector2f const d = m_grid.site(n) - m_grid.site(cell);
        float const slope = (here - m_elevations[std::size_t(n)]) /
                            std::sqrt(d.x * d.x + d.y * d.y);
        if (slope > steepest)
        {
            steepest = slope;
            best = n;
        }
    }
    return (best >= 0) ? best : m_drainage.receivers[std::size_t(cell)];
}

//------------------------------------------------------------------------------
//! Like a drop of water, each river leaves its summit and always goes to the
//! lower neighbour down the steepest slope, until it reaches the sea or a
//! river already traced, which it joins. The main rivers (from the island
//! summits) are traced first, so the community rivers become their
//! tributaries. A river is wider where it drains a larger area.
//------------------------------------------------------------------------------
void Terrain::traceRivers()
{
    std::size_t const count = m_grid.count();
    float const cell_world = CELL / m_scale;
    std::vector<char> wet(count, 0);
    for (Summit const& summit: m_summits)
    {
        if (wet[std::size_t(summit.cell)] != 0)
            continue;
        std::vector<int> path;
        std::vector<char> seen(count, 0);
        int c = summit.cell;
        while (c >= 0 && seen[std::size_t(c)] == 0)
        {
            seen[std::size_t(c)] = 1;
            path.push_back(c);
            // The mouth: the sea, or the river it flows into.
            if (m_outlet[std::size_t(c)] != 0 || (wet[std::size_t(c)] != 0 && path.size() > 1u))
                break;
            c = downhill(c);
        }
        if (path.size() < SHORTEST_RIVER)
            continue;

        River river;
        for (std::size_t k = 0u; k < path.size(); ++k)
        {
            std::size_t const cell = std::size_t(path[k]);
            if (k + 1u < path.size())
                wet[cell] = 1;
            float const cells = m_drainage.areas[cell] / (CELL * CELL);
            river.points.push_back(toWorld(m_grid.site(int(cell))));
            float const width = cell_world * std::min(0.6f, 0.12f + 0.045f * std::sqrt(cells));
            // A river never gets narrower downstream.
            river.widths.push_back(river.widths.empty() ? width : std::max(width, river.widths.back()));
        }
        // The spring is the node itself, not the center of its cell: the
        // river visibly leaves the node.
        river.points.front() = toWorld(summit.at);
        m_rivers.push_back(std::move(river));
    }
}

//------------------------------------------------------------------------------
//! Each cell around weighs by a bell curve of its distance: the ground is
//! smooth, without steps at the borders of the cells.
//------------------------------------------------------------------------------
float Terrain::smoothHeight(sf::Vector2f local) const
{
    float const spread = 1.0f / (0.62f * CELL * 0.62f * CELL);
    int const gi = clamp(int(local.x / CELL), 0, m_grid.columns() - 1);
    int const gj = clamp(int(local.y / CELL), 0, m_grid.rows() - 1);
    float sum = 0.0f;
    float weights = 0.0f;
    for (int j = std::max(0, gj - 1); j <= std::min(m_grid.rows() - 1, gj + 1); ++j)
    {
        for (int i = std::max(0, gi - 1); i <= std::min(m_grid.columns() - 1, gi + 1); ++i)
        {
            int const c = m_grid.cellAt(i, j);
            sf::Vector2f const d = local - m_grid.site(c);
            float const w = std::exp(-(d.x * d.x + d.y * d.y) * spread);
            sum += w * m_elevations[std::size_t(c)];
            weights += w;
        }
    }
    return sum / std::max(weights, 1.0e-12f);
}

//------------------------------------------------------------------------------
float Terrain::heightAt(sf::Vector2f world) const
{
    sf::Vector2f const p = toLocal(world);
    float const smooth = smoothHeight(p);
    // Roughness on the slopes, none on the beaches.
    return smooth + 7.0f * (fbm(p.x / 140.0f, p.y / 140.0f, m_seed + 81u, 2) - 0.5f) *
                    smoothstep(8.0f, 60.0f, smooth);
}

//------------------------------------------------------------------------------
void Terrain::findLand()
{
    sf::Vector2f low(1.0e30f, 1.0e30f);
    sf::Vector2f high(-1.0e30f, -1.0e30f);
    float const half = 0.5f * CELL;
    for (std::size_t c = 0u; c < m_grid.count(); ++c)
    {
        if (m_islands_of[c] < 0)
            continue;
        sf::Vector2f const p = m_grid.site(int(c));
        low.x = std::min(low.x, p.x - half);
        low.y = std::min(low.y, p.y - half);
        high.x = std::max(high.x, p.x + half);
        high.y = std::max(high.y, p.y + half);
    }
    if (low.x > high.x)
    {
        m_land = m_area;
        return;
    }
    sf::Vector2f const a = toWorld(low);
    sf::Vector2f const b = toWorld(high);
    m_land = sf::FloatRect(a, b - a);
}
