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

#include "IslandedBrowser.hpp"
#include <iostream>

IslandedBrowser::IslandedBrowser(sf::Vector2f const dimension)
   : m_force_directed(dimension, m_graph)
{
    init(m_bookmarks, m_folders);
    createGraph();
    m_force_directed.reset();
}

void IslandedBrowser::createGraph()
{
    m_graph.reset();
    for (auto const& it: m_folders)
    {
        m_graph.add_edge(it.second.parent, it.second.id);
    }

    for (auto const& it: m_bookmarks)
    {
        m_graph.add_edge(it.second.parent, it.second.id);
    }
}

void IslandedBrowser::forceDirectedGraph()
{
    m_force_directed.update();
}
