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

#include "Toast.hpp"
#include "Colors.hpp"
#include <algorithm>
#include <cmath>

namespace {

//! \brief Seconds to appear, and to disappear.
float const FADE_IN = 0.15f;
float const FADE_OUT = 0.4f;

} // namespace

//------------------------------------------------------------------------------
Toast::Toast(TextPainter& painter)
    : m_painter(painter)
{}

//------------------------------------------------------------------------------
void Toast::show(std::string const& message)
{
    m_message = utf8(message);
    m_clock.restart();
}

//------------------------------------------------------------------------------
void Toast::draw()
{
    if (m_message.isEmpty() || !m_painter.hasFont())
        return;
    float const t = m_clock.getElapsedTime().asSeconds();
    if (t >= TOAST_DURATION + FADE_OUT)
    {
        m_message.clear();
        return;
    }

    // From 0 (invisible) to 1 (fully visible): rises quickly, stays at 1,
    // then goes down at the end.
    float const alpha = std::min(std::min(t / FADE_IN, 1.0f),
                                 std::max(0.0f, 1.0f - (t - TOAST_DURATION) / FADE_OUT));

    sf::Vector2f const window = m_painter.windowSize();
    float const padding = 16.0f;
    unsigned const size = SMALL_CHARACTER_SIZE + 1u;
    sf::String const message = m_painter.wrap(m_message, std::min(700.0f, window.x - 80.0f),
                                              size, 1u).front();
    float const width = m_painter.width(message, size) + 2.0f * padding;
    float const height = 36.0f;
    // A few pixels lower at the start: it seems to slide up.
    float const lift = 8.0f * (1.0f - std::min(t / FADE_IN, 1.0f));
    sf::FloatRect const rect(std::round((window.x - width) / 2.0f),
                             window.y - STATUSBAR_HEIGHT - height - 16.0f + lift,
                             width, height);
    m_painter.box(rect, fade(PANEL_COLOR, alpha), fade(PANEL_BORDER_COLOR, alpha));
    m_painter.bar(sf::FloatRect(rect.left, rect.top, 3.0f, height), fade(URL_COLOR, alpha));
    m_painter.write(message, sf::Vector2f(rect.left + padding, rect.top + 8.0f), size,
                    fade(TEXT_COLOR, alpha));
}
