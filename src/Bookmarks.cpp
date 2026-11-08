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

#include "Bookmarks.hpp"
#include "Json.hpp"
#include <map>
#include <stdexcept>

// C++14 needs a definition for an odr-used static constexpr member.
constexpr size_t BookmarkNode::none;

namespace {

// -----------------------------------------------------------------------------
size_t addNode(Bookmarks& b, BookmarkNode::Kind kind, std::string const& title,
               std::string const& url, size_t parent)
{
    BookmarkNode node;
    node.kind = kind;
    node.title = title;
    node.url = url;
    node.parent = parent;
    b.nodes.push_back(node);
    size_t const index = b.nodes.size() - 1u;
    if (parent != BookmarkNode::none)
        b.nodes[parent].children.push_back(index);
    if (kind == BookmarkNode::Kind::Bookmark)
        ++b.bookmark_count;
    return index;
}

// -----------------------------------------------------------------------------
bool isFolder(Json const& node)
{
    Json const* children = node.get("children");
    return children != nullptr && children->isArray();
}

// -----------------------------------------------------------------------------
//! \brief Firefox backup: folders have "children", bookmarks have "uri".
//! The tags root is skipped (its entries duplicate real bookmarks),
//! separators and place: queries too.
// -----------------------------------------------------------------------------
void loadFirefoxNode(Json const& json, size_t parent, Bookmarks& b)
{
    if (json.kind != Json::Object)
        return;
    std::string const root = textOf(json.get("root"));
    if (root == "tagsFolder" || textOf(json.get("guid")) == "tags________")
        return;
    std::string const title = textOf(json.get("title"));
    if (isFolder(json))
    {
        size_t const folder = addNode(b, BookmarkNode::Kind::Folder, title, "", parent);
        for (Json const& child: json.get("children")->array)
            loadFirefoxNode(child, folder, b);
        return;
    }
    std::string const url = textOf(json.get("uri"));
    if (url.empty() || url.compare(0, 6, "place:") == 0)
        return;
    addNode(b, BookmarkNode::Kind::Bookmark, title.empty() ? url : title, url, parent);
}

// -----------------------------------------------------------------------------
void loadFirefox(Json const& document, Bookmarks& b)
{
    if (!isFolder(document))
        throw std::runtime_error("not a Firefox bookmark backup (no root children)");
    addNode(b, BookmarkNode::Kind::Folder, "", "", BookmarkNode::none);
    for (Json const& child: document.get("children")->array)
    {
        // Roots without any bookmark (often "mobile" or "unfiled") are not
        // drawn: an empty island only adds noise.
        size_t const nodes = b.nodes.size();
        size_t const bookmarks = b.bookmark_count;
        loadFirefoxNode(child, 0u, b);
        if (b.bookmark_count == bookmarks && b.nodes.size() > nodes)
        {
            b.nodes.resize(nodes);
            b.nodes[0].children.pop_back();
        }
    }
}

// -----------------------------------------------------------------------------
//! \brief organization.json: communities form the folder tree (islands have
//! no parent), each bookmark hangs from its "community", "links" holds
//! [from, to, weight] triplets of bookmark ids.
// -----------------------------------------------------------------------------
void loadOrganization(Json const& document, Bookmarks& b)
{
    Json const* communities = document.get("communities");
    Json const* items = document.get("bookmarks");
    if (communities == nullptr || !communities->isArray() ||
        items == nullptr || !items->isArray())
    {
        throw std::runtime_error("organization without communities or bookmarks");
    }

    addNode(b, BookmarkNode::Kind::Folder, "", "", BookmarkNode::none);

    // Create the folders first, attach them once all are known: a parent may
    // be listed after its children, and a hand edit may have broken the tree.
    std::map<std::string, size_t> folder_of;
    std::vector<std::string> parent_names;
    for (Json const& community: communities->array)
    {
        std::string const id = textOf(community.get("id"));
        if (id.empty() || folder_of.count(id))
            continue;
        std::string name = textOf(community.get("name"));
        if (name.empty())
            name = id;
        BookmarkNode node;
        node.title = name;
        b.nodes.push_back(node);
        folder_of[id] = b.nodes.size() - 1u;
        parent_names.push_back(textOf(community.get("parent")));
    }
    for (size_t i = 0u; i < parent_names.size(); ++i)
    {
        size_t const node = i + 1u;
        size_t parent = 0u;
        auto const found = folder_of.find(parent_names[i]);
        if (found != folder_of.end())
        {
            // Refuse cycles: the parent must not descend from this node.
            size_t ancestor = found->second;
            size_t guard = 0u;
            while (ancestor != 0u && ancestor != node && ancestor != BookmarkNode::none &&
                   guard++ < b.nodes.size())
            {
                ancestor = b.nodes[ancestor].parent;
            }
            if (ancestor != node)
                parent = found->second;
        }
        b.nodes[node].parent = parent;
        b.nodes[parent].children.push_back(node);
    }

    std::map<long long, size_t> bookmark_of;
    for (Json const& item: items->array)
    {
        std::string const url = textOf(item.get("url"));
        Json const* id = item.get("id");
        if (url.empty() || id == nullptr || !id->integral)
            continue;
        size_t parent = 0u;
        auto const found = folder_of.find(textOf(item.get("community")));
        if (found != folder_of.end())
            parent = found->second;
        std::string const title = textOf(item.get("title"));
        bookmark_of[id->integer] =
            addNode(b, BookmarkNode::Kind::Bookmark, title.empty() ? url : title, url, parent);
    }

    Json const* links = document.get("links");
    if (links == nullptr || !links->isArray())
        return;
    for (Json const& link: links->array)
    {
        if (!link.isArray() || link.array.size() < 3u)
            continue;
        auto const from = bookmark_of.find(link.array[0].integer);
        auto const to = bookmark_of.find(link.array[1].integer);
        float const weight = static_cast<float>(link.array[2].real);
        if (from == bookmark_of.end() || to == bookmark_of.end() ||
            from->second == to->second || !(weight > 0.0f))
        {
            continue;
        }
        b.links.push_back({ from->second, to->second, std::min(weight, 1.0f) });
    }
}

// -----------------------------------------------------------------------------
//! \brief Fill depth and island of every node, iteratively (no recursion).
// -----------------------------------------------------------------------------
void computeIslands(Bookmarks& b)
{
    std::vector<size_t> stack{ 0u };
    while (!stack.empty())
    {
        size_t const index = stack.back();
        stack.pop_back();
        for (size_t const child: b.nodes[index].children)
        {
            BookmarkNode& node = b.nodes[child];
            node.depth = b.nodes[index].depth + 1u;
            node.island = (index == 0u) ? child : b.nodes[index].island;
            stack.push_back(child);
        }
    }
    b.islands = b.nodes[0].children;
}

} // namespace

// -----------------------------------------------------------------------------
Bookmarks Bookmarks::load(std::string const& path)
{
    Json const document = Json::load(path);
    Bookmarks b;
    if (textOf(document.get("format")) == "islanded-browser-organization")
        loadOrganization(document, b);
    else if (document.get("communities") != nullptr)
        throw std::runtime_error(path + ": organization written by an older script, "
                                 "run bookmarks/organize.py again");
    else
        loadFirefox(document, b);
    if (b.bookmark_count == 0u)
        throw std::runtime_error(path + ": no bookmark found");
    computeIslands(b);
    return b;
}
