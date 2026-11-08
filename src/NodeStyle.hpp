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

#ifndef NODE_STYLE_HPP
#  define NODE_STYLE_HPP

#  include "Bookmarks.hpp"
#  include <SFML/System/String.hpp>

//-----------------------------------------------------------------------------
//! \brief Size of the disc of a node on the screen, in pixels: big for an
//! island, medium for a community or a folder, small for a bookmark. The size
//! stays the same whatever the zoom, so the map never gets cluttered.
//-----------------------------------------------------------------------------
float nodeRadius(BookmarkNode const& node);

//-----------------------------------------------------------------------------
//! \brief What the node is, in capital letters: "ISLAND", "COMMUNITY",
//! "FOLDER" or "BOOKMARK". A plain Firefox export has no similarity links:
//! its groups are only folders, not islands nor communities.
//-----------------------------------------------------------------------------
sf::String nodeKind(Bookmarks const& bookmarks, size_t node);

//-----------------------------------------------------------------------------
//! \brief Same as nodeKind() but written "Island", "Community"...
//-----------------------------------------------------------------------------
sf::String nodeKindName(Bookmarks const& bookmarks, size_t node);

#endif
