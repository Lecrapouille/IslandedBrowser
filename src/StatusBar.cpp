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

#include "StatusBar.hpp"
#include <sstream>

//------------------------------------------------------------------------------
StatusBar::StatusBar(TextPainter& painter, IslandedBrowser const& island,
                     std::string const& file_name)
    : m_painter(painter), m_island(island), m_file_name(file_name)
{}

//------------------------------------------------------------------------------
void StatusBar::draw(Mode mode, bool paused)
{
    if (!m_painter.hasFont())
        return;
    sf::Vector2f const window = m_painter.windowSize();
    float const top = window.y - STATUSBAR_HEIGHT;
    m_painter.box(sf::FloatRect(0.0f, top, window.x, STATUSBAR_HEIGHT), PANEL_COLOR,
                  sf::Color::Transparent);

    Bookmarks const& bookmarks = m_island.bookmarks();
    ForceDirectedGraph const& layout = m_island.layout();
    std::ostringstream left;
    left << m_file_name << "   ·   " << bookmarks.bookmark_count << " bookmarks   ·   "
         << bookmarks.islands.size() << (bookmarks.links.empty() ? " folders" : " islands")
         << "   ·   layout "
         << (!layout.isRunning() ? "stable" : paused ? "paused (Space)" : "settling…");
    float const y = top + 4.0f;
    m_painter.write(utf8(left.str()), sf::Vector2f(10.0f, y), SMALL_CHARACTER_SIZE,
                    DIM_TEXT_COLOR);

    sf::String hint;
    switch (mode)
    {
    case Mode::Help:
        hint = "H or Esc: close the help";
        break;
    case Mode::Searching:
        hint = L"Type to search    Enter / ↓: next    ↑: previous    Ctrl + Enter: open    Esc: close";
        break;
    case Mode::Selected:
        hint = "Enter: open    Z: zoom    Esc: unselect    /: search    H: help";
        break;
    case Mode::Browsing:
        hint = "Click: select    Double-click: open    Drag: move    Wheel: zoom    "
               "/: search    H: help";
        break;
    }
    // Aligned on the right edge of the window.
    m_painter.write(hint, sf::Vector2f(window.x - 10.0f -
                                       m_painter.width(hint, SMALL_CHARACTER_SIZE), y),
                    SMALL_CHARACTER_SIZE, TEXT_COLOR);
}
