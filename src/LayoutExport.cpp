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

#include "LayoutExport.hpp"
#include <cstdio>
#include <fstream>
#include <stdexcept>

namespace {

//------------------------------------------------------------------------------
//! \brief A JSON string, quotes included.
//------------------------------------------------------------------------------
std::string quoted(std::string const& text)
{
    std::string out = "\"";
    for (char const c: text)
    {
        switch (c)
        {
        case '"': out += "\\\""; break;
        case '\\': out += "\\\\"; break;
        case '\n': out += "\\n"; break;
        case '\r': out += "\\r"; break;
        case '\t': out += "\\t"; break;
        default:
            if (static_cast<unsigned char>(c) < 0x20u)
            {
                char buffer[8];
                std::snprintf(buffer, sizeof(buffer), "\\u%04x", unsigned(c));
                out += buffer;
            }
            else
                out += c;
        }
    }
    return out + "\"";
}

} // namespace

//------------------------------------------------------------------------------
void writeLayout(std::string const& path, std::string const& source,
                 Bookmarks const& bookmarks, ForceDirectedGraph const& graph)
{
    std::ofstream file(path);
    if (!file)
        throw std::runtime_error("cannot write " + path);

    // The vertices are made in the order of the nodes (root excluded), and a
    // parent always comes before its children: the vertex index is the id.
    file << "{\n  \"format\": \"islanded-browser-layout\",\n  \"version\": 1,\n"
         << "  \"source\": " << quoted(source) << ",\n  \"nodes\": [\n";
    auto const& vertices = graph.vertices();
    for (size_t i = 0u; i < vertices.size(); ++i)
    {
        BookmarkNode const& node = bookmarks.nodes[vertices[i].node];
        size_t const parent = graph.vertexOf(node.parent);
        file << "    {\"id\": " << i
             << ", \"parent\": " << (parent == BookmarkNode::none ? std::string("-1") : std::to_string(parent))
             << ", \"kind\": \"" << (node.isBookmark() ? "bookmark" : "folder") << "\""
             << ", \"title\": " << quoted(node.title)
             << ", \"url\": " << quoted(node.url)
             << ", \"x\": " << vertices[i].position.x
             << ", \"y\": " << vertices[i].position.y << "}"
             << (i + 1u < vertices.size() ? ",\n" : "\n");
    }
    file << "  ],\n  \"links\": [";
    bool first = true;
    for (BookmarkLink const& link: bookmarks.links)
    {
        size_t const a = graph.vertexOf(link.from);
        size_t const b = graph.vertexOf(link.to);
        if (a == BookmarkNode::none || b == BookmarkNode::none)
            continue;
        file << (first ? "\n    " : ",\n    ") << "[" << a << ", " << b << ", " << link.weight << "]";
        first = false;
    }
    file << "\n  ]\n}\n";
    if (!file)
        throw std::runtime_error("cannot write " + path);
}
