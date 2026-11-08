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

#include "NodeStyle.hpp"
#include "Settings.hpp"

//------------------------------------------------------------------------------
float nodeRadius(BookmarkNode const& node)
{
    if (node.isBookmark())
        return BOOKMARK_RADIUS;
    return (node.depth <= 1u) ? ISLAND_RADIUS : FOLDER_RADIUS;
}

//------------------------------------------------------------------------------
sf::String nodeKind(Bookmarks const& bookmarks, size_t node)
{
    BookmarkNode const& n = bookmarks.nodes[node];
    if (n.isBookmark())
        return "BOOKMARK";
    if (n.depth <= 1u)
        return bookmarks.links.empty() ? "FOLDER" : "ISLAND";
    if (n.depth == 2u && !bookmarks.links.empty())
        return "COMMUNITY";
    return "FOLDER";
}

//------------------------------------------------------------------------------
sf::String nodeKindName(Bookmarks const& bookmarks, size_t node)
{
    // Keep the first letter, lower the others.
    sf::String kind = nodeKind(bookmarks, node);
    for (size_t i = 1u; i < kind.getSize(); ++i)
    {
        if (kind[i] >= 'A' && kind[i] <= 'Z')
            kind[i] += 'a' - 'A';
    }
    return kind;
}
