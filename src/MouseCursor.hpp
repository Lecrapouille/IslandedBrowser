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

#ifndef MOUSE_CURSOR_HPP
#  define MOUSE_CURSOR_HPP

#  include <SFML/Graphics/RenderWindow.hpp>

// ****************************************************************************
//! \brief Changes the look of the mouse pointer to tell the user what a
//! click would do: an arrow on the sea, a hand on something clickable, a
//! four-way arrow while moving the map.
// ****************************************************************************
class MouseCursor
{
public:

    enum class Shape { Arrow, Hand, Move };

public:

    explicit MouseCursor(sf::RenderWindow& window);

    //! \brief Change the pointer (nothing happens if it already has this
    //! shape, or if the system has no such pointer).
    void show(Shape shape);

private:

    sf::RenderWindow& m_window;
    sf::Cursor m_arrow;
    sf::Cursor m_hand;
    sf::Cursor m_move;
    //! \brief False when the system could not give the pointers.
    bool m_available = false;
    bool m_shown = false;
    Shape m_shape = Shape::Arrow;
};

#endif
