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

#ifndef NOISE_HPP
#  define NOISE_HPP

#  include <cstdint>

// ****************************************************************************
//! \brief Smooth random values, to make coasts wave and relief bumpy.
//!
//! Pure randomness looks like television snow. Nature is "smoothly random":
//! two close points have close values. Value noise gives a random number at
//! each corner of a grid and blends smoothly between them. Fractal noise
//! (fbm) adds several layers of it, each one twice as detailed and half as
//! strong, like the large bays, small coves and pebbles of a real coast.
//! The same seed always gives the same values: the map is reproducible.
// ****************************************************************************
namespace noise {

//-----------------------------------------------------------------------------
//! \brief 0 below from, 1 above to, and a soft S-shaped curve in between.
//-----------------------------------------------------------------------------
float smoothstep(float from, float to, float x);

//-----------------------------------------------------------------------------
//! \brief A random number in [0, 1] attached to the integer point (x, y):
//! always the same for the same point and seed.
//-----------------------------------------------------------------------------
float lattice(int x, int y, std::uint32_t seed);

//-----------------------------------------------------------------------------
//! \brief Value noise in [0, 1]: smooth, about one bump per unit.
//-----------------------------------------------------------------------------
float value(float x, float y, std::uint32_t seed);

//-----------------------------------------------------------------------------
//! \brief Fractal noise in [0, 1]: several layers of value noise.
//-----------------------------------------------------------------------------
float fbm(float x, float y, std::uint32_t seed, int octaves);

} // namespace noise

#endif
