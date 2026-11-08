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

#include "TerrainPainter.hpp"
#include "Maths.hpp"
#include "Meshes.hpp"
#include "Noise.hpp"
#include "Settings.hpp"
#include <algorithm>
#include <cmath>

using noise::smoothstep;

namespace {

//! \brief Most pixels along the longest side of the picture.
unsigned const PICTURE = 2048u;

//------------------------------------------------------------------------------
//! \brief A color as three numbers from 0 to 1, easier to mix than bytes.
//------------------------------------------------------------------------------
struct Rgb
{
    float r;
    float g;
    float b;
};

Rgb mix(Rgb a, Rgb b, float t)
{
    return { a.r + (b.r - a.r) * t, a.g + (b.g - a.g) * t, a.b + (b.b - a.b) * t };
}

Rgb rgb(sf::Color c)
{
    return { float(c.r) / 255.0f, float(c.g) / 255.0f, float(c.b) / 255.0f };
}

sf::Color toColor(Rgb c)
{
    auto byte = [](float v) { return sf::Uint8(clamp(int(v * 255.0f + 0.5f), 0, 255)); };
    return sf::Color(byte(c.r), byte(c.g), byte(c.b));
}

// The colors of the map. The deep sea is the background of the window, so
// the picture melts into it.
Rgb const DEEP = rgb(SEA_COLOR);
Rgb const SHALLOW{ 0.10f, 0.30f, 0.42f };
Rgb const LAGOON{ 0.24f, 0.52f, 0.58f };
Rgb const FOAM{ 0.80f, 0.90f, 0.92f };
Rgb const SAND{ 0.86f, 0.80f, 0.60f };
Rgb const GRASS{ 0.52f, 0.68f, 0.36f };
Rgb const FOREST{ 0.28f, 0.46f, 0.26f };
Rgb const UPLAND{ 0.58f, 0.56f, 0.42f };
Rgb const ROCK{ 0.56f, 0.52f, 0.47f };
Rgb const SUMMIT{ 0.82f, 0.80f, 0.76f };
Rgb const BORDER{ 1.00f, 0.96f, 0.86f };
sf::Color const RIVER(70, 150, 215);

//------------------------------------------------------------------------------
//! \brief How much a line, half px pixels wide, covers a pixel at distance
//! px pixels from its middle: 1 on it, 0 far from it, soft in between.
//------------------------------------------------------------------------------
float line(float distance, float half)
{
    return clamp(half - distance + 0.5f, 0.0f, 1.0f);
}

} // namespace

//------------------------------------------------------------------------------
TerrainPainter::TerrainPainter(Terrain const& terrain, Palette const& palette)
    : m_terrain(terrain), m_palette(palette), m_rivers(sf::Triangles)
{}

//------------------------------------------------------------------------------
std::vector<float> TerrainPainter::heights(unsigned width, unsigned height, float pixel) const
{
    sf::FloatRect const area = m_terrain.area();
    std::vector<float> result(std::size_t(width) * height);
    #pragma omp parallel for schedule(dynamic, 8)
    for (int y = 0; y < int(height); ++y)
    {
        for (unsigned x = 0u; x < width; ++x)
        {
            sf::Vector2f const at(area.left + (float(x) + 0.5f) * pixel,
                                  area.top + (float(y) + 0.5f) * pixel);
            result[std::size_t(y) * width + x] = m_terrain.heightAt(at);
        }
    }
    return result;
}

