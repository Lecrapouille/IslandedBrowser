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

#ifndef LAYOUT_EXPORT_HPP
#  define LAYOUT_EXPORT_HPP

#  include "Bookmarks.hpp"
#  include "ForceDirectedGraph.hpp"
#  include <string>

//------------------------------------------------------------------------------
//! \brief Write the converged layout for other programs (the 10e_Archipelago
//! demo of Compages builds its islands around it).
//!
//! The file is an object { "format": "islanded-browser-layout", "version": 1,
//! "source", "nodes", "links" }. Each node is { "id", "parent" (-1 for an
//! island), "kind" ("folder" or "bookmark"), "title", "url", "x", "y" }; ids
//! are the positions in "nodes", parents come before their children. Each
//! link is [id, id, similarity].
//! \throw std::runtime_error when the file cannot be written.
//------------------------------------------------------------------------------
void writeLayout(std::string const& path, std::string const& source,
                 Bookmarks const& bookmarks, ForceDirectedGraph const& graph);

#endif
