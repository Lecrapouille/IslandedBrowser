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

#include "DetailsPanel.hpp"
#include "NodeStyle.hpp"
#include <algorithm>
#include <cmath>
#include <sstream>

namespace {

//! \brief Room around the texts, and width left for the texts (pixels).
float const PADDING = 14.0f;
float const INNER = PANEL_WIDTH - 2.0f * PADDING;

} // namespace

//------------------------------------------------------------------------------
DetailsPanel::DetailsPanel(TextPainter& painter, IslandedBrowser const& island,
                           Palette const& palette)
    : m_painter(painter), m_island(island), m_palette(palette)
{}

//------------------------------------------------------------------------------
bool DetailsPanel::contains(sf::Vector2i pixel) const
{
    return m_box.contains(sf::Vector2f(pixel));
}

//------------------------------------------------------------------------------
size_t DetailsPanel::rowAt(sf::Vector2i pixel) const
{
    for (Row const& row: m_rows)
    {
        if (row.box.contains(sf::Vector2f(pixel)))
            return row.node;
    }
    return BookmarkNode::none;
}

//------------------------------------------------------------------------------
void DetailsPanel::add(sf::String const& text, unsigned size, sf::Color color, bool bold,
                       size_t target, float before, float indent)
{
    m_lines.push_back({ text, size, color, bold, target, before, indent });
}

//------------------------------------------------------------------------------
void DetailsPanel::draw(Selection const& selection, sf::Vector2i mouse)
{
    m_rows.clear();
    m_lines.clear();
    m_box = sf::FloatRect();
    size_t const node = selection.selected();
    if (node == BookmarkNode::none || !m_painter.hasFont())
        return;

    m_accent = m_palette.ofNode(node);
    addHeader(node);
    if (m_island.bookmarks().nodes[node].isBookmark())
        addBookmark(node, selection);
    else
        addFolder(node);
    addHints(node);
    paint(mouse);
}

//------------------------------------------------------------------------------
void DetailsPanel::addHeader(size_t node)
{
    Bookmarks const& bookmarks = m_island.bookmarks();
    add(nodeKind(bookmarks, node), 13u, m_accent, true);
    for (sf::String const& l: m_painter.wrap(utf8(bookmarks.nodes[node].title), INNER, 20u, 3u))
        add(l, 20u, TEXT_COLOR, true);

    // The folders leading to the node, each one a bit more to the right,
    // like a staircase.
    std::vector<size_t> const path = m_island.ancestors(node);
    float indent = 0.0f;
    for (size_t i = 0u; i < path.size(); ++i)
    {
        sf::String const t = sf::String(L"› ") + utf8(bookmarks.nodes[path[i]].title);
        add(m_painter.wrap(t, INNER - indent, 14u, 1u).front(), 14u, DIM_TEXT_COLOR, false,
            path[i], i == 0u ? 4.0f : 0.0f, indent);
        indent += 12.0f;
    }
}

//------------------------------------------------------------------------------
void DetailsPanel::addBookmark(size_t node, Selection const& selection)
{
    Bookmarks const& bookmarks = m_island.bookmarks();
    bool first = true;
    for (sf::String const& l: m_painter.wrap(utf8(bookmarks.nodes[node].url), INNER, 14u, 4u))
    {
        add(l, 14u, URL_COLOR, false, BookmarkNode::none, first ? 8.0f : 0.0f);
        first = false;
    }
    if (selection.related().empty())
        return;

    add("Similar bookmarks", 14u, TEXT_COLOR, true, BookmarkNode::none, 12.0f);
    for (auto const& r: selection.related())
    {
        // The title is shortened so the percentage always fits after it.
        std::ostringstream score;
        score << "  " << int(std::round(r.second * 100.0f)) << "%";
        sf::String const s = utf8(score.str());
        float const room = INNER - m_painter.width(s, 14u) - 12.0f;
        sf::String const t = sf::String(L"• ") + utf8(bookmarks.nodes[r.first].title);
        add(m_painter.wrap(t, room, 14u, 1u).front() + s, 14u, TEXT_COLOR, false, r.first);
    }
}

