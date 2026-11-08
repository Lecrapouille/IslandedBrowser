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

#include "SearchBox.hpp"
#include <algorithm>
#include <cmath>
#include <sstream>

//------------------------------------------------------------------------------
SearchBox::SearchBox(TextPainter& painter, IslandedBrowser const& island)
    : m_painter(painter), m_island(island)
{}

//------------------------------------------------------------------------------
void SearchBox::open()
{
    m_open = true;
    m_caret_clock.restart();
    search();
}

//------------------------------------------------------------------------------
void SearchBox::close()
{
    m_open = false;
    m_query.clear();
    m_matches.clear();
}

//------------------------------------------------------------------------------
bool SearchBox::contains(sf::Vector2i pixel) const
{
    return m_open && m_box.contains(sf::Vector2f(pixel));
}

//------------------------------------------------------------------------------
void SearchBox::search()
{
    m_matches = m_island.search(toUtf8(m_query));
    m_current = m_matches.size();
}

//------------------------------------------------------------------------------
void SearchBox::type(sf::Uint32 unicode)
{
    if (unicode == 8u)
    {
        // Backspace.
        if (!m_query.isEmpty())
            m_query.erase(m_query.getSize() - 1u);
    }
    else if (unicode >= 32u && unicode != 127u)
    {
        m_query += unicode;
    }
    else
    {
        // Other control characters (Ctrl + letter, Escape...) are not text.
        return;
    }
    search();
    m_caret_clock.restart();
}

//------------------------------------------------------------------------------
size_t SearchBox::next(int direction)
{
    size_t const count = m_matches.size();
    if (count == 0u)
        return BookmarkNode::none;
    if (m_current >= count)
        m_current = (direction > 0) ? 0u : count - 1u;
    else
        m_current = (m_current + (direction > 0 ? 1u : count - 1u)) % count;
    return m_matches[m_current];
}

//------------------------------------------------------------------------------
void SearchBox::draw()
{
    m_box = sf::FloatRect();
    if (!m_open || !m_painter.hasFont())
        return;
    sf::Vector2f const window = m_painter.windowSize();
    float const width = std::min(520.0f, window.x - 24.0f);
    m_box = sf::FloatRect(std::round((window.x - width) / 2.0f), 12.0f, width, 40.0f);
    m_painter.box(m_box, PANEL_COLOR, URL_COLOR);

    float const x = m_box.left + 12.0f;
    float const y = m_box.top + 9.0f;
    float const w = m_painter.write("Search:", sf::Vector2f(x, y), 17u, DIM_TEXT_COLOR);

    // A long text loses its beginning: the end, being typed, stays visible.
    sf::String shown = m_query;
    float const room = width - w - 150.0f;
    while (shown.getSize() > 1u && m_painter.width(shown, 17u) > room)
        shown.erase(0u);
    float const q = m_painter.write(shown, sf::Vector2f(x + w + 8.0f, y), 17u, TEXT_COLOR);

    // Text cursor, visible half a second out of every second.
    if (int(m_caret_clock.getElapsedTime().asSeconds() * 2.0f) % 2 == 0)
        m_painter.bar(sf::FloatRect(std::round(x + w + 10.0f + q), y + 1.0f, 2.0f, 20.0f),
                      TEXT_COLOR);

    // On the right: how many results, and which one is shown.
    std::ostringstream count;
    if (m_query.isEmpty())
        count << "titles and URLs";
    else if (m_matches.empty())
        count << "no match";
    else if (m_current < m_matches.size())
        count << (m_current + 1u) << " / " << m_matches.size();
    else
        count << m_matches.size() << (m_matches.size() > 1u ? " matches" : " match");
    sf::String const c = utf8(count.str());
    sf::Color const color = (m_matches.empty() && !m_query.isEmpty())
                            ? sf::Color(255, 140, 120) : DIM_TEXT_COLOR;
    m_painter.write(c, sf::Vector2f(m_box.left + width - 12.0f - m_painter.width(c, 15u),
                                    y + 2.0f), 15u, color);
}
