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

#include "Colors.hpp"
#include "Settings.hpp"
#include <cmath>

//------------------------------------------------------------------------------
sf::Color blend(sf::Color a, sf::Color b, float t)
{
    auto mix = [t](sf::Uint8 x, sf::Uint8 y) {
        return sf::Uint8(float(x) + (float(y) - float(x)) * t);
    };
    return sf::Color(mix(a.r, b.r), mix(a.g, b.g), mix(a.b, b.b), a.a);
}

//------------------------------------------------------------------------------
sf::Color fade(sf::Color color, float alpha)
{
    color.a = sf::Uint8(float(color.a) * alpha);
    return color;
}

//------------------------------------------------------------------------------
Palette::Palette(IslandedBrowser const& island)
    : m_island(island)
{
    for (size_t i = 0u; i < island.bookmarks().islands.size(); ++i)
        m_colors.push_back(hue(i));
}

//------------------------------------------------------------------------------
//! The color is first chosen as a position on the color wheel (the "hue"),
//! with a fixed vividness and brightness, then converted to the red, green
//! and blue amounts that screens understand (HSV to RGB conversion).
//------------------------------------------------------------------------------
sf::Color Palette::hue(size_t index)
{
    float const h = std::fmod(0.11f + 0.618034f * float(index), 1.0f) * 6.0f;
    float const s = 0.65f;
    float const v = 0.95f;
    float const c = v * s;
    float const x = c * (1.0f - std::fabs(std::fmod(h, 2.0f) - 1.0f));
    float r = 0, g = 0, b = 0;
    switch (int(h))
    {
    case 0: r = c; g = x; break;
    case 1: r = x; g = c; break;
    case 2: g = c; b = x; break;
    case 3: g = x; b = c; break;
    case 4: r = x; b = c; break;
    default: r = c; b = x; break;
    }
    float const m = v - c;
    return sf::Color(sf::Uint8((r + m) * 255), sf::Uint8((g + m) * 255),
                     sf::Uint8((b + m) * 255));
}

//------------------------------------------------------------------------------
sf::Color Palette::island(size_t rank) const
{
    return m_colors.empty() ? sf::Color::White : m_colors[rank % m_colors.size()];
}

//------------------------------------------------------------------------------
sf::Color Palette::ofNode(size_t node) const
{
    ForceDirectedGraph const& layout = m_island.layout();
    size_t const vertex = layout.vertexOf(node);
    if (vertex == BookmarkNode::none || m_colors.empty())
        return TEXT_COLOR;
    return island(layout.vertices()[vertex].island);
}
