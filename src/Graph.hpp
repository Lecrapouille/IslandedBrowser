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

#ifndef GRAPH_HPP
#  define GRAPH_HPP

#  include <map>
#  include <set>
#  include <vector>
#  include <iostream>

// *****************************************************************************
//! \brief
// *****************************************************************************
class Graph
{
public:

    using Node = size_t;
    using Nodes = std::set<Node>;
    using Neighbors = std::vector<Node>;
    using Edges = std::map<Node, Neighbors>;

    inline void reset()
    {
        m_edges.clear();
        m_nodes.clear();
    }

    inline void add_node(Node const node)
    {
        m_nodes.insert(node);
        m_edges[node];
    }

    void add_edge(Node const from, Node const to)
    {
        add_node(from);
        if (to != from) // No cycles (ugly hack to fix root node with parent refering to itself)
        {
            add_node(to);
            m_edges[from].push_back(to);
            // FIXME: The graph is not necessary bigraph but they are needed for the ForceDirectedGraph
            m_edges[to].push_back(from);
        }
    }

    inline Nodes const& nodes() const
    {
        return m_nodes;
    }

    inline Edges const& edges() const
    {
        return m_edges;
    }

    inline Neighbors const& neighbors(Node const node) const
    {
        auto const& it = m_edges.find(node);
        if (it == m_edges.end())
        {
            static Neighbors const dummy;
            return dummy;
        }
        return it->second;
    }

    inline size_t degree(Node const node) const
    {
        return neighbors(node).size();
    }

    friend std::ostream& operator<<(std::ostream& os, Graph const& g)
    {
        for (auto const& node: g.m_nodes)
        {
            os << node << ":";
            for (auto const& neighbor: g.neighbors(node))
            {
                os << " " << neighbor;
            }
            os << std::endl;
        }
        return os;
    }

private:

    Nodes m_nodes;
    Edges m_edges;
};

#endif
