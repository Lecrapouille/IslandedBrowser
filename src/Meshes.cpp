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

#include "Meshes.hpp"
#include "Settings.hpp"
#include <cmath>

namespace mesh {

//------------------------------------------------------------------------------
void line(sf::VertexArray& lines, sf::Vector2f a, sf::Vector2f b, sf::Color color)
{
    lines.append(sf::Vertex(a, color));
    lines.append(sf::Vertex(b, color));
}

//------------------------------------------------------------------------------
void band(sf::VertexArray& triangles, sf::Vector2f a, sf::Vector2f b,
          float width, sf::Color color)
{
    sf::Vector2f const direction = b - a;
    float const length = std::sqrt(direction.x * direction.x + direction.y * direction.y);
    if (length <= 0.0f)
        return;
    // Half the width, perpendicular to the segment: it gives the four
    // corners of the rectangle.
    sf::Vector2f const normal(-direction.y / length * width / 2.0f,
                              direction.x / length * width / 2.0f);
    sf::Vertex const corners[4] = {
        sf::Vertex(a + normal, color), sf::Vertex(b + normal, color),
        sf::Vertex(b - normal, color), sf::Vertex(a - normal, color)
    };
    for (int i: { 0, 1, 2, 0, 2, 3 })
        triangles.append(corners[i]);
}

//------------------------------------------------------------------------------
void disc(sf::VertexArray& triangles, sf::Vector2f center, float radius, sf::Color color)
{
    int const sides = CIRCLE_COUNT_POINTS;
    sf::Vector2f previous(center.x + radius, center.y);
    for (int i = 1; i <= sides; ++i)
    {
        float const angle = 6.2831853f * float(i) / float(sides);
        sf::Vector2f const next(center.x + radius * std::cos(angle),
                                center.y + radius * std::sin(angle));
        triangles.append(sf::Vertex(center, color));
        triangles.append(sf::Vertex(previous, color));
        triangles.append(sf::Vertex(next, color));
        previous = next;
    }
}

//------------------------------------------------------------------------------
void ring(sf::VertexArray& triangles, sf::Vector2f center, float inner,
          float outer, sf::Color color)
{
    int const sides = 24;
    for (int i = 0; i < sides; ++i)
    {
        float const a0 = 6.2831853f * float(i) / float(sides);
        float const a1 = 6.2831853f * float(i + 1) / float(sides);
        sf::Vector2f const u0(std::cos(a0), std::sin(a0));
        sf::Vector2f const u1(std::cos(a1), std::sin(a1));
        sf::Vertex const corners[4] = {
            sf::Vertex(center + u0 * inner, color), sf::Vertex(center + u0 * outer, color),
            sf::Vertex(center + u1 * outer, color), sf::Vertex(center + u1 * inner, color)
        };
        for (int k: { 0, 1, 2, 0, 2, 3 })
            triangles.append(corners[k]);
    }
}

} // namespace mesh
