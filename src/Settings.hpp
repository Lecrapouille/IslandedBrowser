/* *****************************************************************************
** MIT License
**
** Copyright (c) 2022 Quentin Quadrat
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

#ifndef SETTINGS_HPP
#  define SETTINGS_HPP

//! \brief Windows dimensions
#  define WINDOWS_WIDTH  1920
//! \brief Windows dimensions
#  define WINDOWS_HEIGHT 1080
//! \brief Text size
#  define CHARACTER_SIZE 20
//! \brief MessageBar SIZE
#  define MESSAGEBAR_HEIGHT (CHARACTER_SIZE + 5)
//! \brief Duration in seconds for fading text in message box
#  define MESSAGEBAR_FADING_DURATION 2.5f
//! \brief Color of the messagebar
#  define MESSAGEBAR_COLOR sf::Color::Green
//! \brief If set draw arrow heads for graph edges
#  undef DRAW_ARROW_HEAD
//! \brief Arrow head size for drawin graph edges
#  define ARROW_HEAD_SIZE 10.0f
//! \brief Arrow tail size for drawin graph edges
#  define ARROW_TAIL_SIZE 8.5f
//! \brief Arrow head offset
#  define ARROW_HEAD_OFFSET 8.5f
//! \brief Radius of the circle depicting graph nodes
#  define NODE_RADIUS 5.0f
//! \brief Number of points composing the circle for each nodes
#  define CIRCLE_COUNT_POINTS 8
//! \brief Layout border
#  define LAYOUT_BORDER_X NODE_RADIUS
//! \brief Layout border
#  define LAYOUT_BORDER_Y (NODE_RADIUS + MESSAGEBAR_HEIGHT)
//! \brief Layout color
#  define LAYOUT_COLOR sf::Color::Red
//! \brief Color of folder nodes
#  define FOLDER_COLOR sf::Color::Red
//! \brief Color of bookmark nodes
#  define BOOKMARK_COLOR sf::Color::Blue
//! \brief The name of your favorite browser
#  define BROWSER_NAME "firefox"

#endif
