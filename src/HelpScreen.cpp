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

#include "HelpScreen.hpp"
#include <algorithm>
#include <cmath>
#include <sstream>
#include <utility>
#include <vector>

//------------------------------------------------------------------------------
HelpScreen::HelpScreen(TextPainter& painter)
    : m_painter(painter)
{}

//------------------------------------------------------------------------------
void HelpScreen::draw()
{
    if (!m_visible || !m_painter.hasFont())
        return;
    sf::Vector2f const window = m_painter.windowSize();
    // Darken the whole map behind the help.
    m_painter.box(sf::FloatRect(0.0f, 0.0f, window.x, window.y), sf::Color(0, 0, 0, 120),
                  sf::Color::Transparent);

    std::ostringstream tabs;
    tabs << "open in " << BROWSER_NAME << " (a folder: its " << MAX_TABS << " first bookmarks)";
    // Each entry: what the user does, and what happens.
    struct Entry { sf::String key; sf::String action; };
    std::vector<std::pair<sf::String, std::vector<Entry>>> const sections = {
        { "Mouse", {
            { "Hover", "title, URL; its community is highlighted" },
            { "Left click", "select: details in the right panel" },
            { "Double click", "open a bookmark, zoom on a folder" },
            { "Middle or Ctrl + click", utf8(tabs.str()) },
            { "Left or right drag", "move the map" },
            { "Wheel", "zoom around the cursor" },
            { "Click on the sea", "unselect" } } },
        { "Keyboard", {
            { "/  or  Ctrl + F", "search the titles and URLs" },
            { "Enter", "open the selection" },
            { "Z  /  F", "zoom on the selection  /  fit the whole map" },
            { "Arrows,  +  -", "move, zoom" },
            { "Space  /  R", "pause  /  restart the layout" },
            { "L  /  N", "show the links and sea routes  /  the names" },
            { "T", "terrain: relief, Voronoi cells, rivers" },
            { "S", "screenshot in islanded-browser.png" },
            { "Esc  /  Q", "close, unselect, then quit  /  quit" } } },
        { "Map", {
            { "Big disc", "island (a subject)" },
            { "Medium disc", "community or folder" },
            { "Small disc", "bookmark" },
            { "Blue bands", "sea routes: islands with similar bookmarks" } } }
    };

    // Height of the page: title, sections (header, entries, gap), footer.
    float const column = 200.0f;
    float const width = std::min(window.x - 24.0f, 760.0f);
    float height = 18.0f + 40.0f + 36.0f;
    for (auto const& section: sections)
        height += 30.0f + TextPainter::lineHeight(16u) * float(section.second.size()) + 4.0f;
    sf::Vector2f const p(std::round((window.x - width) / 2.0f),
                         std::round(std::max(12.0f, (window.y - height) / 2.0f)));
    m_painter.box(sf::FloatRect(p.x, p.y, width, height), PANEL_COLOR, URL_COLOR);

    float y = p.y + 18.0f;
    m_painter.write("Islanded Browser", sf::Vector2f(p.x + 24.0f, y), 24u, TEXT_COLOR, true);
    y += 40.0f;
    for (auto const& section: sections)
    {
        m_painter.write(section.first, sf::Vector2f(p.x + 24.0f, y), 17u, URL_COLOR, true);
        y += 30.0f;
        for (Entry const& e: section.second)
        {
            m_painter.write(e.key, sf::Vector2f(p.x + 36.0f, y), 16u, SELECTION_COLOR);
            m_painter.write(e.action, sf::Vector2f(p.x + 36.0f + column, y), 16u, TEXT_COLOR);
            y += TextPainter::lineHeight(16u);
        }
        y += 4.0f;
    }
    m_painter.write("Press H, Esc or click to close",
                    sf::Vector2f(p.x + 24.0f, p.y + height - 30.0f), 14u, DIM_TEXT_COLOR);
}
