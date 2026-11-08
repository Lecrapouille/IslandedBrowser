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

#ifndef BOOKMARKS_HPP
#  define BOOKMARKS_HPP

#  include <cstddef>
#  include <string>
#  include <vector>

// *****************************************************************************
//! \brief One node of the bookmark tree: a folder (island, community or
//! Firefox folder) or a bookmark. Nodes are stored in a vector and refer to
//! each other by index, so nothing dangles when the vector grows.
// *****************************************************************************
struct BookmarkNode
{
    enum class Kind { Folder, Bookmark };

    //! \brief Sentinel for "no node".
    static constexpr size_t none = static_cast<size_t>(-1);

    Kind kind = Kind::Folder;
    //! \brief Folder name or bookmark title.
    std::string title;
    //! \brief URL of a bookmark, empty for a folder.
    std::string url;
    //! \brief Index of the parent node, none for the root.
    size_t parent = none;
    //! \brief Indices of the children nodes.
    std::vector<size_t> children;
    //! \brief Index of the island this node belongs to (a direct child of the
    //! root), none for the root itself.
    size_t island = none;
    //! \brief Distance to the root (root = 0, island = 1).
    size_t depth = 0;

    bool isBookmark() const { return kind == Kind::Bookmark; }
};

// *****************************************************************************
//! \brief Similarity between two bookmarks, computed by organize.py. Links
//! are not part of the tree: they are the transversal relations ("rivers")
//! between communities and islands.
// *****************************************************************************
struct BookmarkLink
{
    size_t from;
    size_t to;
    //! \brief Cosine similarity in ]0, 1].
    float weight;
};

// *****************************************************************************
//! \brief Bookmark collection: a tree rooted at node 0 plus similarity links.
// *****************************************************************************
struct Bookmarks
{
    //! \brief nodes[0] is the root. It is never displayed.
    std::vector<BookmarkNode> nodes;
    //! \brief Similarity links (empty for a plain Firefox export).
    std::vector<BookmarkLink> links;
    //! \brief Indices of the islands (the children of the root).
    std::vector<size_t> islands;
    //! \brief Number of bookmark nodes.
    size_t bookmark_count = 0;

    //! \brief Load a Firefox backup (bookmarks.json) or an organization
    //! written by bookmarks/organize.py (organization.json).
    //! \throw std::runtime_error on unreadable or empty files.
    static Bookmarks load(std::string const& path);
};

#endif
