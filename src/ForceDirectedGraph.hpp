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

#ifndef FORCEDIRECTEDGRAPH_HPP
#  define FORCEDIRECTEDGRAPH_HPP

#  include "Graph.hpp"
#  include <SFML/System/Vector2.hpp>
#  include <map>
#  include <vector>
#  include <cstdlib>
#  include <cmath>

class ForceDirectedGraph
{
public:

    struct Vertex
    {
        struct Neighbor
        {
            sf::Vector2f* position = nullptr;
            size_t id;
        };

        sf::Vector2f position = { float(rand()) / float(RAND_MAX),
                                  float(rand()) / float(RAND_MAX) };
        sf::Vector2f displacement = { 0.0f, 0.0f };
        std::vector<ForceDirectedGraph::Vertex::Neighbor> neighbors;
        size_t id;
    };

    using Vertices = std::vector<ForceDirectedGraph::Vertex>;

public:

    ForceDirectedGraph(sf::Vector2f const dimension, Graph& graph);
    void reset();
    void step();
    void update();

    inline Vertices const& vertices() const
    {
        return m_vertices;
    }

private:

    inline float distance(sf::Vector2f const& p) const
    {
        return std::max(0.001f, sqrtf(p.x * p.x + p.y * p.y));
    }

    inline float repulsive_force(float const dist) const
    {
        return K * K / dist / float(N);
    }

    inline float attractive_force(float const dist) const
    {
        return dist * dist / K / float(N);
    }

    inline float cooling()
    {
        m_temperature *= 0.95f;
        return m_temperature;
    }

private:

    Graph& m_graph;
    Vertices m_vertices;
    float m_width;
    float m_height;
    float m_temperature;
    float K;
    size_t N;
};

#endif
