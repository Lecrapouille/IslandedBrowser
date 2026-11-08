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

#include "Selection.hpp"
#include "Settings.hpp"
#include <algorithm>

//------------------------------------------------------------------------------
Selection::Selection(IslandedBrowser const& island)
    : m_island(island)
{
    m_bright.assign(island.bookmarks().nodes.size(), 0);
}

//------------------------------------------------------------------------------
void Selection::select(size_t node)
{
    m_selected = node;
    m_related.clear();
    if (node != BookmarkNode::none && m_island.bookmarks().nodes[node].isBookmark())
        m_related = m_island.related(node, PANEL_MAX_ROWS / 2u);
}

//------------------------------------------------------------------------------
void Selection::hover(size_t on_map, size_t in_panel)
{
    m_hovered = on_map;
    m_hovered_row = in_panel;
}

//------------------------------------------------------------------------------
void Selection::brighten(size_t node)
{
    for (size_t n: m_island.subtree(node))
        m_bright[n] = 1;
    for (size_t n: m_island.ancestors(node))
        m_bright[n] = 1;
    m_has_focus = true;
}

//------------------------------------------------------------------------------
void Selection::updateFocus(std::vector<size_t> const& matches)
{
    std::fill(m_bright.begin(), m_bright.end(), 0);
    m_has_focus = false;

    size_t const pointed = (m_hovered != BookmarkNode::none) ? m_hovered : m_hovered_row;
    if (pointed != BookmarkNode::none)
    {
        brighten(m_island.community(pointed));
    }
    else if (!matches.empty())
    {
        for (size_t node: matches)
            brighten(node);
    }
    else if (m_selected != BookmarkNode::none)
    {
        brighten(m_island.community(m_selected));
    }
}
