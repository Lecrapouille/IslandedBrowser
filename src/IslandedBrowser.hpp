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

#ifndef ISLANDEDBROWSER_HPP
#  define ISLANDEDBROWSER_HPP

#  include "Bookmarks.hpp"
#  include "ForceDirectedGraph.hpp"
#  include <string>
#  include <utility>
#  include <vector>

// *****************************************************************************
//! \brief Context of the application, independent of the rendering: the
//! bookmarks, their layout and the actions on them.
// *****************************************************************************
class IslandedBrowser
{
public:

    //----------------------------------------------------------------------
    //! \brief Load the bookmark file and compute the initial layout.
    //! \param[in] path organization.json (bookmarks/organize.py) or a
    //! Firefox backup (bookmarks/firefox_export.py).
    //! \throw std::runtime_error when the file cannot be used.
    //----------------------------------------------------------------------
    explicit IslandedBrowser(std::string const& path);

    Bookmarks const& bookmarks() const { return m_bookmarks; }
    ForceDirectedGraph& layout() { return m_layout; }
    ForceDirectedGraph const& layout() const { return m_layout; }

    //----------------------------------------------------------------------
    //! \brief Node whose vertex is the closest to a world position.
    //! \param[in] radius maximum distance, in world units.
    //! \return BookmarkNode::none if no vertex is close enough.
    //----------------------------------------------------------------------
    size_t pick(sf::Vector2f const& position, float radius) const;

    //----------------------------------------------------------------------
    //! \brief URLs opened by a click: the bookmark itself, or the bookmarks
    //! of a folder (direct children first, then deeper), at most max_count.
    //! \param[out] total number of bookmarks under the node.
    //----------------------------------------------------------------------
    std::vector<std::string> urls(size_t node, size_t max_count, size_t& total) const;

    //----------------------------------------------------------------------
    //! \brief Number of bookmarks under the node (1 for a bookmark).
    //----------------------------------------------------------------------
    size_t bookmarkCount(size_t node) const;

    //----------------------------------------------------------------------
    //! \brief The node and all its descendants.
    //----------------------------------------------------------------------
    std::vector<size_t> subtree(size_t node) const;

    //----------------------------------------------------------------------
    //! \brief Folders from the island down to the parent of the node (the
    //! root is not included).
    //----------------------------------------------------------------------
    std::vector<size_t> ancestors(size_t node) const;

    //----------------------------------------------------------------------
    //! \brief Nodes whose title or URL contains the text (case insensitive
    //! for ASCII letters). Folders come first, then bookmarks.
    //----------------------------------------------------------------------
    std::vector<size_t> search(std::string const& text) const;

    //----------------------------------------------------------------------
    //! \brief Bookmarks the most similar to a bookmark (similarity links
    //! of organize.py), strongest first, at most max_count.
    //----------------------------------------------------------------------
    std::vector<std::pair<size_t, float>> related(size_t node, size_t max_count) const;

    //----------------------------------------------------------------------
    //! \brief Start the web browser on the given URLs, without a shell (an
    //! URL cannot inject a command). Return false on failure.
    //----------------------------------------------------------------------
    static bool openInBrowser(std::string const& browser,
                              std::vector<std::string> const& urls);

private:

    Bookmarks m_bookmarks;
    ForceDirectedGraph m_layout;
};

#endif
