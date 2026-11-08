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

#ifndef STATUS_BAR_HPP
#  define STATUS_BAR_HPP

#  include "IslandedBrowser.hpp"
#  include "TextPainter.hpp"
#  include <string>

// ****************************************************************************
//! \brief The thin bar at the bottom of the window. On the left: the file
//! shown, how many bookmarks and islands it has, and whether the map is
//! still moving. On the right: the keys that are useful right now.
// ****************************************************************************
class StatusBar
{
public:

    //-------------------------------------------------------------------------
    //! \brief What the user is doing: it decides which keys are recalled.
    //-------------------------------------------------------------------------
    enum class Mode { Browsing, Selected, Searching, Help };

public:

    StatusBar(TextPainter& painter, IslandedBrowser const& island,
              std::string const& file_name);

    //-------------------------------------------------------------------------
    //! \param[in] paused true when the user stopped the layout (Space).
    //-------------------------------------------------------------------------
    void draw(Mode mode, bool paused);

private:

    TextPainter& m_painter;
    IslandedBrowser const& m_island;
    std::string m_file_name;
};

#endif
