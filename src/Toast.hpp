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

#ifndef TOAST_HPP
#  define TOAST_HPP

#  include "TextPainter.hpp"
#  include <string>

// ****************************************************************************
//! \brief A short message above the status bar ("Opened 15 bookmarks",
//! "Screenshot saved"...). It slides up while appearing, stays a few
//! seconds, then fades away by itself: the user has nothing to close.
// ****************************************************************************
class Toast
{
public:

    explicit Toast(TextPainter& painter);

    //! \brief Show a new message; it replaces the previous one.
    void show(std::string const& message);

    void draw();

private:

    TextPainter& m_painter;
    sf::String m_message;
    //! \brief Time since the message appeared.
    sf::Clock m_clock;
};

#endif
