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

//------------------------------------------------------------------------------
ForceDirectedGraph::ForceDirectedGraph(sf::Vector2f const dimension, DiGraph& digraph)
    : m_digraph(digraph), m_width(dimension.x), m_height(dimension.y)
{}

//------------------------------------------------------------------------------
void ForceDirectedGraph::reset()
{
    // convert std::set to std::vector to have direct access
    std::vector<DiGraph::Node> nodes(m_digraph.nodes().begin(),
                                     m_digraph.nodes().end());
    N = nodes.size();
    K = sqrtf(m_width * m_height / float(N));
    m_temperature = m_width + m_height;
    m_vertices.resize(N);

    // Copy graph nodes to Graph vertices
    #pragma omp parallel for default(shared) schedule(dynamic)
    for (size_t n = 0u; n < N; ++n)
    {
        Vertex& v = m_vertices[n];
        v.id = nodes[n];
        v.position.x *= m_width;
        v.position.y *= m_height;
        v.displacement.x = 0.0f;
        v.displacement.y = 0.0f;
        v.color = (m_digraph.degree(v.id) == 0u)
                  ? BOOKMARK_COLOR
                  : FOLDER_COLOR;
    }

    // Lookup table: Node ID to index on the vector
    std::map<size_t, size_t> lookup;
    for (size_t n = 0u; n < N; ++n)
    {
        lookup[m_vertices[n].id] = n;
    }

    // Add edges "source node" -> "destination node"
    #pragma omp parallel for default(shared) schedule(dynamic)
    for (size_t n = 0u; n < N; ++n)
    {
        Vertex& v = m_vertices[n];
        DiGraph::Neighbors const& neighbors = m_digraph.neighbors(v.id);
        v.neighbors.resize(neighbors.size());

        size_t i = neighbors.size();
        while (i--)
        {
           v.neighbors[i].position = &m_vertices[lookup[neighbors[i]]].position;
           v.neighbors[i].id = m_vertices[lookup[neighbors[i]]].id;
        }
    }

    // We need undirected graph so add edges "destination node" -> "source node"
    for (auto& vertex: m_vertices)
    {
        for (auto& neighbor: vertex.neighbors)
        {
           // Insert if not already present
           Vertex& v = m_vertices[lookup[neighbor.id]];
           size_t i = 0u;
           while ((i < v.neighbors.size()) && (v.neighbors[i].id != vertex.id))
               ++i;
           if (i == v.neighbors.size())
           {
               v.neighbors.push_back({&vertex.position, vertex.id});
           }
        }
    }
}

//------------------------------------------------------------------------------
void ForceDirectedGraph::update()
{
    if (m_temperature < 0.1f)
        return ;

    step();
}

//------------------------------------------------------------------------------
void ForceDirectedGraph::step()
{
    #pragma omp parallel for default(shared) schedule(dynamic)
    for (auto& v: m_vertices)
    {
        // Repulsive forces: nodes -- nodes
        for (auto& u: m_vertices)
        {
            if (u.id == v.id)
                continue ;

            const sf::Vector2f direction(v.position - u.position);
            const float dist = distance(direction);
            const float rf = repulsive_force(dist);
            v.displacement += direction / dist * rf;
        }

        // Attractive forces: edges
        for (auto& u: v.neighbors)
        {
            if (u.id == v.id)
                continue ;

            const sf::Vector2f direction(v.position - (*u.position));
            const float dist = distance(direction);
            const float af = attractive_force(dist);
            v.displacement -= direction / dist * af;
        }
    }

    // Update position and constrain position to the window bounds
    #pragma omp parallel for default(shared) schedule(dynamic)
    for (auto& v: m_vertices)
    {
        const float dist = distance(v.displacement);
        v.position += (dist > m_temperature)
                      ? v.displacement * m_temperature / dist
                      : v.displacement;
        v.position.x = std::min(m_width - LAYOUT_BORDER_X,
                                std::max(LAYOUT_BORDER_X, v.position.x));
        v.position.y = std::min(m_height - LAYOUT_BORDER_Y,
                                std::max(LAYOUT_BORDER_Y, v.position.y));
        v.displacement = { 0.0f, 0.0f };
    }

    cooling();
}
