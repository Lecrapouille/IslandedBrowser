/* *****************************************************************************
** MIT License
**
** Copyright (c) 2022 Quentin Quadrat
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
#include "Settings.hpp"
#include <algorithm>
#include <unordered_map>
#include <unordered_set>

//------------------------------------------------------------------------------
ForceDirectedGraph::ForceDirectedGraph(sf::Vector2f const dimension, DiGraph& digraph)
    : m_digraph(digraph), m_width(dimension.x), m_height(dimension.y)
{}

//------------------------------------------------------------------------------
void ForceDirectedGraph::reset()
{
    std::vector<DiGraph::Node> nodes(m_digraph.nodes().begin(),
                                     m_digraph.nodes().end());
    const size_t count = nodes.size();
    m_vertices.clear();
    if (count == 0u)
    {
        m_temperature = 0.0f;
        return;
    }

    // Ideal distance, a little tighter than the plain Fruchterman-Reingold k.
    m_K = sqrtf(m_width * m_height / float(count)) * 0.75f;
    m_temperature = std::min(m_width, m_height) / 2.0f;
    m_vertices.resize(count);

    for (size_t n = 0u; n < count; ++n)
    {
        Vertex& v = m_vertices[n];
        v.id = nodes[n];
        v.displacement = { 0.0f, 0.0f };
        v.color = (m_digraph.degree(v.id) == 0u)
                  ? BOOKMARK_COLOR
                  : FOLDER_COLOR;
    }

    initializePositions();
    buildNeighbors();
}

//------------------------------------------------------------------------------
void ForceDirectedGraph::initializePositions()
{
    const sf::Vector2f center(m_width * 0.5f, m_height * 0.5f);
    const float radius = std::min(m_width, m_height) * 0.3f;
    std::uniform_real_distribution<float> distribution(-radius, radius);

    for (auto& v: m_vertices)
    {
        v.position.x = center.x + distribution(m_rng);
        v.position.y = center.y + distribution(m_rng);
    }
}

//------------------------------------------------------------------------------
void ForceDirectedGraph::buildNeighbors()
{
    std::unordered_map<DiGraph::Node, size_t> lookup;
    lookup.reserve(m_vertices.size());
    for (size_t n = 0u; n < m_vertices.size(); ++n)
    {
        lookup[m_vertices[n].id] = n;
    }

    // Undirected and deduplicated: a spring pulls both ends.
    std::vector<std::unordered_set<DiGraph::Node>> neighbor_sets(m_vertices.size());
    for (size_t n = 0u; n < m_vertices.size(); ++n)
    {
        for (DiGraph::Node const neighbor: m_digraph.neighbors(m_vertices[n].id))
        {
            auto const found = lookup.find(neighbor);
            if (found == lookup.end())
            {
                continue;
            }
            neighbor_sets[n].insert(neighbor);
            neighbor_sets[found->second].insert(m_vertices[n].id);
        }
    }

    for (size_t n = 0u; n < m_vertices.size(); ++n)
    {
        Vertex& v = m_vertices[n];
        v.neighbors.clear();
        v.neighbors.reserve(neighbor_sets[n].size());
        for (DiGraph::Node const id: neighbor_sets[n])
        {
            v.neighbors.push_back({ &m_vertices[lookup[id]].position, id });
        }
    }
}

//------------------------------------------------------------------------------
void ForceDirectedGraph::update()
{
    for (int i = 0; i < params.steps_per_frame; ++i)
    {
        if (!isRunning())
        {
            return;
        }
        step();
    }
}

//------------------------------------------------------------------------------
void ForceDirectedGraph::step()
{
    const size_t count = m_vertices.size();
    if (count == 0u)
    {
        return;
    }

    const sf::Vector2f canvas_center(m_width * 0.5f, m_height * 0.5f);
    const float norm = 1.0f / float(count);

    #pragma omp parallel for schedule(dynamic)
    for (size_t i = 0u; i < count; ++i)
    {
        Vertex& v = m_vertices[i];

        // Repulsive forces: every node repels the others.
        for (size_t j = 0u; j < count; ++j)
        {
            if (i == j)
            {
                continue;
            }

            const sf::Vector2f delta = v.position - m_vertices[j].position;
            const float dist = distance(delta);
            const float force = repulsive_force(dist);
            v.displacement += delta * (force / dist * norm);
        }

        // Attractive forces: a spring along each edge.
        for (auto const& neighbor: v.neighbors)
        {
            const sf::Vector2f delta = v.position - (*neighbor.position);
            const float dist = distance(delta);
            const float force = attractive_force(dist);
            v.displacement -= delta * (force / dist * norm);
        }

        // Gravity keeps a wide bookmark tree from spreading into a flat line.
        if (params.gravity > 0.0f)
        {
            const sf::Vector2f to_center = canvas_center - v.position;
            v.displacement += to_center * params.gravity;
        }
    }

    const float min_x = std::max(50.0f, LAYOUT_BORDER_X);
    const float min_y = std::max(50.0f, LAYOUT_BORDER_Y);
    const float max_x = m_width - min_x;
    const float max_y = m_height - min_y;

    #pragma omp parallel for schedule(static)
    for (size_t i = 0u; i < count; ++i)
    {
        Vertex& v = m_vertices[i];
        const float disp_len = distance(v.displacement);
        if (disp_len > m_temperature)
        {
            v.position += v.displacement * (m_temperature / disp_len);
        }
        else
        {
            v.position += v.displacement;
        }

        v.position.x = std::min(max_x, std::max(min_x, v.position.x));
        v.position.y = std::min(max_y, std::max(min_y, v.position.y));
        v.displacement = { 0.0f, 0.0f };
    }

    cooling();
}
