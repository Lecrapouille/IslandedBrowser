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
#  include <SFML/Graphics/Color.hpp>
#  include <map>
#  include <vector>
#  include <cstdlib>
#  include <cmath>

// *****************************************************************************
//! \brief Force-directed graph drawing algorithms are a class of algorithms for
//! drawing graphs in an aesthetically-pleasing way. Their purpose is to
//! position the nodes of a graph in two-dimensional or three-dimensional space
//! so that all the edges are of more or less equal length and there are as few
//! crossing edges as possible, by assigning forces among the set of edges and
//! the set of nodes, based on their relative positions, and then using these
//! forces either to simulate the motion of the edges and nodes or to minimize
//! their energy.
//!
//! Use the spring/repulsion model of Fruchterman and Reingold (1991) with:
//! - Attractive force: af(d) = d^2 / k
//! - Repulsive force: rf(d) = -k^2 / d
//! where d is distance between two vertices and the optimal distance between
//! vertices k is defined as C * sqrt( area / num_vertices ) where C is a
//! parameter we can adjust.
//!
//! For more information see this video https://youtu.be/WWm-g2nLHds
//! This code source is largely inspired by:
//! https://github.com/qdHe/Parallelized-Force-directed-Graph-Drawing
// *****************************************************************************
class ForceDirectedGraph
{
public:

    // *************************************************************************
    //! \brief Vertex is a 2D representation of a graph node.
    // *************************************************************************
    struct Vertex
    {
        // *********************************************************************
        //! \brief Contrary to a graph structure we just care about position of
        //! neighbors.
        // *********************************************************************
        struct Neighbor
        {
            Neighbor() = default;
            Neighbor(sf::Vector2f* p, size_t i) : position(p), id(i) {}

            //! \brief World coordinate position. Use a pointer to track updates
            sf::Vector2f* position = nullptr;
            //! \brief Reference to the graph node.
            size_t id;
        };

        //! \brief World coordinate position. Position are randomized between 0
        //! and 1. The scaling to the windows dimension is made later since we
        //! prefer this astructure does know to the windows class.
        sf::Vector2f position =
        {
            float(rand()) / float(RAND_MAX),
            float(rand()) / float(RAND_MAX)
        };

        //! \brief Displacement due to attractive and reuplsive forces.
        sf::Vector2f displacement = { 0.0f, 0.0f };
        //! \brief List of neighboring nodes.
        std::vector<ForceDirectedGraph::Vertex::Neighbor> neighbors;
        //! \brief Color
        sf::Color color;
        //! \brief Reference to the graph node.
        size_t id;
    };

    using Vertices = std::vector<ForceDirectedGraph::Vertex>;

public:

    //----------------------------------------------------------------------
    //! \brief Default constructor. Set the dimension of the layout and set
    //! the reference to the graph we have to display.
    //----------------------------------------------------------------------
    ForceDirectedGraph(sf::Vector2f const dimension, DiGraph& digraph);

    //----------------------------------------------------------------------
    //! \brief Restore initial states.
    //----------------------------------------------------------------------
    void reset();

    //----------------------------------------------------------------------
    //! \brief Compute one step of forces if temperature is still hot else
    //! do nothing.
    //----------------------------------------------------------------------
    void update();

    //----------------------------------------------------------------------
    //! \brief Const getter of vertices.
    //----------------------------------------------------------------------
    inline Vertices const& vertices() const
    {
        return m_vertices;
    }

    //----------------------------------------------------------------------
    //! \brief Print on the console the graph.
    //----------------------------------------------------------------------
    friend std::ostream& operator<<(std::ostream& os, ForceDirectedGraph const& g)
    {
        for (auto const& vertex: g.m_vertices)
        {
            os << vertex.id << ":";
            for (auto const& neighbor: vertex.neighbors)
            {
                os << " " << neighbor.id;
            }
            os << std::endl;
        }
        return os;
    }

private:

    //----------------------------------------------------------------------
    //! \brief Do a single step for computing forces.
    //----------------------------------------------------------------------
    void step();

    //----------------------------------------------------------------------
    //! \brief Euclidian norm.
    //! \param[in] p world coordinate position.
    //----------------------------------------------------------------------
    inline float distance(sf::Vector2f const& p) const
    {
        return std::max(0.001f, sqrtf(p.x * p.x + p.y * p.y));
    }

    //----------------------------------------------------------------------
    //! \brief Compute repulsive force.
    //! \param[in] distance.
    //----------------------------------------------------------------------
    inline float repulsive_force(float const distance) const
    {
        return K * K / distance / float(N) / 2.0f;
    }

    //----------------------------------------------------------------------
    //! \brief Compute attractive force.
    //! \param[in] distance.
    //----------------------------------------------------------------------
    inline float attractive_force(float const distance) const
    {
        return distance * distance / K / float(N);
    }

    //----------------------------------------------------------------------
    //! \brief Reduce effect of forces.
    //----------------------------------------------------------------------
    inline float cooling()
    {
        m_temperature *= 0.98f;
        return m_temperature;
    }

private:

    //! \brief The directional graph to display.
    DiGraph& m_digraph;
    //! \brief Collection of nodes to display.
    Vertices m_vertices;
    //! \brief Dimension of the screen.
    float m_width;
    //! \brief Dimension of the screen.
    float m_height;
    //! \brief Reduce effect of forces.
    float m_temperature;
    //! \brief Force coeficient: sqrt(area / num_vertices)
    float K;
    //! \brief Number of vertices.
    size_t N;
};

#endif
