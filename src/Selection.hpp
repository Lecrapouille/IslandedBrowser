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

#ifndef SELECTION_HPP
#  define SELECTION_HPP

#  include "IslandedBrowser.hpp"
#  include <utility>
#  include <vector>

// ****************************************************************************
//! \brief Remembers which node the user points at and which one is selected,
//! and decides which part of the map stays bright while the rest fades into
//! the sea.
//!
//! What stays bright, by order of priority:
//! 1. the node under the mouse (on the map or in the details panel) with its
//!    whole community and the folders above it;
//! 2. else the results of the search, with what is inside and above them;
//! 3. else the selected node with its community.
//! When none of them exist, the whole map is bright.
// ****************************************************************************
class Selection
{
public:

    explicit Selection(IslandedBrowser const& island);

    //-------------------------------------------------------------------------
    //! \brief Select a node (BookmarkNode::none to unselect). For a bookmark,
    //! its most similar bookmarks are looked up for the details panel.
    //-------------------------------------------------------------------------
    void select(size_t node);

    //-------------------------------------------------------------------------
    //! \brief Nodes currently under the mouse: one on the map, one in the
    //! details panel (BookmarkNode::none when there is none).
    //-------------------------------------------------------------------------
    void hover(size_t on_map, size_t in_panel);

    //-------------------------------------------------------------------------
    //! \brief Recompute what stays bright (see the class description).
    //! \param[in] matches results of the search, empty when not searching.
    //-------------------------------------------------------------------------
    void updateFocus(std::vector<size_t> const& matches);

    //-------------------------------------------------------------------------
    //! \brief True when the node is out of the focus and must fade.
    //-------------------------------------------------------------------------
    bool dimmed(size_t node) const { return m_has_focus && !m_bright[node]; }
    //! \brief True when a part of the map is highlighted.
    bool hasFocus() const { return m_has_focus; }

    size_t selected() const { return m_selected; }
    size_t hovered() const { return m_hovered; }
    size_t hoveredRow() const { return m_hovered_row; }
    //! \brief Bookmarks similar to the selected one, with their similarity
    //! (0 to 1), the most similar first.
    std::vector<std::pair<size_t, float>> const& related() const { return m_related; }

private:

    //-------------------------------------------------------------------------
    //! \brief Keep bright the node, everything inside it and the folders
    //! leading to it.
    //-------------------------------------------------------------------------
    void brighten(size_t node);

private:

    IslandedBrowser const& m_island;
    size_t m_hovered = BookmarkNode::none;
    size_t m_hovered_row = BookmarkNode::none;
    size_t m_selected = BookmarkNode::none;
    std::vector<std::pair<size_t, float>> m_related;
    //! \brief One flag per node: 1 when it stays bright.
    std::vector<char> m_bright;
    bool m_has_focus = false;
};

#endif
