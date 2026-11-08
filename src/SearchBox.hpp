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

#ifndef SEARCH_BOX_HPP
#  define SEARCH_BOX_HPP

#  include "IslandedBrowser.hpp"
#  include "TextPainter.hpp"
#  include <vector>

// ****************************************************************************
//! \brief The search field at the top of the window (keys / or Ctrl+F).
//!
//! Each typed letter restarts the search among the titles and the URLs.
//! The results are then visited one by one with Enter or the arrows; the
//! visit loops back to the first result after the last one.
// ****************************************************************************
class SearchBox
{
public:

    SearchBox(TextPainter& painter, IslandedBrowser const& island);

    void open();
    //! \brief Close the field and forget the text and the results.
    void close();
    bool isOpen() const { return m_open; }

    //-------------------------------------------------------------------------
    //! \brief A character was typed: add it to the text (or remove the last
    //! one for the Backspace key) and search again.
    //-------------------------------------------------------------------------
    void type(sf::Uint32 unicode);

    //-------------------------------------------------------------------------
    //! \brief Go to the next result (direction > 0) or to the previous one.
    //! \return the node of that result, or BookmarkNode::none when nothing
    //! was found.
    //-------------------------------------------------------------------------
    size_t next(int direction);

    //! \brief All the nodes found (empty when the field is closed).
    std::vector<size_t> const& matches() const { return m_matches; }

    //! \brief True when the pixel is on the field.
    bool contains(sf::Vector2i pixel) const;

    void draw();

private:

    void search();

private:

    TextPainter& m_painter;
    IslandedBrowser const& m_island;
    bool m_open = false;
    //! \brief Text typed by the user.
    sf::String m_query;
    std::vector<size_t> m_matches;
    //! \brief Rank of the result currently shown; equal to the number of
    //! results when none has been visited yet.
    size_t m_current = 0u;
    //! \brief Where the field was drawn.
    sf::FloatRect m_box;
    //! \brief Makes the text cursor blink.
    sf::Clock m_caret_clock;
};

#endif
