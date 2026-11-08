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

#ifndef DETAILS_PANEL_HPP
#  define DETAILS_PANEL_HPP

#  include "Colors.hpp"
#  include "Selection.hpp"
#  include "TextPainter.hpp"
#  include <vector>

// ****************************************************************************
//! \brief The panel on the right of the window describing the selected
//! node: what it is, where it is, its URL, and either the content of a
//! folder or the bookmarks similar to a bookmark.
//!
//! Some lines of the panel (the folders above the node, the content, the
//! similar bookmarks) can be clicked to jump to that node. The panel
//! remembers where it drew them, so a click can be matched to a line.
//! The panel is cut at the status bar when it is too long for the window.
// ****************************************************************************
class DetailsPanel
{
public:

    DetailsPanel(TextPainter& painter, IslandedBrowser const& island, Palette const& palette);

    //-------------------------------------------------------------------------
    //! \brief Draw the panel of the selected node (nothing when no node is
    //! selected). The clickable line under the mouse is highlighted.
    //-------------------------------------------------------------------------
    void draw(Selection const& selection, sf::Vector2i mouse);

    //-------------------------------------------------------------------------
    //! \brief Forget the clickable lines (the selection changed, they will
    //! be computed again by the next draw()).
    //-------------------------------------------------------------------------
    void clear() { m_rows.clear(); }

    //! \brief True when the pixel is inside the panel.
    bool contains(sf::Vector2i pixel) const;

    //-------------------------------------------------------------------------
    //! \brief Node of the clickable line under the pixel, or
    //! BookmarkNode::none.
    //-------------------------------------------------------------------------
    size_t rowAt(sf::Vector2i pixel) const;

private:

    //! \brief Kind and title of the node, then the folders leading to it.
    void addHeader(size_t node);
    //! \brief URL of a bookmark and its most similar bookmarks.
    void addBookmark(size_t node, Selection const& selection);
    //! \brief Size of a folder and its content, sub-folders first.
    void addFolder(size_t node);
    //! \brief What the keyboard can do with the node.
    void addHints(size_t node);
    //! \brief Place the lines in the panel and draw them.
    void paint(sf::Vector2i mouse);

    //-------------------------------------------------------------------------
    //! \param[in] target node reached by clicking the line, or
    //! BookmarkNode::none for a line that cannot be clicked.
    //! \param[in] before empty space above the line (pixels).
    //! \param[in] indent shift to the right (pixels).
    //-------------------------------------------------------------------------
    void add(sf::String const& text, unsigned size, sf::Color color, bool bold = false,
             size_t target = BookmarkNode::none, float before = 0.0f,
             float indent = 0.0f);

private:

    //! \brief One line of the panel, before it is placed.
    struct Line
    {
        sf::String text;
        unsigned size;
        sf::Color color;
        bool bold;
        size_t node;
        float before;
        float indent;
    };

    //! \brief Area of a clickable line on the screen and its node.
    struct Row
    {
        sf::FloatRect box;
        size_t node;
    };

    TextPainter& m_painter;
    IslandedBrowser const& m_island;
    Palette const& m_palette;
    std::vector<Line> m_lines;
    std::vector<Row> m_rows;
    //! \brief Where the panel was drawn (empty when hidden).
    sf::FloatRect m_box;
    //! \brief Color of the island of the selected node.
    sf::Color m_accent;
};

#endif
