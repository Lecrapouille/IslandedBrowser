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

#ifndef MOUSE_TRACKER_HPP
#  define MOUSE_TRACKER_HPP

#  include <SFML/Window.hpp>
#  include <cstddef>

// ****************************************************************************
//! \brief Turns the raw mouse events (button down, button up, move) into
//! what the user means: a click, a double click or a drag of the map.
//!
//! - A press followed by a release without moving is a click. Moving more
//!   than a few pixels with a button held turns it into a drag instead, so a
//!   slightly shaky click still counts as a click.
//! - Two clicks on the same node close enough in time make a double click.
// ****************************************************************************
class MouseTracker
{
public:

    //-------------------------------------------------------------------------
    //! \brief The left or right button went down at this pixel.
    //-------------------------------------------------------------------------
    void press(sf::Mouse::Button button, sf::Vector2i pixel);

    //-------------------------------------------------------------------------
    //! \brief A button went up.
    //! \return true when it was the left button and the mouse did not drag
    //! the map meanwhile: this was a click.
    //-------------------------------------------------------------------------
    bool release(sf::Mouse::Button button);

    //-------------------------------------------------------------------------
    //! \brief The mouse moved to "now".
    //! \param[in] allowed false when the press happened where dragging makes
    //! no sense (on a panel, on the help).
    //! \param[out] previous where the mouse was at the previous move.
    //! \return true when the map must follow the mouse (from previous to now).
    //-------------------------------------------------------------------------
    bool drag(sf::Vector2i now, bool allowed, sf::Vector2i& previous);

    //-------------------------------------------------------------------------
    //! \brief The window lost the keyboard and mouse: forget the buttons
    //! held, their release will never come.
    //-------------------------------------------------------------------------
    void cancel();

    //-------------------------------------------------------------------------
    //! \brief Record a click on a node (BookmarkNode::none for the sea).
    //! \return true when it completes a double click on that node.
    //-------------------------------------------------------------------------
    bool doubleClick(size_t node);

    bool dragging() const { return m_dragging; }
    //! \brief Pixel where the last button was pressed.
    sf::Vector2i pressedAt() const { return m_press; }

    //! \brief Last known position of the mouse in the window.
    void moveTo(sf::Vector2i pixel) { m_position = pixel; }
    sf::Vector2i position() const { return m_position; }

private:

    sf::Vector2i m_position;
    sf::Vector2i m_press;
    sf::Vector2i m_last;
    bool m_left = false;
    bool m_right = false;
    bool m_dragging = false;
    //! \brief Time since the last click, and the node it was on.
    sf::Clock m_click_clock;
    size_t m_last_click = static_cast<size_t>(-1);
};

#endif
