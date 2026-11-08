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

#include "Rivers.hpp"
#include <algorithm>
#include <map>

//------------------------------------------------------------------------------
Rivers::Rivers(IslandedBrowser const& island)
{
    Bookmarks const& bookmarks = island.bookmarks();
    ForceDirectedGraph const& layout = island.layout();

    // Add up the links crossing between each pair of islands. The pair is
    // stored smallest index first, so (A, B) and (B, A) are the same entry.
    std::map<std::pair<size_t, size_t>, float> sums;
    for (BookmarkLink const& link: bookmarks.links)
    {
        size_t a = bookmarks.nodes[link.from].island;
        size_t b = bookmarks.nodes[link.to].island;
        if (a == b)
            continue;
        if (a > b)
            std::swap(a, b);
        sums[std::make_pair(a, b)] += link.weight;
    }

    for (auto const& it: sums)
    {
        size_t const a = layout.vertexOf(it.first.first);
        size_t const b = layout.vertexOf(it.first.second);
        if (a != BookmarkNode::none && b != BookmarkNode::none)
            m_rivers.push_back({ a, b, it.second });
    }

    // Keep the strongest ones, then scale them so the strongest is 1.
    std::sort(m_rivers.begin(), m_rivers.end(), [](River const& x, River const& y) {
        return x.strength > y.strength;
    });
    m_rivers.resize(std::min(m_rivers.size(), 2u * bookmarks.islands.size()));
    if (!m_rivers.empty())
    {
        float const strongest = m_rivers.front().strength;
        for (River& river: m_rivers)
            river.strength /= strongest;
    }
}
