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

#ifndef DRAWABLE_HPP
#  define DRAWABLE_HPP

#  include <SFML/Graphics.hpp>
#  include <SFML/System.hpp>
#  include "Settings.hpp"
#  include <cmath>

// *****************************************************************************
//! \brief A class to draw text inside a rectangle with a fading effect.
// *****************************************************************************
class MessageBar : public sf::Drawable
{
public:

    MessageBar()
    {
        m_shape.setFillColor(sf::Color(100, 100, 100));
        m_shape.setOutlineThickness(-1);
        m_shape.setOutlineColor(sf::Color::Black);
        m_timer.restart();
    }

    void entry(const std::string& message, sf::Color const& color)
    {
        m_message = message;
        m_shape.setFillColor(color);
        m_text.setString(m_message);
        m_timer.restart();
    }

    void font(const char* path)
    {
        m_font.loadFromFile(path);
        m_text.setPosition(0, 0);
        m_text.setFont(m_font);
        m_text.setCharacterSize(CHARACTER_SIZE);
        m_text.setFillColor(sf::Color::Black);
    }

    void size(sf::Vector2u const& dimensions)
    {
        m_shape.setSize(sf::Vector2f(float(dimensions.x), float(MESSAGEBAR_HEIGHT)));
    }

private:

    void draw(sf::RenderTarget& target, sf::RenderStates /*states*/) const override final
    {
        float period = m_timer.getElapsedTime().asSeconds();
        if (period >= MESSAGEBAR_FADING_DURATION)
        {
            period = MESSAGEBAR_FADING_DURATION;
        }
        else
        {
            target.draw(m_shape);
            target.draw(m_text);
        }
    }

private:

    //! \brief Timer for removing the text
    sf::Clock m_timer;

    //! \brief Text displayed on the entry
    sf::Text m_text;
    sf::Font m_font;

    //! \brief Handles appearance of the entry
    sf::RectangleShape m_shape;

    //! \brief String returned when the entry is activated
    std::string m_message;
};

// *****************************************************************************
//! \brief Class allowing to draw an arrow. Arrows are needed for drawing Petri
//! arcs.
// *****************************************************************************
class Arrow : public sf::Drawable
{
public:

    Arrow(const float xa, const float ya, const float xb, const float yb)
    {
        // Arc magnitude
        const float arrowLength = norm(xa, ya, xb, yb);

        // Orientation
        const float teta = (yb - ya) / (xb - xa);
        float arrowAngle = std::atan(teta) * 180.0f / 3.1415f; // rad -> deg
        if (xb < xa)
            arrowAngle += 180.f;
        else if (yb < ya)
            arrowAngle += 360.f;

        // Reduce the arrow magnitude to avoid entering in the place and having
        // a mush of pixels when multiple arrows are pointing on the same
        // position. To get full scaled arrow comment this block of code and
        // uncomment xa, xb, ya, yb and tailSize.
        float r = arrowLength - ARROW_HEAD_OFFSET;
        float dx = ((xb - xa) * r) / arrowLength;
        float dy = ((yb - ya) * r) / arrowLength;
        float a1 = xb - dx;
        float b1 = yb - dy;

        // Head of the arrow
        const sf::Vector2f arrowHeadSize{ ARROW_HEAD_SIZE, ARROW_HEAD_SIZE };
#if defined(DRAW_ARROW_HEAD)
        m_head = sf::ConvexShape{ 3 };
        m_head.setPoint(0, { 0.f, 0.f });
        m_head.setPoint(1, { arrowHeadSize.x, arrowHeadSize.y / 2.f });
        m_head.setPoint(2, { 0.f, arrowHeadSize.y });
        m_head.setOrigin(arrowHeadSize.x, arrowHeadSize.y / 2.f);
        m_head.setPosition(sf::Vector2f(a2, b2 /*xb, yb*/));
        m_head.setRotation(arrowAngle);
#endif

        // Tail of the arrow.
        const sf::Vector2f tailSize{ r - arrowHeadSize.x - ARROW_TAIL_SIZE, 2.f };
        m_tail = sf::RectangleShape{ tailSize };
        m_tail.setOrigin(0.f, tailSize.y / 2.f);
        m_tail.setPosition(sf::Vector2f(a1, b1 /*xa, ya*/));
        m_tail.setRotation(arrowAngle);

#if defined(DRAW_ARROW_HEAD)
        m_head.setFillColor(sf::Color::Black);
#endif
        m_tail.setFillColor(sf::Color::Black);
    }

    Arrow(sf::Vector2f const& startPoint, sf::Vector2f const& endPoint)
        : Arrow(startPoint.x, startPoint.y, endPoint.x, endPoint.y)
    {}

private:

    void draw(sf::RenderTarget& target, sf::RenderStates /*states*/) const override final
    {
        target.draw(m_tail);
#if defined(DRAW_ARROW_HEAD)
        target.draw(m_head);
#endif
    }

    inline float norm(const float xa, const float ya, const float xb, const float yb)
    {
        return sqrtf((xb - xa) * (xb - xa) + (yb - ya) * (yb - ya));
    }

private:

    sf::RectangleShape m_tail;
#if defined(DRAW_ARROW_HEAD)
    sf::ConvexShape m_head;
#endif
};

// *****************************************************************************
//! \brief
// *****************************************************************************
class Circle: public sf::Drawable
{
public:

    Circle(sf::Vector2f p, float r, sf::Color color)
        : m_shape(r, CIRCLE_COUNT_POINTS)
    {
        m_shape.setOrigin(m_shape.getRadius(), m_shape.getRadius());
        m_shape.setPosition(p);
        m_shape.setFillColor(color);
        m_shape.setOutlineThickness(1.0f);
        m_shape.setOutlineColor(color);
    }

    void position(sf::Vector2f const& p)
    {
        m_shape.setPosition(p);
    }

private:

    void draw(sf::RenderTarget& target, sf::RenderStates states) const override final
    {
        target.draw(m_shape, states);
    }

private:

    sf::CircleShape m_shape;
};

#endif
