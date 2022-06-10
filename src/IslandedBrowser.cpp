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

// -----------------------------------------------------------------------------
IslandedBrowser::IslandedBrowser(sf::Vector2f const dimension)
   : m_force_directed(dimension, m_digraph)
{
    init(m_bookmarks, m_folders);
    createGraph();
    m_force_directed.reset();
}

// -----------------------------------------------------------------------------
void IslandedBrowser::createGraph()
{
    m_digraph.reset();
    for (auto const& it: m_folders)
    {
        m_digraph.add_edge(it.second.parent, it.second.id);
    }

    for (auto const& it: m_bookmarks)
    {
        m_digraph.add_edge(it.second.parent, it.second.id);
    }
}

// TODO: to be cleaned !!!!

// -----------------------------------------------------------------------------
std::string const& IslandedBrowser::getURL(sf::Vector2i mouse)
{
    m_cache_urls.clear();

    for (auto const& v: m_force_directed.vertices())
    {
        // Squared distance
        const float distance =
                   (float(mouse.x) - v.position.x) *
                   (float(mouse.x) - v.position.x) +
                   (float(mouse.y) - v.position.y) *
                   (float(mouse.y) - v.position.y);

        if (distance < 16.0f)
        {
            getURL_chapo(v.id);
            return m_cache_urls;
        }
    }

    return m_cache_urls;
}

// -----------------------------------------------------------------------------
//! \note Only working with graph which are trees and directed else you have
//! manage infinite recursion by marking visited nodes.
void IslandedBrowser::getURL_chapo(DiGraph::Node const& node)
{
    if (m_digraph.degree(node) == 0u)
    {
        m_cache_urls += " ";
        m_cache_urls += m_bookmarks[node].uri;
    }
    else
    {
        for (const auto& it: m_digraph.neighbors(node))
        {
             getURL_chapo(it);
        }
    }
}

// -----------------------------------------------------------------------------
std::string const& IslandedBrowser::getTitle(sf::Vector2i mouse)
{
    m_cache_urls.clear();

    for (auto const& v: m_force_directed.vertices())
    {
        // Squared distance
        const float distance =
                   (float(mouse.x) - v.position.x) *
                   (float(mouse.x) - v.position.x) +
                   (float(mouse.y) - v.position.y) *
                   (float(mouse.y) - v.position.y);

        if (distance < 16.0f)
        {
            getTitle_chapo(v.id);
            return m_cache_urls;
        }
    }

    return m_cache_urls;
}

// -----------------------------------------------------------------------------
void IslandedBrowser::getTitle_chapo(DiGraph::Node const& node)
{
    if (m_digraph.degree(node) == 0u)
    {
        m_cache_urls += m_bookmarks[node].title;
    }
    else
    {
        m_cache_urls += m_folders[node].title;
    }
}

#if 0
// -----------------------------------------------------------------------------
std::string const& IslandedBrowser::getURL(DiGraph::Node const& node)
{
    if (m_digraph.degree(node) == 0u)
    {
        m_cache_urls = m_bookmarks[node].uri;
    }
    else
    {
        for (const auto& it: m_digraph.neighbors(node))
        {
             m_cache_urls += " ";
             m_cache_urls += m_bookmarks[node].uri;
        }
    }
    return m_cache_urls;
}
#endif

// -----------------------------------------------------------------------------
void IslandedBrowser::forceDirectedGraph()
{
    m_force_directed.update();
}
