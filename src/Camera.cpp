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

#include "Camera.hpp"
#include <algorithm>
#include <cmath>

//------------------------------------------------------------------------------
Camera::Camera(sf::RenderWindow& window)
    : m_window(window)
{
    sf::Vector2f const size(window.getSize());
    m_screen = sf::View(sf::FloatRect(0.0f, 0.0f, size.x, size.y));
    m_world = m_screen;
}

//------------------------------------------------------------------------------
float Camera::scale() const
{
    return m_world.getSize().x / float(m_window.getSize().x);
}

//------------------------------------------------------------------------------
sf::Vector2f Camera::toWorld(sf::Vector2i pixel) const
{
    return m_window.mapPixelToCoords(pixel, m_world);
}

//------------------------------------------------------------------------------
sf::Vector2i Camera::toPixel(sf::Vector2f position) const
{
    return m_window.mapCoordsToPixel(position, m_world);
}

//------------------------------------------------------------------------------
void Camera::resize(sf::Vector2u size)
{
    float const s = scale();
    sf::Vector2f const pixels(size);
    m_screen = sf::View(sf::FloatRect(0.0f, 0.0f, pixels.x, pixels.y));
    m_world.setSize(pixels * s);
}

//------------------------------------------------------------------------------
sf::Vector2f Camera::fitSize(sf::FloatRect const& area) const
{
    sf::Vector2f const window(m_window.getSize());
    float const margin = 0.05f * std::max(area.width, area.height) + 20.0f;
    // The most constraining direction (width or height) decides the zoom.
    float const s = std::max((area.width + 2.0f * margin) / window.x,
                             (area.height + 2.0f * margin) / window.y);
    return sf::Vector2f(window.x * s, window.y * s);
}

//------------------------------------------------------------------------------
void Camera::fit(sf::FloatRect const& area)
{
    m_world.setCenter(area.left + area.width / 2.0f, area.top + area.height / 2.0f);
    m_world.setSize(fitSize(area));
    m_gliding = false;
}

//------------------------------------------------------------------------------
void Camera::takeControl()
{
    m_following = false;
    m_gliding = false;
}

//------------------------------------------------------------------------------
void Camera::zoom(float factor, sf::Vector2i pixel)
{
    // Zoom from the center, then move back the point that was under the
    // mouse: it looks like the zoom happened around the mouse.
    sf::Vector2f const before = toWorld(pixel);
    m_world.zoom(factor);
    sf::Vector2f const after = toWorld(pixel);
    m_world.move(before - after);
    takeControl();
}

//------------------------------------------------------------------------------
void Camera::pan(sf::Vector2f pixels)
{
    m_world.move(pixels * scale());
    takeControl();
}

//------------------------------------------------------------------------------
void Camera::drag(sf::Vector2i from, sf::Vector2i to)
{
    m_world.move(toWorld(from) - toWorld(to));
    takeControl();
}

//------------------------------------------------------------------------------
void Camera::glideTo(sf::Vector2f position, float hidden)
{
    // When already gliding, keep the zoom it is heading to.
    sf::Vector2f const size = m_gliding ? m_target_size : m_world.getSize();
    float const s = size.x / float(m_window.getSize().x);
    m_target_center = position + sf::Vector2f(hidden / 2.0f * s, 0.0f);
    m_target_size = size;
    m_gliding = true;
    m_following = false;
}

//------------------------------------------------------------------------------
void Camera::glideOver(sf::FloatRect const& area, float hidden)
{
    sf::Vector2f const window(m_window.getSize());
    float const visible = std::max(window.x - hidden, window.x / 2.0f);
    // Pretend the area is wider, so that it fits in the visible part only.
    sf::FloatRect widened = area;
    widened.width = area.width * window.x / visible;
    m_target_size = fitSize(widened);
    float const s = m_target_size.x / window.x;
    m_target_center = sf::Vector2f(area.left + area.width / 2.0f + hidden / 2.0f * s,
                                   area.top + area.height / 2.0f);
    m_gliding = true;
    m_following = false;
}

//------------------------------------------------------------------------------
void Camera::update(float dt)
{
    if (!m_gliding)
        return;

    // Cover a part of the remaining way, the same part whatever the frame
    // rate: the camera slows down when it gets close (exponential easing).
    float const k = 1.0f - std::exp(-10.0f * std::min(dt, 0.1f));
    sf::Vector2f const center = m_world.getCenter();
    sf::Vector2f const size = m_world.getSize();
    m_world.setCenter(center + (m_target_center - center) * k);
    m_world.setSize(size + (m_target_size - size) * k);

    // Close enough (less than half a pixel): land exactly on the target.
    sf::Vector2f const d = m_target_center - m_world.getCenter();
    if (std::fabs(d.x) + std::fabs(d.y) < 0.5f * scale() &&
        std::fabs(m_target_size.x - m_world.getSize().x) < 0.002f * m_target_size.x)
    {
        m_world.setCenter(m_target_center);
        m_world.setSize(m_target_size);
        m_gliding = false;
    }
}
