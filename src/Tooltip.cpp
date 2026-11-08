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

#include "Tooltip.hpp"
#include "NodeStyle.hpp"
#include <algorithm>
#include <sstream>

namespace {

//! \brief Widest text in the bubble, and room around the texts (pixels).
float const WIDTH = 380.0f;
float const PADDING = 10.0f;

} // namespace

//------------------------------------------------------------------------------
Tooltip::Tooltip(TextPainter& painter, IslandedBrowser const& island, Palette const& palette)
    : m_painter(painter), m_island(island), m_palette(palette)
{}

//------------------------------------------------------------------------------
void Tooltip::add(sf::String const& text, unsigned size, sf::Color color, bool bold)
{
    m_lines.push_back({ text, size, color, bold });
}

//------------------------------------------------------------------------------
void Tooltip::describe(size_t node, bool selected)
{
    Bookmarks const& bookmarks = m_island.bookmarks();
    BookmarkNode const& n = bookmarks.nodes[node];
    m_lines.clear();

    for (sf::String const& l: m_painter.wrap(utf8(n.title), WIDTH, 17u, 2u))
        add(l, 17u, TEXT_COLOR, true);

    // A bookmark shows its address, a folder what it holds.
    if (n.isBookmark())
    {
        for (sf::String const& l: m_painter.wrap(utf8(n.url), WIDTH, 14u, 2u))
            add(l, 14u, URL_COLOR);
    }
    else
    {
        std::ostringstream info;
        size_t const count = m_island.bookmarkCount(node);
        info << count << (count > 1u ? " bookmarks" : " bookmark");
        add(nodeKindName(bookmarks, node) + sf::String(L" · ") + utf8(info.str()),
            14u, m_palette.ofNode(node));
    }

    // Where it is: "Island › Community".
    std::vector<size_t> const path = m_island.ancestors(node);
    if (!path.empty())
    {
        std::string joined;
        for (size_t p: path)
            joined += (joined.empty() ? "" : "  ›  ") + bookmarks.nodes[p].title;
        for (sf::String const& l: m_painter.wrap(utf8(joined), WIDTH, 14u, 1u))
            add(l, 14u, DIM_TEXT_COLOR);
    }

    if (selected)
        add(n.isBookmark() ? "Double-click or Enter: open"
                           : "Double-click: zoom    Enter: open its bookmarks",
            13u, DIM_TEXT_COLOR);
    else
        add(n.isBookmark() ? "Click: details    Double-click: open"
                           : "Click: details    Double-click: zoom    Middle-click: open",
            13u, DIM_TEXT_COLOR);
}

//------------------------------------------------------------------------------
void Tooltip::draw(size_t node, bool selected, sf::Vector2i mouse)
{
    if (node == BookmarkNode::none || !m_painter.hasFont())
        return;
    describe(node, selected);

    // Size of the bubble: the sum of the lines, the widest line.
    float height = 2.0f * PADDING;
    float content = 0.0f;
    for (Line const& l: m_lines)
    {
        height += TextPainter::lineHeight(l.size);
        content = std::max(content, m_painter.width(l.text, l.size, l.bold));
    }
    float const w = std::min(WIDTH, content) + 2.0f * PADDING;

    // Next to the mouse, on the other side when there is no room.
    sf::Vector2f const window = m_painter.windowSize();
    sf::Vector2f p(float(mouse.x) + 18.0f, float(mouse.y) + 20.0f);
    if (p.x + w > window.x - 4.0f)
        p.x = float(mouse.x) - w - 12.0f;
    if (p.y + height > window.y - STATUSBAR_HEIGHT - 4.0f)
        p.y = float(mouse.y) - height - 12.0f;
    p.x = std::max(4.0f, p.x);
    p.y = std::max(4.0f, p.y);

    m_painter.box(sf::FloatRect(p.x, p.y, w, height));
    m_painter.bar(sf::FloatRect(p.x, p.y, 3.0f, height), m_palette.ofNode(node));
    float y = p.y + PADDING - 2.0f;
    for (Line const& l: m_lines)
    {
        m_painter.write(l.text, sf::Vector2f(p.x + PADDING, y), l.size, l.color, l.bold);
        y += TextPainter::lineHeight(l.size);
    }
}
