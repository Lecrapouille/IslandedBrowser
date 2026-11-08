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

#include "ForceDirectedGraph.hpp"
#include <algorithm>
#include <cmath>
#include <numeric>
#include <ostream>
#include <random>

namespace {

float length(sf::Vector2f const& v)
{
    return std::sqrt(v.x * v.x + v.y * v.y);
}

} // namespace

//------------------------------------------------------------------------------
void ForceDirectedGraph::reset(Bookmarks const& bookmarks)
{
    m_vertices.clear();
    m_springs.clear();
    m_vertex_of.assign(bookmarks.nodes.size(), BookmarkNode::none);
    m_island_sizes.assign(bookmarks.islands.size(), 0u);

    std::vector<size_t> island_rank(bookmarks.nodes.size(), BookmarkNode::none);
    for (size_t rank = 0u; rank < bookmarks.islands.size(); ++rank)
        island_rank[bookmarks.islands[rank]] = rank;

    // Every node but the root becomes a vertex.
    for (size_t node = 1u; node < bookmarks.nodes.size(); ++node)
    {
        size_t const island = island_rank[bookmarks.nodes[node].island];
        m_vertex_of[node] = m_vertices.size();
        m_vertices.push_back({ {0.0f, 0.0f}, {0.0f, 0.0f}, node, island });
        ++m_island_sizes[island];
    }

    // Tree edges, except those of the root: islands are only held together
    // by gravity and by the similarity links.
    for (size_t node = 1u; node < bookmarks.nodes.size(); ++node)
    {
        size_t const parent = bookmarks.nodes[node].parent;
        if (parent != 0u && parent != BookmarkNode::none)
            m_springs.push_back({ m_vertex_of[parent], m_vertex_of[node], 1.0f, true });
    }
    for (BookmarkLink const& link: bookmarks.links)
    {
        size_t const a = vertexOf(link.from);
        size_t const b = vertexOf(link.to);
        if (a == BookmarkNode::none || b == BookmarkNode::none)
            continue;
        float strength = params.link_strength * link.weight;
        if (m_vertices[a].island != m_vertices[b].island)
            strength *= params.cross_island_factor;
        m_springs.push_back({ a, b, strength, false });
    }

    m_adjacency.assign(m_vertices.size(), {});
    for (Spring const& spring: m_springs)
    {
        m_adjacency[spring.a].push_back({ spring.b, spring.strength });
        m_adjacency[spring.b].push_back({ spring.a, spring.strength });
    }
    restart();
}

//------------------------------------------------------------------------------
void ForceDirectedGraph::restart()
{
    float const k = params.ideal_distance;
    m_temperature = k * 10.0f;
    m_min_temperature = k * 0.05f;
    initializePositions();
}

//------------------------------------------------------------------------------
//! Islands are sorted by size and placed on a sunflower spiral (golden
//! angle), each one in a disc whose area is proportional to its size.
//------------------------------------------------------------------------------
void ForceDirectedGraph::initializePositions()
{
    float const k = params.ideal_distance;
    size_t const count = m_island_sizes.size();
    std::vector<size_t> order(count);
    std::iota(order.begin(), order.end(), 0u);
    std::stable_sort(order.begin(), order.end(), [this](size_t a, size_t b) {
        return m_island_sizes[a] > m_island_sizes[b];
    });

    std::vector<float> radius(count);
    float mean_radius = 0.0f;
    for (size_t i = 0u; i < count; ++i)
    {
        radius[i] = k * std::sqrt(float(m_island_sizes[i]) + 1.0f) * 0.6f;
        mean_radius += radius[i] / float(count);
    }

    float const golden_angle = 2.39996323f;
    std::vector<sf::Vector2f> centers(count);
    for (size_t rank = 0u; rank < count; ++rank)
    {
        float const distance = 2.5f * mean_radius * std::sqrt(float(rank));
        float const angle = golden_angle * float(rank);
        centers[order[rank]] = { distance * std::cos(angle), distance * std::sin(angle) };
    }

    std::mt19937 rng(params.seed);
    std::uniform_real_distribution<float> unit(0.0f, 1.0f);
    for (Vertex& v: m_vertices)
    {
        // Uniform in the disc of the island.
        float const r = radius[v.island] * std::sqrt(unit(rng));
        float const a = 6.2831853f * unit(rng);
        v.position = centers[v.island] + sf::Vector2f(r * std::cos(a), r * std::sin(a));
        v.displacement = { 0.0f, 0.0f };
    }
}

//------------------------------------------------------------------------------
void ForceDirectedGraph::update()
{
    for (int i = 0; i < params.steps_per_update && isRunning(); ++i)
        step();
}

//------------------------------------------------------------------------------
size_t ForceDirectedGraph::run()
{
    size_t steps = 0u;
    while (isRunning())
    {
        step();
        ++steps;
    }
    return steps;
}

//------------------------------------------------------------------------------
void ForceDirectedGraph::step()
{
    long const count = long(m_vertices.size());
    float const k = params.ideal_distance;
    float const k2 = k * k;
    float const min_distance = k * 0.01f;

    #pragma omp parallel for schedule(dynamic, 64)
    for (long i = 0; i < count; ++i)
    {
        Vertex& v = m_vertices[size_t(i)];
        sf::Vector2f force(0.0f, 0.0f);

        // Repulsion: every vertex pushes the others away.
        for (long j = 0; j < count; ++j)
        {
            if (i == j)
                continue;
            sf::Vector2f const delta = v.position - m_vertices[size_t(j)].position;
            float const d2 = std::max(delta.x * delta.x + delta.y * delta.y,
                                      min_distance * min_distance);
            // (delta / d) * (k^2 / d) = delta * k^2 / d^2
            force += delta * (k2 / d2);
        }

        // Attraction along the springs: (delta / d) * strength * d^2 / k.
        for (auto const& spring: m_adjacency[size_t(i)])
        {
            sf::Vector2f const delta = v.position - m_vertices[spring.first].position;
            float const d = std::max(length(delta), min_distance);
            force -= delta * (spring.second * d / k);
        }

        // Gravity toward the origin.
        force -= v.position * params.gravity;
        v.displacement = force;
    }

    #pragma omp parallel for schedule(static)
    for (long i = 0; i < count; ++i)
    {
        Vertex& v = m_vertices[size_t(i)];
        float const d = length(v.displacement);
        if (d > m_temperature)
            v.displacement *= m_temperature / d;
        v.position += v.displacement;
    }

    m_temperature *= params.cooling_rate;
}

//------------------------------------------------------------------------------
sf::FloatRect ForceDirectedGraph::bounds() const
{
    if (m_vertices.empty())
        return sf::FloatRect(0.0f, 0.0f, 1.0f, 1.0f);
    sf::Vector2f low = m_vertices[0].position;
    sf::Vector2f high = low;
    for (Vertex const& v: m_vertices)
    {
        low.x = std::min(low.x, v.position.x);
        low.y = std::min(low.y, v.position.y);
        high.x = std::max(high.x, v.position.x);
        high.y = std::max(high.y, v.position.y);
    }
    return sf::FloatRect(low, high - low);
}

//------------------------------------------------------------------------------
std::ostream& operator<<(std::ostream& os, ForceDirectedGraph const& g)
{
    for (ForceDirectedGraph::Vertex const& v: g.m_vertices)
    {
        os << v.node << " island " << v.island << " (" << v.position.x << ", "
           << v.position.y << ")\n";
    }
    return os;
}
