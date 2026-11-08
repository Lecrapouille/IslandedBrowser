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
#include <algorithm>
#include <cctype>
#include <csignal>
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
size_t IslandedBrowser::bookmarkCount(size_t node) const
{
    size_t total = 0u;
    urls(node, 0u, total);
    return total;
}

// -----------------------------------------------------------------------------
std::vector<size_t> IslandedBrowser::subtree(size_t node) const
{
    std::vector<size_t> nodes;
    if (node >= m_bookmarks.nodes.size())
        return nodes;
    nodes.push_back(node);
    for (size_t i = 0u; i < nodes.size(); ++i)
    {
        std::vector<size_t> const& children = m_bookmarks.nodes[nodes[i]].children;
        nodes.insert(nodes.end(), children.begin(), children.end());
    }
    return nodes;
}

// -----------------------------------------------------------------------------
std::vector<size_t> IslandedBrowser::ancestors(size_t node) const
{
    std::vector<size_t> path;
    if (node >= m_bookmarks.nodes.size())
        return path;
    for (size_t p = m_bookmarks.nodes[node].parent;
         p != BookmarkNode::none && m_bookmarks.nodes[p].parent != BookmarkNode::none;
         p = m_bookmarks.nodes[p].parent)
    {
        path.push_back(p);
    }
    std::reverse(path.begin(), path.end());
    return path;
}

// -----------------------------------------------------------------------------
std::vector<size_t> IslandedBrowser::search(std::string const& text) const
{
    auto lower = [](std::string s) {
        std::transform(s.begin(), s.end(), s.begin(), [](unsigned char c) {
            return char(std::tolower(c));
        });
        return s;
    };
    std::vector<size_t> folders, bookmarks;
    std::string const needle = lower(text);
    if (needle.empty())
        return folders;
    // Node 0 is the root: it is never displayed.
    for (size_t i = 1u; i < m_bookmarks.nodes.size(); ++i)
    {
        BookmarkNode const& node = m_bookmarks.nodes[i];
        if (lower(node.title).find(needle) == std::string::npos &&
            lower(node.url).find(needle) == std::string::npos)
            continue;
        (node.isBookmark() ? bookmarks : folders).push_back(i);
    }
    folders.insert(folders.end(), bookmarks.begin(), bookmarks.end());
    return folders;
}

// -----------------------------------------------------------------------------
std::vector<std::pair<size_t, float>> IslandedBrowser::related(size_t node,
                                                                size_t max_count) const
{
    std::vector<std::pair<size_t, float>> found;
    for (BookmarkLink const& link: m_bookmarks.links)
    {
        if (link.from == node)
            found.emplace_back(link.to, link.weight);
        else if (link.to == node)
            found.emplace_back(link.from, link.weight);
    }
    std::sort(found.begin(), found.end(), [](std::pair<size_t, float> const& a,
                                             std::pair<size_t, float> const& b) {
        return a.second > b.second;
    });
    if (found.size() > max_count)
        found.resize(max_count);
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

    // The child is reaped by the kernel: main() ignores SIGCHLD. The browser
    // must not inherit this disposition, else its own waitpid() calls fail
    // (Firefox aborts when it cannot wait for its sandboxed processes).
    posix_spawnattr_t attributes;
    posix_spawnattr_init(&attributes);
    sigset_t signals;
    sigemptyset(&signals);
    sigaddset(&signals, SIGCHLD);
    posix_spawnattr_setsigdefault(&attributes, &signals);
    posix_spawnattr_setflags(&attributes, POSIX_SPAWN_SETSIGDEF);
    pid_t pid;
    bool const started = posix_spawnp(&pid, browser.c_str(), nullptr, &attributes,
                                      argv.data(), environ) == 0;
    posix_spawnattr_destroy(&attributes);
    return started;
}
