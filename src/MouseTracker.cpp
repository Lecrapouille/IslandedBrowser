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

#include "MouseTracker.hpp"
#include "Bookmarks.hpp"
#include "Settings.hpp"

//------------------------------------------------------------------------------
void MouseTracker::press(sf::Mouse::Button button, sf::Vector2i pixel)
{
    if (button == sf::Mouse::Left)
        m_left = true;
    else if (button == sf::Mouse::Right)
        m_right = true;
    m_press = m_last = pixel;
}

//------------------------------------------------------------------------------
bool MouseTracker::release(sf::Mouse::Button button)
{
    bool click = false;
    if (button == sf::Mouse::Left)
    {
        click = m_left && !m_dragging;
        m_left = false;
    }
    else if (button == sf::Mouse::Right)
    {
        m_right = false;
    }
    // The drag ends when no button is held any more.
    if (!m_left && !m_right)
        m_dragging = false;
    return click;
}

//------------------------------------------------------------------------------
bool MouseTracker::drag(sf::Vector2i now, bool allowed, sf::Vector2i& previous)
{
    if ((m_left || m_right) && !m_dragging && allowed)
    {
        sf::Vector2i const d = now - m_press;
        m_dragging = (d.x * d.x + d.y * d.y > DRAG_THRESHOLD * DRAG_THRESHOLD);
    }
    if (!m_dragging)
        return false;
    previous = m_last;
    m_last = now;
    return true;
}

//------------------------------------------------------------------------------
void MouseTracker::cancel()
{
    m_left = m_right = m_dragging = false;
}

//------------------------------------------------------------------------------
bool MouseTracker::doubleClick(size_t node)
{
    bool const twice = (node != BookmarkNode::none) && (node == m_last_click) &&
        (m_click_clock.getElapsedTime().asSeconds() < DOUBLE_CLICK_DELAY);
    // After a double click, the next click starts a new pair: a triple click
    // is not two double clicks.
    m_last_click = twice ? BookmarkNode::none : node;
    m_click_clock.restart();
    return twice;
}
