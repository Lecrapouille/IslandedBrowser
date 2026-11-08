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

#include "TextPainter.hpp"
#include <cmath>
#include <iostream>

//------------------------------------------------------------------------------
sf::String utf8(std::string const& s)
{
    return sf::String::fromUtf8(s.begin(), s.end());
}

//------------------------------------------------------------------------------
std::string toUtf8(sf::String const& s)
{
    auto const bytes = s.toUtf8();
    return std::string(bytes.begin(), bytes.end());
}

//------------------------------------------------------------------------------
TextPainter::TextPainter(sf::RenderTarget& target, std::string const& font_path)
    : m_target(target)
{
    m_has_font = m_font.loadFromFile(font_path);
    if (m_has_font)
        m_text.setFont(m_font);
    else
        std::cerr << font_path << " not found: texts are not displayed" << std::endl;
}

//------------------------------------------------------------------------------
float TextPainter::lineHeight(unsigned size)
{
    return std::round(float(size) * 1.35f);
}

//------------------------------------------------------------------------------
float TextPainter::write(sf::String const& string, sf::Vector2f position,
                         unsigned size, sf::Color color, bool bold)
{
    if (!m_has_font)
        return 0.0f;
    m_text.setString(string);
    m_text.setCharacterSize(size);
    m_text.setFillColor(color);
    m_text.setStyle(bold ? sf::Text::Bold : sf::Text::Regular);
    // Whole pixels: a text placed between two pixels looks blurry.
    m_text.setPosition(std::round(position.x), std::round(position.y));
    m_target.draw(m_text);
    return m_text.getLocalBounds().width;
}

//------------------------------------------------------------------------------
float TextPainter::width(sf::String const& string, unsigned size, bool bold) const
{
    if (!m_has_font)
        return 0.0f;
    // Each letter has its own width ("advance"): add them up.
    float total = 0.0f;
    for (sf::Uint32 c: string)
        total += m_font.getGlyph(c, size, bold).advance;
    return total;
}

//------------------------------------------------------------------------------
//! Letters are added one by one to the current line until it is full. The
//! line is then cut after its last space, so words stay whole. A word longer
//! than a whole line (typically an URL) is cut anywhere.
//------------------------------------------------------------------------------
std::vector<sf::String> TextPainter::wrap(sf::String const& string, float width,
                                          unsigned size, size_t max_lines) const
{
    static constexpr size_t no_space = static_cast<size_t>(-1);
    std::vector<sf::String> lines;
    if (!m_has_font)
        return lines;
    size_t const n = string.getSize();
    size_t i = 0u;
    while (i < n && lines.size() < max_lines)
    {
        // A line never starts with a space.
        while (i < n && string[i] == ' ')
            ++i;
        size_t const start = i;
        size_t last_space = no_space;
        float w = 0.0f;
        size_t j = i;
        for (; j < n; ++j)
        {
            float const advance = m_font.getGlyph(string[j], size, false).advance;
            if (w + advance > width && j > start)
                break;
            w += advance;
            if (string[j] == ' ')
                last_space = j;
        }
        if (j < n && last_space != no_space && last_space > start)
            j = last_space;
        lines.push_back(string.substring(start, j - start));
        i = j;
    }

    // Some text did not fit: shorten the last line until "…" fits after it.
    while (i < n && string[i] == ' ')
        ++i;
    if (i < n && !lines.empty())
    {
        sf::String& last = lines.back();
        sf::String const ellipsis(L"…");
        while (!last.isEmpty() && this->width(last + ellipsis, size) > width)
            last.erase(last.getSize() - 1u);
        last += ellipsis;
    }
    return lines;
}

//------------------------------------------------------------------------------
void TextPainter::box(sf::FloatRect const& rect, sf::Color fill, sf::Color border)
{
    sf::RectangleShape shape(sf::Vector2f(std::round(rect.width), std::round(rect.height)));
    shape.setPosition(std::round(rect.left), std::round(rect.top));
    shape.setFillColor(fill);
    shape.setOutlineColor(border);
    shape.setOutlineThickness(border.a ? 1.0f : 0.0f);
    m_target.draw(shape);
}

//------------------------------------------------------------------------------
void TextPainter::bar(sf::FloatRect const& rect, sf::Color color)
{
    sf::RectangleShape shape(sf::Vector2f(rect.width, rect.height));
    shape.setPosition(rect.left, rect.top);
    shape.setFillColor(color);
    m_target.draw(shape);
}
