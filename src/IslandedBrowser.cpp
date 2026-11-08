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

#include "IslandedBrowser.hpp"
#include <spawn.h>
#include <sys/types.h>

extern char** environ;

// -----------------------------------------------------------------------------
IslandedBrowser::IslandedBrowser(std::string const& path)
    : m_bookmarks(Bookmarks::load(path))
{
    m_layout.reset(m_bookmarks);
}

// -----------------------------------------------------------------------------
size_t IslandedBrowser::pick(sf::Vector2f const& position, float radius) const
{
    size_t best = BookmarkNode::none;
    float best_distance = radius * radius;
    for (ForceDirectedGraph::Vertex const& v: m_layout.vertices())
    {
        sf::Vector2f const delta = v.position - position;
        float const distance = delta.x * delta.x + delta.y * delta.y;
        if (distance <= best_distance)
        {
            best_distance = distance;
            best = v.node;
        }
    }
    return best;
}

// -----------------------------------------------------------------------------
std::vector<std::string> IslandedBrowser::urls(size_t node, size_t max_count,
                                               size_t& total) const
{
    std::vector<std::string> found;
    total = 0u;
    if (node >= m_bookmarks.nodes.size())
        return found;

    // Breadth first: the bookmarks of the folder itself come first.
    std::vector<size_t> queue{ node };
    for (size_t i = 0u; i < queue.size(); ++i)
    {
        BookmarkNode const& current = m_bookmarks.nodes[queue[i]];
        if (current.isBookmark())
        {
            ++total;
            if (found.size() < max_count)
                found.push_back(current.url);
        }
        queue.insert(queue.end(), current.children.begin(), current.children.end());
    }
    return found;
}

// -----------------------------------------------------------------------------
bool IslandedBrowser::openInBrowser(std::string const& browser,
                                    std::vector<std::string> const& urls)
{
    if (urls.empty())
        return true;
    std::vector<char*> argv;
    argv.push_back(const_cast<char*>(browser.c_str()));
    for (std::string const& url: urls)
        argv.push_back(const_cast<char*>(url.c_str()));
    argv.push_back(nullptr);

    // The child is reaped by the SIGCHLD handler set in main().
    pid_t pid;
    return posix_spawnp(&pid, browser.c_str(), nullptr, nullptr,
                        argv.data(), environ) == 0;
}
