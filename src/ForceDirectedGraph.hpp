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

#ifndef FORCEDIRECTEDGRAPH_HPP
#  define FORCEDIRECTEDGRAPH_HPP

#  include "Bookmarks.hpp"
#  include <SFML/Graphics/Rect.hpp>
#  include <SFML/System/Vector2.hpp>
#  include <cstdint>
#  include <iosfwd>
#  include <vector>

// *****************************************************************************
//! \brief Force-directed layout of the bookmark tree, prepared for islands.
//!
//! Spring/repulsion model of Fruchterman and Reingold (1991):
//! - repulsion between every pair of vertices: K^2 / d;
//! - attraction along each spring: strength * d^2 / K;
//! - a weak gravity toward the origin keeps disconnected islands together;
//! - displacements are bounded by a temperature that cools down (simulated
//!   annealing), so the layout converges and then stops.
//!
//! Two kinds of springs: the tree edges (island > community > bookmark) and
//! the similarity links computed by organize.py, weaker, which pull related
//! communities and islands toward each other (future rivers).
//!
//! Each island starts in its own disc, on a sunflower spiral: islands are
//! separated from the first step instead of emerging from random noise.
//! Positions are in world units, independent of the window size.
//!
//! See https://youtu.be/WWm-g2nLHds and
//! https://github.com/qdHe/Parallelized-Force-directed-Graph-Drawing
// *****************************************************************************
class ForceDirectedGraph
{
public:

    // *************************************************************************
    //! \brief 2D representation of a node of the bookmark tree.
    // *************************************************************************
    struct Vertex
    {
        sf::Vector2f position;
        sf::Vector2f displacement;
        //! \brief Index of the node in Bookmarks::nodes.
        size_t node;
        //! \brief Rank of its island in Bookmarks::islands.
        size_t island;
    };

    // *************************************************************************
    //! \brief Spring between two vertices (indices in vertices()).
    // *************************************************************************
    struct Spring
    {
        size_t a;
        size_t b;
        float strength;
        //! \brief True for a tree edge, false for a similarity link.
        bool tree;
    };

    // *************************************************************************
    //! \brief Tunable parameters.
    // *************************************************************************
    struct Parameters
    {
        //! \brief Ideal distance between two linked vertices (world units).
        float ideal_distance = 30.0f;
        //! \brief Factor applied to the similarity weight of a link spring.
        float link_strength = 0.3f;
        //! \brief Extra factor for a link between two islands: strong enough
        //! to bring related islands closer, weak enough not to merge them.
        float cross_island_factor = 0.03f;
        //! \brief Pull toward the origin (0 disables it).
        float gravity = 0.1f;
        //! \brief Temperature decay per step.
        float cooling_rate = 0.985f;
        //! \brief Steps computed by each update().
        int steps_per_update = 2;
        //! \brief Seed of the initial positions (the layout is reproducible).
        uint32_t seed = 42u;
    };

    Parameters params;

public:

    //----------------------------------------------------------------------
    //! \brief Build vertices and springs from the bookmarks and place the
    //! islands. The root is not displayed.
    //----------------------------------------------------------------------
    void reset(Bookmarks const& bookmarks);

    //----------------------------------------------------------------------
    //! \brief Restart the annealing from the initial positions.
    //----------------------------------------------------------------------
    void restart();

    //----------------------------------------------------------------------
    //! \brief Compute params.steps_per_update steps while still warm.
    //----------------------------------------------------------------------
    void update();

    //----------------------------------------------------------------------
    //! \brief Compute steps until the layout has cooled down.
    //! \return the number of steps done.
    //----------------------------------------------------------------------
    size_t run();

    //----------------------------------------------------------------------
    //! \brief The layout is still moving.
    //----------------------------------------------------------------------
    bool isRunning() const
    {
        return m_temperature > m_min_temperature;
    }

    //----------------------------------------------------------------------
    //! \brief Bounding box of the vertices.
    //----------------------------------------------------------------------
    sf::FloatRect bounds() const;

    //----------------------------------------------------------------------
    //! \brief Index of the vertex of a bookmark node, or BookmarkNode::none.
    //----------------------------------------------------------------------
    size_t vertexOf(size_t node) const
    {
        return (node < m_vertex_of.size()) ? m_vertex_of[node] : BookmarkNode::none;
    }

    std::vector<Vertex> const& vertices() const { return m_vertices; }
    std::vector<Spring> const& springs() const { return m_springs; }

    friend std::ostream& operator<<(std::ostream& os, ForceDirectedGraph const& g);

private:

    void initializePositions();
    void step();

private:

    std::vector<Vertex> m_vertices;
    std::vector<Spring> m_springs;
    //! \brief Springs of each vertex: (other vertex, strength).
    std::vector<std::vector<std::pair<size_t, float>>> m_adjacency;
    //! \brief Vertex index of each bookmark node.
    std::vector<size_t> m_vertex_of;
    //! \brief Number of vertices of each island.
    std::vector<size_t> m_island_sizes;
    float m_temperature = 0.0f;
    float m_min_temperature = 0.0f;
};

#endif