//------------------------------------------------------------------------------
sf::Color TerrainPainter::colorAt(std::vector<float> const& heights, unsigned x, unsigned y,
                                  unsigned width, unsigned height, float pixel) const
{
    auto h = [&](unsigned i, unsigned j) {
        return heights[std::size_t(std::min(j, height - 1u)) * width + std::min(i, width - 1u)];
    };
    float const here = h(x, y);
    float const metres = pixel * m_terrain.metresPerUnit(); // metres per pixel
    // How fast the ground changes here, in both directions (metres per metre).
    float const dx = (h(x + 1u, y) - h(x > 0u ? x - 1u : 0u, y)) / (2.0f * metres);
    float const dy = (h(x, y + 1u) - h(x, y > 0u ? y - 1u : 0u)) / (2.0f * metres);

    if (here <= 0.0f)
    {
        float const depth = -here;
        Rgb color = mix(LAGOON, SHALLOW, smoothstep(0.0f, 18.0f, depth));
        color = mix(color, DEEP, smoothstep(15.0f, 85.0f, depth));
        // Lines of equal depth: the distance to the level, in pixels, is the
        // gap in height divided by the slope.
        float const slope = std::max(std::fabs(dx) + std::fabs(dy), 1.0e-4f);
        for (float level: { 6.0f, 30.0f })
            color = mix(color, FOAM, 0.22f * line(std::fabs(depth - level) / slope / metres, 0.5f));
        return toColor(color);
    }

    sf::Vector2f const at(m_terrain.area().left + (float(x) + 0.5f) * pixel,
                          m_terrain.area().top + (float(y) + 0.5f) * pixel);
    VoronoiGrid::Hit const hit = m_terrain.cellAt(at);
    int const region = m_terrain.regionOf(hit.cell);
    bool const part = m_terrain.holdsBookmark(hit.cell);

    // Ground: wetter areas are forests; then the height.
    float const wet = noise::value(at.x * m_terrain.metresPerUnit() / 900.0f,
                                   at.y * m_terrain.metresPerUnit() / 900.0f,
                                   m_terrain.seed() + 41u);
    Rgb color = mix(GRASS, FOREST, smoothstep(0.45f, 0.7f, wet));
    color = mix(SAND, color, smoothstep(2.0f, 7.0f, here));
    color = mix(color, UPLAND, smoothstep(110.0f, 250.0f, here));
    color = mix(color, ROCK, smoothstep(250.0f, 400.0f, here));
    color = mix(color, SUMMIT, smoothstep(440.0f, 540.0f, here));
    // Each cell a slightly different tone: the mosaic of the cells shows.
    float const tone = 0.95f + 0.1f * noise::lattice(hit.cell, 7, m_terrain.seed());
    color = { color.r * tone, color.g * tone, color.b * tone };

    // Tint of the island, a different shade per region.
    int const island = m_terrain.islandOf(hit.cell);
    if (island >= 0)
    {
        Rgb const hue = rgb(m_palette.island(size_t(island)));
        float const shade = (region >= 0) ? 0.75f + 0.12f * float(region % 4) : 0.85f;
        color = mix(color, { hue.r * shade, hue.g * shade, hue.b * shade }, part ? 0.55f : 0.32f);
    }

    // Light from the top left: slopes facing it are brighter.
    float const relief = 4.0f;
    float const nx = -relief * dx;
    float const ny = -relief * dy;
    float const norm = std::sqrt(nx * nx + ny * ny + 1.0f);
    float const lx = -0.5f, ly = -0.5f, lz = 0.707f;
    float const light = (nx * lx + ny * ly + lz) / norm;
    float const shade = clamp(1.0f + 1.3f * (light - lz), 0.45f, 1.4f);
    color = { color.r * shade, color.g * shade, color.b * shade };

    // Borders of the cells, in pixels.
    if (hit.across >= 0)
    {
        bool const other_region = (m_terrain.regionOf(hit.across) != region);
        bool const other_part = other_region || part || m_terrain.holdsBookmark(hit.across);
        float half = 0.3f;
        float strength = 0.16f;
        if (other_region)
        {
            half = 0.9f;
            strength = 0.7f;
        }
        else if (other_part)
        {
            half = 0.5f;
            strength = 0.4f;
        }
        color = mix(color, BORDER, strength * line(hit.edge / metres, half));
    }
    return toColor(color);
}

