/* *****************************************************************************
** MIT License
**
** Copyright (c) 2022 Quentin Quadrat
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

#ifndef BOOKMARKS_HPP
#  define BOOKMARKS_HPP

#  include <SFML/System/Vector2.hpp>
#  include <string>
#  include <cstddef>
#  include <iostream>

// *****************************************************************************
//! \brief Structure holding Firefox bookmark folder. See this document
//! explaining fields of the Firefox bookmark if you want to extand this
//! structure:
//! https://searchfox.org/mozilla-release/source/toolkit/components/places/Bookmarks.jsm
// *****************************************************************************
struct Folder
{
    //! \brief Folder name
    std::string title;
    //! \brief unique identifier (not the universal id)
    size_t id;
    //! \brief unique identifier of the parent (folder)
    size_t parent;
};

static inline
std::ostream& operator<<(std::ostream& os, const Folder& folder)
{
    os << "Folder " << folder.title << " (parent: " << folder.parent << ")\n";
    return os;
}

// *****************************************************************************
//! \brief Structure holding Firefox bookmark URL. See this document
//! explaining fields of the Firefox bookmark if you want to extand this
//! structure:
//! https://searchfox.org/mozilla-release/source/toolkit/components/places/Bookmarks.jsm
// *****************************************************************************
struct Bookmark
{
    //! \brief Name of the bookmark
    std::string title;
    //! \brief URL
    std::string uri;
    //! \brief unique identifier (not the universal id)
    size_t id;
    //! \brief unique identifier of the parent (folder)
    size_t parent;
};

static inline
std::ostream& operator<<(std::ostream& os, const Bookmark& bookmark)
{
    os << "Bookmark " << bookmark.title << ": " << bookmark.uri
       << " (parent: " << bookmark.parent << ")\n";
    return os;
}

#endif
