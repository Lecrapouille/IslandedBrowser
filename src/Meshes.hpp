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

#ifndef MESHES_HPP
#  define MESHES_HPP

#  include <SFML/Graphics.hpp>

// ****************************************************************************
//! \brief Building blocks of the map: lines, bands, discs and rings.
//!
//! A graphics card draws triangles very fast, but each separate drawing
//! order is slow. So instead of drawing the 2500 discs one by one, each
//! function below only adds its triangles to a big list (an SFML vertex
//! array), and the whole list is drawn at once.
// ****************************************************************************
namespace mesh {

//! \brief Add a one pixel line from a to b.
void line(sf::VertexArray& lines, sf::Vector2f a, sf::Vector2f b, sf::Color color);

//! \brief Add a band from a to b, as wide as asked (two triangles forming
//! a long thin rectangle). Used for the sea routes and the rivers.
void band(sf::VertexArray& triangles, sf::Vector2f a, sf::Vector2f b,
          float width, sf::Color color);

//! \brief Add a filled disc: a few triangles around the center, like the
//! slices of a pizza.
void disc(sf::VertexArray& triangles, sf::Vector2f center, float radius,
          sf::Color color);

//! \brief Add a ring (a disc with a hole) between two radii. Used to circle
//! the selected node and the search results.
void ring(sf::VertexArray& triangles, sf::Vector2f center, float inner,
          float outer, sf::Color color);

} // namespace mesh

#endif