//------------------------------------------------------------------------------
void TerrainPainter::update()
{
    if (!m_terrain.ready())
        return;
    sf::FloatRect const area = m_terrain.area();
    unsigned const longest = std::min(PICTURE, sf::Texture::getMaximumSize());
    float const pixel = std::max(area.width, area.height) / float(longest);
    unsigned const width = unsigned(std::ceil(area.width / pixel));
    unsigned const height = unsigned(std::ceil(area.height / pixel));

    std::vector<float> const h = heights(width, height, pixel);
    std::vector<sf::Uint8> pixels(std::size_t(width) * height * 4u);
    #pragma omp parallel for schedule(dynamic, 8)
    for (int y = 0; y < int(height); ++y)
    {
        for (unsigned x = 0u; x < width; ++x)
        {
            sf::Color const c = colorAt(h, x, unsigned(y), width, height, pixel);
            std::size_t const i = (std::size_t(y) * width + x) * 4u;
            pixels[i] = c.r;
            pixels[i + 1u] = c.g;
            pixels[i + 2u] = c.b;
            pixels[i + 3u] = 255u;
        }
    }
    m_texture.create(width, height);
    m_texture.update(pixels.data());
    m_texture.setSmooth(true);
    m_sprite.setTexture(m_texture, true);
    m_sprite.setPosition(area.left, area.top);
    m_sprite.setScale(pixel, pixel);
    traceRivers();
}

//------------------------------------------------------------------------------
//! The centers of the cells make a zigzag. A Catmull-Rom spline rounds it
//! into a smooth curve that still passes through every center: a river
//! keeps touching its spring (the node) and its mouth, which is a point of
//! the river it flows into, so a tributary really joins its river. A disc
//! at each joint hides the gaps between two pieces of different directions.
//------------------------------------------------------------------------------
void TerrainPainter::traceRivers()
{
    int const steps = 6; // pieces between two centers
    m_rivers.clear();
    for (Terrain::River const& river: m_terrain.rivers())
    {
        std::vector<sf::Vector2f> const& p = river.points;
        std::vector<float> const& w = river.widths;
        std::size_t const n = p.size();
        std::vector<sf::Vector2f> points{ p.front() };
        std::vector<float> widths{ w.front() };
        for (std::size_t k = 0u; k + 1u < n; ++k)
        {
            // The curve from p[k] to p[k + 1] bends toward the points before
            // and after (the ends are repeated at the extremities).
            sf::Vector2f const p0 = p[k > 0u ? k - 1u : k];
            sf::Vector2f const p1 = p[k];
            sf::Vector2f const p2 = p[k + 1u];
            sf::Vector2f const p3 = p[k + 2u < n ? k + 2u : k + 1u];
            for (int i = 1; i <= steps; ++i)
            {
                float const t = float(i) / float(steps);
                float const t2 = t * t;
                float const t3 = t2 * t;
                points.push_back(0.5f * (2.0f * p1 + (p2 - p0) * t +
                                         (2.0f * p0 - 5.0f * p1 + 4.0f * p2 - p3) * t2 +
                                         (3.0f * p1 - p0 - 3.0f * p2 + p3) * t3));
                widths.push_back(w[k] + (w[k + 1u] - w[k]) * t);
            }
        }
        for (std::size_t k = 0u; k + 1u < points.size(); ++k)
        {
            float const width = 0.5f * (widths[k] + widths[k + 1u]);
            mesh::band(m_rivers, points[k], points[k + 1u], width, RIVER);
            mesh::disc(m_rivers, points[k + 1u], width / 2.0f, RIVER);
        }
        // A round spring on the node.
        mesh::disc(m_rivers, points.front(), widths.front() / 2.0f, RIVER);
    }
}

//------------------------------------------------------------------------------
void TerrainPainter::draw(sf::RenderTarget& target) const
{
    if (!m_terrain.ready())
        return;
    target.draw(m_sprite);
    target.draw(m_rivers);
}
