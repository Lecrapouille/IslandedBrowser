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

#ifndef CAMERA_HPP
#  define CAMERA_HPP

#  include <SFML/Graphics.hpp>

// ****************************************************************************
//! \brief Decides which part of the map is visible in the window, like a
//! camera flying over the archipelago.
//!
//! Two coordinate systems exist side by side:
//! - the "world": positions of the nodes computed by the layout, in their
//!   own units, independent of the window;
//! - the "screen": pixels of the window, where texts and panels are drawn so
//!   they keep the same size whatever the zoom.
//!
//! While the layout is still moving, the camera follows it so the whole map
//! stays visible. As soon as the user zooms or moves the map, the camera
//! stops following and stays where the user put it.
//!
//! Moves asked by the program (zoom on a folder, go to a search result) are
//! not instantaneous: the camera glides toward its target, covering a fixed
//! part of the remaining distance at each frame, so it slows down smoothly
//! when it arrives.
// ****************************************************************************
class Camera
{
public:

    explicit Camera(sf::RenderWindow& window);

    //-------------------------------------------------------------------------
    //! \brief The window changed size: keep the same zoom, show more or less
    //! of the map.
    //-------------------------------------------------------------------------
    void resize(sf::Vector2u size);

    //-------------------------------------------------------------------------
    //! \brief Show the given area of the world, with a small margin, at once.
    //-------------------------------------------------------------------------
    void fit(sf::FloatRect const& area);

    //-------------------------------------------------------------------------
    //! \brief Zoom in (factor < 1) or out (factor > 1). The point of the map
    //! under the given pixel stays under it, so the user zooms where the
    //! mouse is.
    //-------------------------------------------------------------------------
    void zoom(float factor, sf::Vector2i pixel);

    //-------------------------------------------------------------------------
    //! \brief Move the map by a number of pixels (arrow keys).
    //-------------------------------------------------------------------------
    void pan(sf::Vector2f pixels);

    //-------------------------------------------------------------------------
    //! \brief Move the map with the mouse: the point that was under the
    //! pixel "from" is now under the pixel "to".
    //-------------------------------------------------------------------------
    void drag(sf::Vector2i from, sf::Vector2i to);

    //-------------------------------------------------------------------------
    //! \brief Glide to a world position without changing the zoom.
    //! \param[in] hidden width in pixels hidden on the right of the window by
    //! a panel: the position is centered in what remains visible.
    //-------------------------------------------------------------------------
    void glideTo(sf::Vector2f position, float hidden);

    //-------------------------------------------------------------------------
    //! \brief Glide and zoom so that the given world area fills the part of
    //! the window that is not hidden by a panel on the right.
    //-------------------------------------------------------------------------
    void glideOver(sf::FloatRect const& area, float hidden);

    //-------------------------------------------------------------------------
    //! \brief Advance the gliding by dt seconds (called at each frame).
    //-------------------------------------------------------------------------
    void update(float dt);

    //-------------------------------------------------------------------------
    //! \brief Follow the layout again: the whole map is shown while it moves.
    //-------------------------------------------------------------------------
    void follow() { m_following = true; }
    bool following() const { return m_following; }

    //-------------------------------------------------------------------------
    //! \brief How many world units fit in one pixel: small when zoomed in.
    //-------------------------------------------------------------------------
    float scale() const;

    sf::View const& world() const { return m_world; }
    sf::View const& screen() const { return m_screen; }
    sf::Vector2f toWorld(sf::Vector2i pixel) const;
    sf::Vector2i toPixel(sf::Vector2f position) const;

private:

    //-------------------------------------------------------------------------
    //! \brief Size of the view that shows the area with a margin, with the
    //! same proportions as the window (so the map is not stretched).
    //-------------------------------------------------------------------------
    sf::Vector2f fitSize(sf::FloatRect const& area) const;

    //-------------------------------------------------------------------------
    //! \brief The user took control: stop following and gliding.
    //-------------------------------------------------------------------------
    void takeControl();

private:

    sf::RenderWindow& m_window;
    //! \brief Part of the world shown in the window.
    sf::View m_world;
    //! \brief The window itself, in pixels.
    sf::View m_screen;
    bool m_following = true;
    bool m_gliding = false;
    sf::Vector2f m_target_center;
    sf::Vector2f m_target_size;
};

#endif