//------------------------------------------------------------------------------
void DetailsPanel::addFolder(size_t node)
{
    Bookmarks const& bookmarks = m_island.bookmarks();
    BookmarkNode const& n = bookmarks.nodes[node];
    size_t folders = 0u;
    for (size_t child: n.children)
        folders += bookmarks.nodes[child].isBookmark() ? 0u : 1u;
    std::ostringstream info;
    size_t const count = m_island.bookmarkCount(node);
    info << count << (count > 1u ? " bookmarks" : " bookmark");
    if (folders > 0u)
        info << "  ·  " << folders << (folders > 1u ? " sub-folders" : " sub-folder");
    add(utf8(info.str()), 14u, TEXT_COLOR, false, BookmarkNode::none, 8.0f);

    // Sub-folders first, then the bookmarks; each group keeps its order.
    std::vector<size_t> children = n.children;
    std::stable_sort(children.begin(), children.end(), [&](size_t a, size_t b) {
        return !bookmarks.nodes[a].isBookmark() && bookmarks.nodes[b].isBookmark();
    });
    if (!children.empty())
        add("Content", 14u, TEXT_COLOR, true, BookmarkNode::none, 12.0f);
    size_t shown = 0u;
    for (size_t child: children)
    {
        if (shown++ >= PANEL_MAX_ROWS)
            break;
        BookmarkNode const& c = bookmarks.nodes[child];
        sf::String t;
        if (c.isBookmark())
        {
            t = sf::String(L"• ") + utf8(c.title);
        }
        else
        {
            // A sub-folder shows how many bookmarks it holds: "(12)".
            std::ostringstream size;
            size << "  (" << m_island.bookmarkCount(child) << ")";
            sf::String const s = utf8(size.str());
            float const room = INNER - m_painter.width(s, 14u) - 12.0f;
            t = m_painter.wrap(sf::String(L"› ") + utf8(c.title), room, 14u, 1u).front() + s;
        }
        add(m_painter.wrap(t, INNER, 14u, 1u).front(), 14u,
            c.isBookmark() ? TEXT_COLOR : blend(m_accent, sf::Color::White, 0.3f),
            false, child);
    }
    if (children.size() > PANEL_MAX_ROWS)
    {
        std::ostringstream more;
        more << "… and " << (children.size() - PANEL_MAX_ROWS) << " more";
        add(utf8(more.str()), 14u, DIM_TEXT_COLOR);
    }
}

//------------------------------------------------------------------------------
void DetailsPanel::addHints(size_t node)
{
    std::ostringstream hint;
    if (m_island.bookmarks().nodes[node].isBookmark())
    {
        hint << "Enter or double-click: open in " << BROWSER_NAME;
    }
    else
    {
        size_t const count = m_island.bookmarkCount(node);
        hint << "Enter: open " << std::min<size_t>(count, MAX_TABS)
             << (count > MAX_TABS ? " first" : "") << " bookmarks in " << BROWSER_NAME;
    }
    add(utf8(hint.str()), 13u, DIM_TEXT_COLOR, false, BookmarkNode::none, 14.0f);
    add("Z: zoom on it    Click an entry to go to it    Esc: close", 13u, DIM_TEXT_COLOR);
}

//------------------------------------------------------------------------------
void DetailsPanel::paint(sf::Vector2i mouse)
{
    // Keep only the lines that fit above the status bar.
    sf::Vector2f const window = m_painter.windowSize();
    float const top = 12.0f;
    float const bottom = window.y - STATUSBAR_HEIGHT - 12.0f;
    float height = 2.0f * PADDING;
    size_t count = 0u;
    for (Line const& l: m_lines)
    {
        float const h = l.before + TextPainter::lineHeight(l.size);
        if (top + height + h > bottom)
            break;
        height += h;
        ++count;
    }

    float const left = window.x - PANEL_WIDTH - 12.0f;
    m_box = sf::FloatRect(left, top, PANEL_WIDTH, height);
    m_painter.box(m_box);
    m_painter.bar(sf::FloatRect(left, top, PANEL_WIDTH, 3.0f), m_accent);

    float y = top + PADDING;
    for (size_t i = 0u; i < count; ++i)
    {
        Line const& l = m_lines[i];
        y += l.before;
        float const h = TextPainter::lineHeight(l.size);
        if (l.node != BookmarkNode::none)
        {
            sf::FloatRect const row(left + 4.0f, y - 1.0f, PANEL_WIDTH - 8.0f, h);
            m_rows.push_back({ row, l.node });
            if (row.contains(sf::Vector2f(mouse)))
                m_painter.box(row, sf::Color(255, 255, 255, 28), sf::Color::Transparent);
        }
        m_painter.write(l.text, sf::Vector2f(left + PADDING + l.indent, y), l.size,
                        l.color, l.bold);
        y += h;
    }
}
