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

#ifndef ISLANDEDBROWSER_HPP
#  define ISLANDEDBROWSER_HPP

#  include "Bookmarks.hpp"
#  include "ForceDirectedGraph.hpp"

// *****************************************************************************
//! \brief
// *****************************************************************************
class IslandedBrowser
{
public:

    using Bookmarks = std::map<int, Bookmark>;
    using Folders = std::map<int, Folder>;

    IslandedBrowser(sf::Vector2f const dimension);

    friend std::ostream& operator<<(std::ostream& os, IslandedBrowser const& island)
    {
        os << island.m_graph;
        return os;
    }

    inline Graph const& graph() const
    {
        return m_graph;
    }

    inline ForceDirectedGraph::Vertices const& vertices() const
    {
        return m_force_directed.vertices();
    }

    //! \brief Deploy the graph nodes.
    void forceDirectedGraph();

private:

    //! \brief Generated code from the ../tool/bookmark.py
    void init(Bookmarks& bookmarks, Folders& folders);

    void createGraph();

private:

    //! \brief Graph of the bookmarks.
    Graph m_graph;
    //! \brief
    ForceDirectedGraph m_force_directed;
    //! \brief Database for the graph (bookmarks).
    Bookmarks m_bookmarks;
    //! \brief Database for the graph (bookmark folders).
    Folders m_folders;
};

#endif
