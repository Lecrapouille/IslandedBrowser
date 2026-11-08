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

#include "Noise.hpp"
#include "Maths.hpp"
#include <algorithm>
#include <cmath>

namespace noise {

//------------------------------------------------------------------------------
float smoothstep(float from, float to, float x)
{
    float const t = clamp((x - from) / (to - from), 0.0f, 1.0f);
    return t * t * (3.0f - 2.0f * t);
}

//------------------------------------------------------------------------------
float lattice(int x, int y, std::uint32_t seed)
{
    // Mix the bits of the coordinates and of the seed (a hash): close
    // points get unrelated numbers.
    std::uint32_t h = (static_cast<std::uint32_t>(x) * 0x8da6b343u) ^
                      (static_cast<std::uint32_t>(y) * 0xd8163841u) ^
                      (seed * 0xcb1ab31fu);
    h ^= h >> 13u;
    h *= 0x5bd1e995u;
    h ^= h >> 15u;
    return float(h & 0xffffffu) / float(0xffffffu);
}

//------------------------------------------------------------------------------
float value(float x, float y, std::uint32_t seed)
{
    float const fx = std::floor(x);
    float const fy = std::floor(y);
    int const ix = int(fx);
    int const iy = int(fy);
    float const tx = x - fx;
    float const ty = y - fy;
    // Perlin's quintic curve: no visible crease at the grid lines.
    float const sx = tx * tx * tx * (tx * (tx * 6.0f - 15.0f) + 10.0f);
    float const sy = ty * ty * ty * (ty * (ty * 6.0f - 15.0f) + 10.0f);
    float const a = lattice(ix, iy, seed);
    float const b = lattice(ix + 1, iy, seed);
    float const c = lattice(ix, iy + 1, seed);
    float const d = lattice(ix + 1, iy + 1, seed);
    return a + (b - a) * sx + (c - a) * sy + (a - b - c + d) * sx * sy;
}

//------------------------------------------------------------------------------
float fbm(float x, float y, std::uint32_t seed, int octaves)
{
    float sum = 0.0f;
    float amplitude = 0.5f;
    float total = 0.0f;
    for (int i = 0; i < octaves; ++i)
    {
        sum += amplitude * value(x, y, seed + std::uint32_t(i));
        total += amplitude;
        x *= 2.03f;
        y *= 2.03f;
        amplitude *= 0.5f;
    }
    return sum / total;
}

} // namespace noise
