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

#ifndef VORONOI_GRID_HPP
#  define VORONOI_GRID_HPP

#  include <SFML/System/Vector2.hpp>
#  include <array>
#  include <cstddef>
#  include <cstdint>
#  include <vector>

// ****************************************************************************
//! \brief The map cut into cells, like the plots of a land register.
//!
//! A Voronoi diagram is made of "sites" (points); the cell of a site is the
//! area closer to it than to any other site. With sites thrown at random,
//! the cells look natural, unlike the squares of a chessboard.
//!
//! Here there is one site per square of a grid, moved randomly a little
//! from the center of its square. The site closest to any point is then
//! always among the 3 x 3 squares around it: the cell of a point is found
//! with nine distances, without building the diagram.
//!
//! Lengths are in metres of the map (see Terrain), the origin at the top
//! left corner.
// ****************************************************************************
class VoronoiGrid
{
public:

    //-------------------------------------------------------------------------
    //! \brief The cells around a cell (up to 8): for (int n: grid.neighbours(c)).
    //-------------------------------------------------------------------------
    struct Around
    {
        std::array<int, 8> cells{};
        std::size_t size = 0u;

        int const* begin() const { return cells.data(); }
        int const* end() const { return cells.data() + size; }
    };

    //-------------------------------------------------------------------------
    //! \brief The cell holding a point, and the closest border of that cell:
    //! the cell on the other side of it and how far it is (metres).
    //-------------------------------------------------------------------------
    struct Hit
    {
        int cell = 0;
        int across = -1;
        float edge = 1.0e9f;
    };

public:

    VoronoiGrid() = default;

    //-------------------------------------------------------------------------
    //! \param[in] columns, rows number of squares of the grid.
    //! \param[in] cell side of a square (metres).
    //! \param[in] seed the same seed gives the same cells.
    //-------------------------------------------------------------------------
    VoronoiGrid(int columns, int rows, float cell, std::uint32_t seed);

    std::size_t count() const { return m_sites.size(); }
    int columns() const { return m_columns; }
    int rows() const { return m_rows; }
    float cellSize() const { return m_cell; }
    sf::Vector2f site(int cell) const { return m_sites[std::size_t(cell)]; }
    int cellAt(int i, int j) const { return j * m_columns + i; }
    int column(int cell) const { return cell % m_columns; }
    int row(int cell) const { return cell / m_columns; }

    //! \brief True for a cell on the border of the grid.
    bool onBorder(int cell) const;

    Around neighbours(int cell) const;

    //-------------------------------------------------------------------------
    //! \brief The cell holding a point (its closest site), and its closest
    //! border.
    //-------------------------------------------------------------------------
    Hit hit(sf::Vector2f point) const;

    //-------------------------------------------------------------------------
    //! \brief Distance from a point to the border between two cells (the
    //! line halfway between their sites); positive on the side of own.
    //-------------------------------------------------------------------------
    static float bisector(sf::Vector2f own, sf::Vector2f other, sf::Vector2f point);

private:

    //! \brief Column (or row) of the square holding a coordinate.
    int squareOf(float coordinate, int count) const;

private:

    int m_columns = 0;
    int m_rows = 0;
    float m_cell = 1.0f;
    std::vector<sf::Vector2f> m_sites;
};

#endif
