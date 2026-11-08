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

#ifndef TOOLTIP_HPP
#  define TOOLTIP_HPP

#  include "Colors.hpp"
#  include "TextPainter.hpp"
#  include <vector>

// ****************************************************************************
//! \brief The small bubble that follows the mouse over a node: its title,
//! its URL (or how many bookmarks a folder holds), where it is, and what a
//! click would do.
//!
//! The bubble is placed below and to the right of the mouse; when it would
//! go out of the window, it jumps to the other side of the mouse.
// ****************************************************************************
class Tooltip
{
public:

    Tooltip(TextPainter& painter, IslandedBrowser const& island, Palette const& palette);

    //-------------------------------------------------------------------------
    //! \param[in] node node under the mouse.
    //! \param[in] selected true when this node is also the selected one (the
    //! hint at the bottom changes).
    //! \param[in] mouse position of the mouse in the window.
    //-------------------------------------------------------------------------
    void draw(size_t node, bool selected, sf::Vector2i mouse);

private:

    //! \brief Fill m_lines with what the bubble says about the node.
    void describe(size_t node, bool selected);
    void add(sf::String const& text, unsigned size, sf::Color color, bool bold = false);

private:

    //! \brief One line of the bubble.
    struct Line
    {
        sf::String text;
        unsigned size;
        sf::Color color;
        bool bold;
    };

    TextPainter& m_painter;
    IslandedBrowser const& m_island;
    Palette const& m_palette;
    std::vector<Line> m_lines;
};

#endif
