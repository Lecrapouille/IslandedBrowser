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
#  include <cmath>
#  include <random>

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
//! Use the spring/repulsion model of Fruchterman and Reingold (1991), in the
//! form used by TimedPetriNetEditor:
//! - Attractive force: af(d) = d^2 / k (connected nodes attract)
//! - Repulsive force: rf(d) = k^2 / d (every node repels the others)
//! - Gravity: pulls nodes toward the center so the layout stays compact
//! Displacements are limited by a temperature that cools down (simulated
//! annealing). k = C * sqrt(area / num_vertices) is the ideal distance.
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

        //! \brief World coordinate position, set by reset().
        sf::Vector2f position = { 0.0f, 0.0f };

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

    //----------------------------------------------------------------------
    //! \brief Algorithm parameters (can be tuned).
    //----------------------------------------------------------------------
    struct Parameters
    {
        //! \brief Gravity toward the center (0 disables it).
        float gravity = 0.1f;
        //! \brief Temperature decay per step.
        float cooling_rate = 0.95f;
        //! \brief Iterations computed by each update().
        int steps_per_frame = 3;
        //! \brief Stop once the temperature falls below this.
        float min_temperature = 0.5f;
    };

    Parameters params;

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
    //! \brief Compute several steps of forces while the layout is still warm.
    //----------------------------------------------------------------------
    void update();

    //----------------------------------------------------------------------
    //! \brief The layout is still moving.
    //----------------------------------------------------------------------
    bool isRunning() const
    {
        return m_temperature >= params.min_temperature;
    }

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
    void initializePositions();
    void buildNeighbors();

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
        return m_K * m_K / distance;
    }

    //----------------------------------------------------------------------
    //! \brief Compute attractive force.
    //! \param[in] distance.
    //----------------------------------------------------------------------
    inline float attractive_force(float const distance) const
    {
        return distance * distance / m_K;
    }

    //----------------------------------------------------------------------
    //! \brief Reduce effect of forces.
    //----------------------------------------------------------------------
    inline void cooling()
    {
        m_temperature *= params.cooling_rate;
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
    float m_temperature = 0.0f;
    //! \brief Ideal distance between nodes: C * sqrt(area / num_vertices).
    float m_K = 1.0f;
    //! \brief Random generator for the initial disk of positions.
    std::mt19937 m_rng{42};
};

#endif
