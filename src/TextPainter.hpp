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

#ifndef TEXT_PAINTER_HPP
#  define TEXT_PAINTER_HPP

#  include <SFML/Graphics.hpp>
#  include "Settings.hpp"
#  include <string>
#  include <vector>

//! \brief Bookmark titles are stored as UTF-8 bytes; SFML wants its own
//! string type to show accents and symbols correctly. These two functions
//! translate from one to the other.
sf::String utf8(std::string const& s);
std::string toUtf8(sf::String const& s);

// ****************************************************************************
//! \brief The "pen" of the interface: it writes texts and paints the
//! rectangles used as background by the panels, the tooltip, the help...
//!
//! It also knows how much room a text takes, which lets the panels cut long
//! titles into several lines instead of letting them overflow.
// ****************************************************************************
class TextPainter
{
public:

    //-------------------------------------------------------------------------
    //! \param[in] target where the texts are drawn (the window).
    //! \param[in] font_path TrueType font file. When it is missing, the
    //! program still works but no text is shown.
    //-------------------------------------------------------------------------
    TextPainter(sf::RenderTarget& target, std::string const& font_path);

    //! \brief False when the font file could not be read: nothing to write.
    bool hasFont() const { return m_has_font; }
    sf::Font const& font() const { return m_font; }
    sf::RenderTarget& target() { return m_target; }
    sf::Vector2f windowSize() const { return sf::Vector2f(m_target.getSize()); }

    //-------------------------------------------------------------------------
    //! \brief Write a text, its top left corner at the given position (in
    //! pixels of the window).
    //! \return how many pixels wide the written text is, so the caller can
    //! write something right after it.
    //-------------------------------------------------------------------------
    float write(sf::String const& string, sf::Vector2f position, unsigned size,
                sf::Color color, bool bold = false);

    //-------------------------------------------------------------------------
    //! \brief Measure a text without drawing it (pixels).
    //-------------------------------------------------------------------------
    float width(sf::String const& string, unsigned size, bool bold = false) const;

    //-------------------------------------------------------------------------
    //! \brief Cut a long text into lines that fit in the given width, like a
    //! word processor does. At most max_lines lines are returned; if the text
    //! is still too long, the last line ends with "…".
    //-------------------------------------------------------------------------
    std::vector<sf::String> wrap(sf::String const& string, float width,
                                 unsigned size, size_t max_lines) const;

    //-------------------------------------------------------------------------
    //! \brief Paint a filled rectangle with a thin border. A transparent
    //! border color means "no border".
    //-------------------------------------------------------------------------
    void box(sf::FloatRect const& rect, sf::Color fill = PANEL_COLOR,
             sf::Color border = PANEL_BORDER_COLOR);

    //-------------------------------------------------------------------------
    //! \brief Paint a plain rectangle without border: the colored stripes on
    //! the side of the panels, the blinking caret of the search box.
    //-------------------------------------------------------------------------
    void bar(sf::FloatRect const& rect, sf::Color color);

    //-------------------------------------------------------------------------
    //! \brief Vertical distance between two lines of text of this size.
    //-------------------------------------------------------------------------
    static float lineHeight(unsigned size);

private:

    sf::RenderTarget& m_target;
    sf::Font m_font;
    //! \brief Reused for every text, to avoid creating one per frame.
    sf::Text m_text;
    bool m_has_font = false;
};

#endif
