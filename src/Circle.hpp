#ifndef CIRCLE_HPP
#  define CIRCLE_HPP

#  include <SFML/Graphics.hpp>
#  include <SFML/System.hpp>

// *****************************************************************************
//! \brief Class allowing to draw an arrow. Arrows are needed for drawing Petri
//! arcs.
// *****************************************************************************
class Arrow : public sf::Drawable
{
public:

    Arrow(const float xa, const float ya, const float xb, const float yb)
    {
        const float AS = 10.0f;
        const float TS = 8.5f;
        const float R = 8.5f;

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
        float r = arrowLength - R;
        float dx = ((xb - xa) * r) / arrowLength;
        float dy = ((yb - ya) * r) / arrowLength;
        float a1 = xb - dx;
        float b1 = yb - dy;
        float a2 = xa + dx;
        float b2 = ya + dy;

        // Head of the arrow
        const sf::Vector2f arrowHeadSize{ AS, AS };
        //m_head = sf::ConvexShape{ 3 };
        //m_head.setPoint(0, { 0.f, 0.f });
        //m_head.setPoint(1, { arrowHeadSize.x, arrowHeadSize.y / 2.f });
        //m_head.setPoint(2, { 0.f, arrowHeadSize.y });
        //m_head.setOrigin(arrowHeadSize.x, arrowHeadSize.y / 2.f);
        //m_head.setPosition(sf::Vector2f(a2, b2 /*xb, yb*/));
        //m_head.setRotation(arrowAngle);

        // Tail of the arrow.
        const sf::Vector2f tailSize{ r - arrowHeadSize.x - TS, 2.f };
        m_tail = sf::RectangleShape{ tailSize };
        m_tail.setOrigin(0.f, tailSize.y / 2.f);
        m_tail.setPosition(sf::Vector2f(a1, b1 /*xa, ya*/));
        m_tail.setRotation(arrowAngle);

        //m_head.setFillColor(sf::Color::Black);
        m_tail.setFillColor(sf::Color::Black);
    }

    Arrow(sf::Vector2f const& startPoint, sf::Vector2f const& endPoint)
        : Arrow(startPoint.x, startPoint.y, endPoint.x, endPoint.y)
    {}

private:

    void draw(sf::RenderTarget& target, sf::RenderStates /*states*/) const override final
    {
        target.draw(m_tail);
        //target.draw(m_head);
    }

    inline float norm(const float xa, const float ya, const float xb, const float yb)
    {
        return sqrtf((xb - xa) * (xb - xa) + (yb - ya) * (yb - ya));
    }

private:

    sf::RectangleShape m_tail;
    //sf::ConvexShape m_head;
};

// *****************************************************************************
//! \brief
// *****************************************************************************
class Circle: public sf::Drawable
{
public:

    Circle(float x, float y, float r, sf::Color color = sf::Color::Red)
        : m_shape(r, 16)
    {
        m_shape.setOrigin(m_shape.getRadius(), m_shape.getRadius());
        m_shape.setPosition(x, y);
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
