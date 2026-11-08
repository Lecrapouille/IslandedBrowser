// 2021 -- 2022 Quentin Quadrat quentin.quadrat@gmail.com
//
// This is free and unencumbered software released into the public domain.
//
// Anyone is free to copy, modify, publish, use, compile, sell, or
// distribute this software, either in source code form or as a compiled
// binary, for any purpose, commercial or non-commercial, and by any
// means.
//
// In jurisdictions that recognize copyright laws, the author or authors
// of this software dedicate any and all copyright interest in the
// software to the public domain. We make this dedication for the benefit
// of the public at large and to the detriment of our heirs and
// successors. We intend this dedication to be an overt act of
// relinquishment in perpetuity of all present and future rights to this
// software under copyright law.
//
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
// EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
// MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
// IN NO EVENT SHALL THE AUTHORS BE LIABLE FOR ANY CLAIM, DAMAGES OR
// OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE,
// ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR
// OTHER DEALINGS IN THE SOFTWARE.

#include "IslandedBrowserGUI.hpp"
#include <algorithm>
#include <cmath>
#include <iostream>
#include <map>
#include <sstream>

namespace {

//------------------------------------------------------------------------------
//! \brief Well separated hues (golden ratio), one per island.
//------------------------------------------------------------------------------
sf::Color hue(size_t index)
{
    float const h = std::fmod(0.11f + 0.618034f * float(index), 1.0f) * 6.0f;
    float const s = 0.65f;
    float const v = 0.95f;
    float const c = v * s;
    float const x = c * (1.0f - std::fabs(std::fmod(h, 2.0f) - 1.0f));
    float r = 0, g = 0, b = 0;
    switch (int(h))
    {
    case 0: r = c; g = x; break;
    case 1: r = x; g = c; break;
    case 2: g = c; b = x; break;
    case 3: g = x; b = c; break;
    case 4: r = x; b = c; break;
    default: r = c; b = x; break;
    }
    float const m = v - c;
    return sf::Color(sf::Uint8((r + m) * 255), sf::Uint8((g + m) * 255),
                     sf::Uint8((b + m) * 255));
}

//------------------------------------------------------------------------------
//! \brief Linear mix of two colors, t = 0 gives a, t = 1 gives b.
//------------------------------------------------------------------------------
sf::Color blend(sf::Color a, sf::Color b, float t)
{
    auto mix = [t](sf::Uint8 x, sf::Uint8 y) {
        return sf::Uint8(float(x) + (float(y) - float(x)) * t);
    };
    return sf::Color(mix(a.r, b.r), mix(a.g, b.g), mix(a.b, b.b), a.a);
}

//------------------------------------------------------------------------------
sf::String utf8(std::string const& s)
{
    return sf::String::fromUtf8(s.begin(), s.end());
}

//------------------------------------------------------------------------------
std::string toUtf8(sf::String const& s)
{
    auto const bytes = s.toUtf8();
    return std::string(bytes.begin(), bytes.end());
}

//------------------------------------------------------------------------------
void addLine(sf::VertexArray& lines, sf::Vector2f a, sf::Vector2f b, sf::Color color)
{
    lines.append(sf::Vertex(a, color));
    lines.append(sf::Vertex(b, color));
}

//------------------------------------------------------------------------------
//! \brief Segment of the given width (in world units) made of two triangles.
//------------------------------------------------------------------------------
void addThickLine(sf::VertexArray& triangles, sf::Vector2f a, sf::Vector2f b,
                  float width, sf::Color color)
{
    sf::Vector2f const direction = b - a;
    float const length = std::sqrt(direction.x * direction.x + direction.y * direction.y);
    if (length <= 0.0f)
        return;
    sf::Vector2f const normal(-direction.y / length * width / 2.0f,
                              direction.x / length * width / 2.0f);
    sf::Vertex const corners[4] = {
        sf::Vertex(a + normal, color), sf::Vertex(b + normal, color),
        sf::Vertex(b - normal, color), sf::Vertex(a - normal, color)
    };
    for (int i: { 0, 1, 2, 0, 2, 3 })
        triangles.append(corners[i]);
}

//------------------------------------------------------------------------------
//! \brief Disc made of triangles (one draw call for all the nodes).
//------------------------------------------------------------------------------
void addDisc(sf::VertexArray& triangles, sf::Vector2f center, float radius, sf::Color color)
{
    int const sides = CIRCLE_COUNT_POINTS;
    sf::Vector2f previous(center.x + radius, center.y);
    for (int i = 1; i <= sides; ++i)
    {
        float const angle = 6.2831853f * float(i) / float(sides);
        sf::Vector2f const next(center.x + radius * std::cos(angle),
                                center.y + radius * std::sin(angle));
        triangles.append(sf::Vertex(center, color));
        triangles.append(sf::Vertex(previous, color));
        triangles.append(sf::Vertex(next, color));
        previous = next;
    }
}

//------------------------------------------------------------------------------
//! \brief Ring between two radii, made of triangles.
//------------------------------------------------------------------------------
void addRing(sf::VertexArray& triangles, sf::Vector2f center, float inner,
             float outer, sf::Color color)
{
    int const sides = 24;
    for (int i = 0; i < sides; ++i)
    {
        float const a0 = 6.2831853f * float(i) / float(sides);
        float const a1 = 6.2831853f * float(i + 1) / float(sides);
        sf::Vector2f const u0(std::cos(a0), std::sin(a0));
        sf::Vector2f const u1(std::cos(a1), std::sin(a1));
        sf::Vertex const corners[4] = {
            sf::Vertex(center + u0 * inner, color), sf::Vertex(center + u0 * outer, color),
            sf::Vertex(center + u1 * outer, color), sf::Vertex(center + u1 * inner, color)
        };
        for (int k: { 0, 1, 2, 0, 2, 3 })
            triangles.append(corners[k]);
    }
}

//------------------------------------------------------------------------------
float radiusOf(BookmarkNode const& node)
{
    if (node.isBookmark())
        return BOOKMARK_RADIUS;
    return (node.depth <= 1u) ? ISLAND_RADIUS : FOLDER_RADIUS;
}

//------------------------------------------------------------------------------
float lineHeight(unsigned size)
{
    return std::round(float(size) * 1.35f);
}

} // namespace

//------------------------------------------------------------------------------
IslandedBrowserGUI::IslandedBrowserGUI(Application& application, const char* name,
                                       std::string const& bookmarks_path,
                                       std::string const& screenshot)
    : Application::GUI(application, name, SEA_COLOR),
      m_island(bookmarks_path), m_screenshot(screenshot)
{
    Bookmarks const& bookmarks = m_island.bookmarks();
    std::cout << bookmarks_path << ": " << bookmarks.bookmark_count << " bookmarks, "
              << bookmarks.islands.size() << " islands, " << bookmarks.links.size()
              << " similarity links" << std::endl;

    size_t const slash = bookmarks_path.find_last_of('/');
    m_file_name = (slash == std::string::npos) ? bookmarks_path
                                               : bookmarks_path.substr(slash + 1u);
    for (size_t i = 0u; i < bookmarks.islands.size(); ++i)
        m_palette.push_back(hue(i));
    m_focus.assign(bookmarks.nodes.size(), 0);
    computeRivers();

    sf::Vector2f const size(m_renderer.getSize());
    m_interface = sf::View(sf::FloatRect(0.0f, 0.0f, size.x, size.y));
    m_world = m_interface;
    m_has_font = m_font.loadFromFile("data/font.ttf");
    if (m_has_font)
    {
        m_text.setFont(m_font);
    }
    else
    {
        std::cerr << "data/font.ttf not found: texts are not displayed" << std::endl;
    }
    m_has_cursors = m_arrow_cursor.loadFromSystem(sf::Cursor::Arrow) &&
                    m_hand_cursor.loadFromSystem(sf::Cursor::Hand) &&
                    m_move_cursor.loadFromSystem(sf::Cursor::SizeAll);
    fitView();
    if (m_screenshot.empty())
    {
        toast("Click a node to see its details, double-click to open it, "
              "/ to search, H for help");
    }
}

//------------------------------------------------------------------------------
//! Sum the similarity links between each pair of islands and keep the
//! strongest pairs: about two rivers per island.
//------------------------------------------------------------------------------
void IslandedBrowserGUI::computeRivers()
{
    Bookmarks const& bookmarks = m_island.bookmarks();
    ForceDirectedGraph const& layout = m_island.layout();
    std::map<std::pair<size_t, size_t>, float> sums;
    for (BookmarkLink const& link: bookmarks.links)
    {
        size_t a = bookmarks.nodes[link.from].island;
        size_t b = bookmarks.nodes[link.to].island;
        if (a == b)
            continue;
        if (a > b)
            std::swap(a, b);
        sums[std::make_pair(a, b)] += link.weight;
    }
    std::vector<River> rivers;
    for (auto const& it: sums)
    {
        size_t const a = layout.vertexOf(it.first.first);
        size_t const b = layout.vertexOf(it.first.second);
        if (a != BookmarkNode::none && b != BookmarkNode::none)
            rivers.push_back({ a, b, it.second });
    }
    std::sort(rivers.begin(), rivers.end(), [](River const& x, River const& y) {
        return x.strength > y.strength;
    });
    rivers.resize(std::min(rivers.size(), 2u * bookmarks.islands.size()));
    if (!rivers.empty())
    {
        float const strongest = rivers.front().strength;
        for (River& river: rivers)
            river.strength /= strongest;
    }
    m_rivers = rivers;
}

//------------------------------------------------------------------------------
void IslandedBrowserGUI::activate()
{
    m_renderer.setView(m_world);
}

// =============================================================================
// View
// =============================================================================

//------------------------------------------------------------------------------
float IslandedBrowserGUI::scale() const
{
    return m_world.getSize().x / float(m_renderer.getSize().x);
}

//------------------------------------------------------------------------------
//! Size of the view showing the box with a margin, with the aspect ratio of
//! the window.
//------------------------------------------------------------------------------
sf::Vector2f IslandedBrowserGUI::fitSize(sf::FloatRect box) const
{
    sf::Vector2f const window(m_renderer.getSize());
    float const margin = 0.05f * std::max(box.width, box.height) + 20.0f;
    float const scale = std::max((box.width + 2.0f * margin) / window.x,
                                 (box.height + 2.0f * margin) / window.y);
    return sf::Vector2f(window.x * scale, window.y * scale);
}

//------------------------------------------------------------------------------
void IslandedBrowserGUI::fitView()
{
    sf::FloatRect const box = m_island.layout().bounds();
    m_world.setCenter(box.left + box.width / 2.0f, box.top + box.height / 2.0f);
    m_world.setSize(fitSize(box));
    m_animating = false;
}

//------------------------------------------------------------------------------
void IslandedBrowserGUI::zoom(float factor, sf::Vector2i pixel)
{
    // Zoom around the mouse cursor.
    sf::Vector2f const before = m_renderer.mapPixelToCoords(pixel, m_world);
    m_world.zoom(factor);
    sf::Vector2f const after = m_renderer.mapPixelToCoords(pixel, m_world);
    m_world.move(before - after);
    m_auto_fit = false;
    m_animating = false;
}

//------------------------------------------------------------------------------
void IslandedBrowserGUI::pan(sf::Vector2f pixels)
{
    m_world.move(pixels * scale());
    m_auto_fit = false;
    m_animating = false;
}

//------------------------------------------------------------------------------
//! The selected node is shown in the middle of the part of the window left
//! visible by the panel.
//------------------------------------------------------------------------------
void IslandedBrowserGUI::centerOn(size_t node)
{
    size_t const vertex = m_island.layout().vertexOf(node);
    if (vertex == BookmarkNode::none)
        return;
    sf::Vector2f const size = m_animating ? m_target_size : m_world.getSize();
    float const panel = (m_selected != BookmarkNode::none) ? PANEL_WIDTH + 12.0f : 0.0f;
    float const s = size.x / float(m_renderer.getSize().x);
    m_target_center = m_island.layout().vertices()[vertex].position +
                      sf::Vector2f(panel / 2.0f * s, 0.0f);
    m_target_size = size;
    m_animating = true;
    m_auto_fit = false;
}

//------------------------------------------------------------------------------
void IslandedBrowserGUI::zoomOn(size_t node)
{
    ForceDirectedGraph const& layout = m_island.layout();
    // A bookmark is shown with its community.
    size_t root = node;
    BookmarkNode const& n = m_island.bookmarks().nodes[node];
    if (n.isBookmark() && n.parent != BookmarkNode::none && n.parent != 0u)
        root = n.parent;
    bool first = true;
    sf::FloatRect box;
    for (size_t i: m_island.subtree(root))
    {
        size_t const vertex = layout.vertexOf(i);
        if (vertex == BookmarkNode::none)
            continue;
        sf::Vector2f const p = layout.vertices()[vertex].position;
        if (first)
        {
            box = sf::FloatRect(p.x, p.y, 0.0f, 0.0f);
            first = false;
            continue;
        }
        float const right = std::max(box.left + box.width, p.x);
        float const bottom = std::max(box.top + box.height, p.y);
        box.left = std::min(box.left, p.x);
        box.top = std::min(box.top, p.y);
        box.width = right - box.left;
        box.height = bottom - box.top;
    }
    if (first)
        return;
    float const minimum = 4.0f * layout.params.ideal_distance;
    if (box.width < minimum)
    {
        box.left -= (minimum - box.width) / 2.0f;
        box.width = minimum;
    }
    if (box.height < minimum)
    {
        box.top -= (minimum - box.height) / 2.0f;
        box.height = minimum;
    }
    // Leave room for the panel on the right.
    sf::Vector2f const window(m_renderer.getSize());
    float const panel = (m_selected != BookmarkNode::none) ? PANEL_WIDTH + 12.0f : 0.0f;
    float const visible = std::max(window.x - panel, window.x / 2.0f);
    sf::FloatRect widened = box;
    widened.width = box.width * window.x / visible;
    m_target_size = fitSize(widened);
    float const s = m_target_size.x / window.x;
    m_target_center = sf::Vector2f(box.left + box.width / 2.0f + panel / 2.0f * s,
                                   box.top + box.height / 2.0f);
    m_animating = true;
    m_auto_fit = false;
}

//------------------------------------------------------------------------------
bool IslandedBrowserGUI::onPanel(sf::Vector2i pixel) const
{
    sf::Vector2f const p(pixel);
    if (p.y >= float(m_renderer.getSize().y) - STATUSBAR_HEIGHT)
        return true;
    if (m_selected != BookmarkNode::none && m_panel_box.contains(p))
        return true;
    return m_searching && m_search_box.contains(p);
}

//------------------------------------------------------------------------------
size_t IslandedBrowserGUI::pickAt(sf::Vector2i pixel) const
{
    sf::Vector2u const window = m_renderer.getSize();
    if (m_show_help || pixel.x < 0 || pixel.y < 0 || pixel.x >= int(window.x) ||
        pixel.y >= int(window.y) || onPanel(pixel))
        return BookmarkNode::none;
    // The disc under the cursor wins, the biggest one when they overlap; a
    // small disc can also be pointed from a few pixels away.
    sf::Vector2f const world = m_renderer.mapPixelToCoords(pixel, m_world);
    float const s = scale();
    auto const& nodes = m_island.bookmarks().nodes;
    size_t best = BookmarkNode::none;
    float best_score = 0.0f;
    for (ForceDirectedGraph::Vertex const& v: m_island.layout().vertices())
    {
        sf::Vector2f const d = (v.position - world) / s;
        float const distance = std::sqrt(d.x * d.x + d.y * d.y);
        float const radius = radiusOf(nodes[v.node]);
        if (distance > std::max(radius + 3.0f, PICK_DISTANCE))
            continue;
        float const score = distance - radius;
        if (best == BookmarkNode::none || score < best_score)
        {
            best = v.node;
            best_score = score;
        }
    }
    return best;
}

//------------------------------------------------------------------------------
void IslandedBrowserGUI::saveScreenshot(std::string const& path)
{
    sf::Texture texture;
    sf::Vector2u const size = m_renderer.getSize();
    if (texture.create(size.x, size.y))
    {
        texture.update(m_renderer);
        if (texture.copyToImage().saveToFile(path))
        {
            std::cout << "Screenshot: " << path << std::endl;
            toast("Screenshot saved in " + path);
            return;
        }
    }
    std::cerr << "Cannot save " << path << std::endl;
    toast("Cannot save " + path);
}

// =============================================================================
// Actions
// =============================================================================

//------------------------------------------------------------------------------
void IslandedBrowserGUI::select(size_t node)
{
    m_selected = node;
    m_related.clear();
    m_rows.clear();
    if (node != BookmarkNode::none && m_island.bookmarks().nodes[node].isBookmark())
        m_related = m_island.related(node, PANEL_MAX_ROWS / 2u);
}

//------------------------------------------------------------------------------
void IslandedBrowserGUI::open(size_t node)
{
    if (node == BookmarkNode::none)
        return;
    size_t total = 0u;
    std::vector<std::string> const urls = m_island.urls(node, MAX_TABS, total);
    std::ostringstream message;
    if (urls.empty())
    {
        message << "No bookmark in " << m_island.bookmarks().nodes[node].title;
    }
    else if (IslandedBrowser::openInBrowser(BROWSER_NAME, urls))
    {
        if (total == 1u)
            message << "Opened " << m_island.bookmarks().nodes[node].title;
        else
            message << "Opened " << urls.size() << " of " << total << " bookmarks of "
                    << m_island.bookmarks().nodes[node].title;
    }
    else
    {
        message << "Cannot start " << BROWSER_NAME;
    }
    std::cout << message.str() << std::endl;
    toast(message.str());
}

//------------------------------------------------------------------------------
void IslandedBrowserGUI::updateMatches()
{
    m_matches = m_island.search(toUtf8(m_query));
    m_match = m_matches.size();
}

//------------------------------------------------------------------------------
void IslandedBrowserGUI::nextMatch(int direction)
{
    size_t const count = m_matches.size();
    if (count == 0u)
        return;
    if (m_match >= count)
        m_match = (direction > 0) ? 0u : count - 1u;
    else
        m_match = (m_match + (direction > 0 ? 1u : count - 1u)) % count;
    select(m_matches[m_match]);
    centerOn(m_matches[m_match]);
}

//------------------------------------------------------------------------------
//! The node under the mouse (or else the search results, or else the
//! selection) stays bright with its community and its ancestors: the rest
//! of the map fades into the sea.
//------------------------------------------------------------------------------
void IslandedBrowserGUI::updateFocus()
{
    Bookmarks const& bookmarks = m_island.bookmarks();
    std::vector<size_t> nodes;
    auto add = [&](size_t node) {
        size_t root = node;
        if (bookmarks.nodes[node].isBookmark() && bookmarks.nodes[node].parent != 0u &&
            bookmarks.nodes[node].parent != BookmarkNode::none)
            root = bookmarks.nodes[node].parent;
        std::vector<size_t> const below = m_island.subtree(root);
        std::vector<size_t> const above = m_island.ancestors(root);
        nodes.insert(nodes.end(), below.begin(), below.end());
        nodes.insert(nodes.end(), above.begin(), above.end());
    };

    size_t const pointed = (m_hovered != BookmarkNode::none) ? m_hovered : m_hovered_row;
    if (pointed != BookmarkNode::none)
    {
        add(pointed);
    }
    else if (m_searching && !m_matches.empty())
    {
        for (size_t node: m_matches)
        {
            std::vector<size_t> const below = m_island.subtree(node);
            std::vector<size_t> const above = m_island.ancestors(node);
            nodes.insert(nodes.end(), below.begin(), below.end());
            nodes.insert(nodes.end(), above.begin(), above.end());
        }
    }
    else if (m_selected != BookmarkNode::none)
    {
        add(m_selected);
    }

    std::fill(m_focus.begin(), m_focus.end(), 0);
    for (size_t node: nodes)
        m_focus[node] = 1;
    m_has_focus = !nodes.empty();
}

//------------------------------------------------------------------------------
void IslandedBrowserGUI::updateCursor()
{
    if (!m_has_cursors)
        return;
    int const wanted = m_dragging ? 2 : (m_hovered != BookmarkNode::none ||
                                         m_hovered_row != BookmarkNode::none) ? 1 : 0;
    if (wanted == m_cursor)
        return;
    m_cursor = wanted;
    m_renderer.setMouseCursor(wanted == 2 ? m_move_cursor :
                              wanted == 1 ? m_hand_cursor : m_arrow_cursor);
}

//------------------------------------------------------------------------------
void IslandedBrowserGUI::quit()
{
    m_running = false;
    m_renderer.close();
}

// =============================================================================
// Events
// =============================================================================

//------------------------------------------------------------------------------
void IslandedBrowserGUI::handleInput()
{
    sf::Event event;
    while (m_renderer.pollEvent(event))
    {
        switch (event.type)
        {
        case sf::Event::Closed:
            quit();
            return;
        case sf::Event::Resized:
        {
            float const s = scale();
            sf::Vector2f const size(float(event.size.width), float(event.size.height));
            m_interface = sf::View(sf::FloatRect(0.0f, 0.0f, size.x, size.y));
            m_world.setSize(size * s);
            if (m_auto_fit)
                fitView();
            break;
        }
        case sf::Event::LostFocus:
            m_left_pressed = m_right_pressed = m_dragging = false;
            break;
        case sf::Event::KeyPressed:
            if (m_searching)
                onSearchKey(event.key);
            else
                onKeyPressed(event.key);
            break;
        case sf::Event::TextEntered:
            onTextEntered(event.text.unicode);
            break;
        case sf::Event::MouseWheelScrolled:
        {
            sf::Vector2i const pixel(event.mouseWheelScroll.x, event.mouseWheelScroll.y);
            if (!m_show_help && !onPanel(pixel))
                zoom(event.mouseWheelScroll.delta > 0.0f ? 0.8f : 1.25f, pixel);
            break;
        }
        case sf::Event::MouseButtonPressed:
        {
            sf::Vector2i const pixel(event.mouseButton.x, event.mouseButton.y);
            if (event.mouseButton.button == sf::Mouse::Middle)
            {
                size_t const node = pickAt(pixel);
                open(node != BookmarkNode::none ? node : m_hovered_row);
                break;
            }
            if (event.mouseButton.button == sf::Mouse::Left)
                m_left_pressed = true;
            else if (event.mouseButton.button == sf::Mouse::Right)
                m_right_pressed = true;
            m_press_pixel = m_drag_last = pixel;
            break;
        }
        case sf::Event::MouseButtonReleased:
        {
            sf::Vector2i const pixel(event.mouseButton.x, event.mouseButton.y);
            if (event.mouseButton.button == sf::Mouse::Left)
            {
                if (m_left_pressed && !m_dragging)
                {
                    onLeftReleased(pixel, sf::Keyboard::isKeyPressed(sf::Keyboard::LControl) ||
                                          sf::Keyboard::isKeyPressed(sf::Keyboard::RControl));
                }
                m_left_pressed = false;
            }
            else if (event.mouseButton.button == sf::Mouse::Right)
            {
                m_right_pressed = false;
            }
            if (!m_left_pressed && !m_right_pressed)
                m_dragging = false;
            break;
        }
        case sf::Event::MouseMoved:
        {
            sf::Vector2i const now(event.mouseMove.x, event.mouseMove.y);
            if ((m_left_pressed || m_right_pressed) && !m_dragging && !m_show_help &&
                !onPanel(m_press_pixel))
            {
                sf::Vector2i const d = now - m_press_pixel;
                m_dragging = (d.x * d.x + d.y * d.y > DRAG_THRESHOLD * DRAG_THRESHOLD);
            }
            if (m_dragging)
            {
                m_world.move(m_renderer.mapPixelToCoords(m_drag_last, m_world) -
                             m_renderer.mapPixelToCoords(now, m_world));
                m_drag_last = now;
                m_auto_fit = false;
                m_animating = false;
            }
            break;
        }
        default:
            break;
        }
    }
    if (!m_renderer.isOpen())
        return;

    // A screenshot shows the graph only, wherever the mouse is.
    m_mouse = sf::Mouse::getPosition(m_renderer);
    m_hovered = (m_screenshot.empty() && !m_dragging) ? pickAt(m_mouse) : BookmarkNode::none;
    m_hovered_row = BookmarkNode::none;
    if (!m_show_help && !m_dragging)
    {
        for (Row const& row: m_rows)
        {
            if (row.box.contains(sf::Vector2f(m_mouse)))
                m_hovered_row = row.node;
        }
    }
    updateFocus();
    updateCursor();
}

//------------------------------------------------------------------------------
void IslandedBrowserGUI::onLeftReleased(sf::Vector2i pixel, bool control)
{
    if (m_show_help)
    {
        m_show_help = false;
        return;
    }
    if (onPanel(pixel))
    {
        for (Row const& row: m_rows)
        {
            if (!row.box.contains(sf::Vector2f(pixel)))
                continue;
            if (control)
            {
                open(row.node);
            }
            else
            {
                select(row.node);
                centerOn(row.node);
            }
            break;
        }
        return;
    }

    size_t const node = pickAt(pixel);
    bool const double_click = (node != BookmarkNode::none) && (node == m_last_click) &&
        (m_click_clock.getElapsedTime().asSeconds() < DOUBLE_CLICK_DELAY);
    m_last_click = double_click ? BookmarkNode::none : node;
    m_click_clock.restart();

    if (node == BookmarkNode::none)
    {
        // Click on the sea.
        select(BookmarkNode::none);
    }
    else if (control)
    {
        open(node);
    }
    else if (double_click)
    {
        if (m_island.bookmarks().nodes[node].isBookmark())
            open(node);
        else
            zoomOn(node);
    }
    else
    {
        select(node);
    }
}

//------------------------------------------------------------------------------
void IslandedBrowserGUI::onKeyPressed(sf::Event::KeyEvent const& key)
{
    sf::Vector2f const window(m_renderer.getSize());
    switch (key.code)
    {
    case sf::Keyboard::Escape:
        if (m_show_help)
            m_show_help = false;
        else if (m_selected != BookmarkNode::none)
            select(BookmarkNode::none);
        else
            quit();
        break;
    case sf::Keyboard::Q:
        quit();
        break;
    case sf::Keyboard::H:
    case sf::Keyboard::F1:
        m_show_help = !m_show_help;
        break;
    case sf::Keyboard::F:
        if (key.control)
        {
            m_searching = true;
            m_show_help = false;
        }
        else
        {
            m_auto_fit = true;
            fitView();
        }
        break;
    case sf::Keyboard::Enter:
    case sf::Keyboard::O:
        open(m_selected);
        break;
    case sf::Keyboard::Z:
        if (m_selected != BookmarkNode::none)
            zoomOn(m_selected);
        break;
    case sf::Keyboard::Space:
        m_paused = !m_paused;
        toast(m_paused ? "Layout paused" : "Layout resumed");
        break;
    case sf::Keyboard::R:
        m_island.layout().restart();
        m_auto_fit = true;
        m_paused = false;
        break;
    case sf::Keyboard::L:
        m_show_links = !m_show_links;
        break;
    case sf::Keyboard::N:
        m_show_names = !m_show_names;
        break;
    case sf::Keyboard::S:
        saveScreenshot("islanded-browser.png");
        break;
    case sf::Keyboard::Left:
        pan(sf::Vector2f(-0.1f * window.x, 0.0f));
        break;
    case sf::Keyboard::Right:
        pan(sf::Vector2f(0.1f * window.x, 0.0f));
        break;
    case sf::Keyboard::Up:
        pan(sf::Vector2f(0.0f, -0.1f * window.y));
        break;
    case sf::Keyboard::Down:
        pan(sf::Vector2f(0.0f, 0.1f * window.y));
        break;
    case sf::Keyboard::Add:
        zoom(0.8f, sf::Vector2i(window / 2.0f));
        break;
    case sf::Keyboard::Subtract:
        zoom(1.25f, sf::Vector2i(window / 2.0f));
        break;
    default:
        break;
    }
}

//------------------------------------------------------------------------------
void IslandedBrowserGUI::onSearchKey(sf::Event::KeyEvent const& key)
{
    switch (key.code)
    {
    case sf::Keyboard::Escape:
        m_searching = false;
        m_query.clear();
        m_matches.clear();
        break;
    case sf::Keyboard::Enter:
        if (key.control)
            open(m_selected);
        else
            nextMatch(key.shift ? -1 : 1);
        break;
    case sf::Keyboard::Down:
    case sf::Keyboard::Tab:
        nextMatch(1);
        break;
    case sf::Keyboard::Up:
        nextMatch(-1);
        break;
    default:
        break;
    }
}

//------------------------------------------------------------------------------
//! Characters typed: the query of the search box, or the shortcuts that
//! depend on the keyboard layout (/ + - ?).
//------------------------------------------------------------------------------
void IslandedBrowserGUI::onTextEntered(sf::Uint32 unicode)
{
    if (m_searching)
    {
        if (unicode == 8u)
        {
            if (!m_query.isEmpty())
                m_query.erase(m_query.getSize() - 1u);
        }
        else if (unicode >= 32u && unicode != 127u)
        {
            m_query += unicode;
        }
        else
        {
            return;
        }
        updateMatches();
        m_caret_clock.restart();
        return;
    }

    sf::Vector2i const center(sf::Vector2f(m_renderer.getSize()) / 2.0f);
    switch (unicode)
    {
    case '/':
        m_searching = true;
        m_show_help = false;
        m_caret_clock.restart();
        updateMatches();
        break;
    case '+':
    case '=':
        zoom(0.8f, center);
        break;
    case '-':
        zoom(1.25f, center);
        break;
    case '?':
        m_show_help = !m_show_help;
        break;
    default:
        break;
    }
}

//------------------------------------------------------------------------------
void IslandedBrowserGUI::update(const float dt)
{
    ForceDirectedGraph& layout = m_island.layout();
    if (!m_paused)
        layout.update();
    if (m_auto_fit && layout.isRunning())
        fitView();
    if (m_animating)
    {
        float const k = 1.0f - std::exp(-10.0f * std::min(dt, 0.1f));
        sf::Vector2f const center = m_world.getCenter();
        sf::Vector2f const size = m_world.getSize();
        m_world.setCenter(center + (m_target_center - center) * k);
        m_world.setSize(size + (m_target_size - size) * k);
        sf::Vector2f const d = m_target_center - m_world.getCenter();
        if (std::fabs(d.x) + std::fabs(d.y) < 0.5f * scale() &&
            std::fabs(m_target_size.x - m_world.getSize().x) < 0.002f * m_target_size.x)
        {
            m_world.setCenter(m_target_center);
            m_world.setSize(m_target_size);
            m_animating = false;
        }
    }
}

// =============================================================================
// Drawing
// =============================================================================

//------------------------------------------------------------------------------
bool IslandedBrowserGUI::dimmed(size_t node) const
{
    return m_has_focus && !m_focus[node];
}

//------------------------------------------------------------------------------
sf::Color IslandedBrowserGUI::islandColor(size_t node) const
{
    size_t const vertex = m_island.layout().vertexOf(node);
    if (vertex == BookmarkNode::none || m_palette.empty())
        return TEXT_COLOR;
    return m_palette[m_island.layout().vertices()[vertex].island % m_palette.size()];
}

//------------------------------------------------------------------------------
sf::String IslandedBrowserGUI::kindOf(size_t node) const
{
    Bookmarks const& bookmarks = m_island.bookmarks();
    BookmarkNode const& n = bookmarks.nodes[node];
    if (n.isBookmark())
        return "BOOKMARK";
    if (n.depth <= 1u)
        return bookmarks.links.empty() ? "FOLDER" : "ISLAND";
    if (n.depth == 2u && !bookmarks.links.empty())
        return "COMMUNITY";
    return "FOLDER";
}

//------------------------------------------------------------------------------
sf::Color IslandedBrowserGUI::colorOf(ForceDirectedGraph::Vertex const& vertex) const
{
    sf::Color color = m_palette.empty() ? sf::Color::White
                                        : m_palette[vertex.island % m_palette.size()];
    BookmarkNode const& node = m_island.bookmarks().nodes[vertex.node];
    if (node.isBookmark())
    {
        // Lighter: the sand of the island.
        color = blend(color, sf::Color::White, 0.5f);
    }
    if (vertex.node == m_hovered)
        color = sf::Color::White;
    else if (dimmed(vertex.node))
        color = blend(color, SEA_COLOR, DIM_FACTOR);
    return color;
}

//------------------------------------------------------------------------------
void IslandedBrowserGUI::drawEdges()
{
    ForceDirectedGraph const& layout = m_island.layout();
    auto const& vertices = layout.vertices();
    sf::VertexArray tree(sf::Lines);
    sf::VertexArray links(sf::Lines);
    for (ForceDirectedGraph::Spring const& spring: layout.springs())
    {
        sf::Vector2f const a = vertices[spring.a].position;
        sf::Vector2f const b = vertices[spring.b].position;
        bool const faded = dimmed(vertices[spring.a].node) || dimmed(vertices[spring.b].node);
        if (spring.tree)
        {
            sf::Color color = TREE_EDGE_COLOR;
            if (faded)
                color.a = sf::Uint8(color.a / 4);
            else if (m_has_focus)
                color.a = sf::Uint8(std::min(255, 2 * int(color.a)));
            addLine(tree, a, b, color);
        }
        else if (m_show_links && vertices[spring.a].island == vertices[spring.b].island)
        {
            // Links inside an island, tinted by the island. Links between
            // islands are summarized by the rivers.
            sf::Color color = colorOf(vertices[spring.a]);
            color.a = faded ? 8 : 25;
            addLine(links, a, b, color);
        }
    }
    m_renderer.draw(links);
    m_renderer.draw(tree);
}

//------------------------------------------------------------------------------
void IslandedBrowserGUI::drawRivers()
{
    if (!m_show_links)
        return;
    float const s = scale();
    auto const& vertices = m_island.layout().vertices();
    sf::VertexArray triangles(sf::Triangles);
    for (River const& river: m_rivers)
    {
        sf::Color color = RIVER_COLOR;
        color.a = sf::Uint8(40.0f + 120.0f * river.strength);
        if (dimmed(vertices[river.a].node) && dimmed(vertices[river.b].node))
            color.a = sf::Uint8(color.a / 4);
        addThickLine(triangles, vertices[river.a].position, vertices[river.b].position,
                     (1.0f + 7.0f * river.strength) * s, color);
    }
    m_renderer.draw(triangles);
}

//------------------------------------------------------------------------------
void IslandedBrowserGUI::drawNodes()
{
    // Constant size on screen, whatever the zoom.
    float const s = scale();
    sf::VertexArray discs(sf::Triangles);
    sf::VertexArray rings(sf::Triangles);
    ForceDirectedGraph const& layout = m_island.layout();
    auto const& nodes = m_island.bookmarks().nodes;
    for (ForceDirectedGraph::Vertex const& v: layout.vertices())
    {
        float radius = radiusOf(nodes[v.node]);
        if (v.node == m_hovered || v.node == m_hovered_row)
            radius *= 1.4f;
        addDisc(discs, v.position, radius * s, colorOf(v));
    }
    // Search results, then the selection.
    if (m_searching)
    {
        for (size_t node: m_matches)
        {
            size_t const vertex = layout.vertexOf(node);
            if (vertex == BookmarkNode::none)
                continue;
            float const r = radiusOf(nodes[node]);
            addRing(rings, layout.vertices()[vertex].position, (r + 2.0f) * s,
                    (r + 4.0f) * s, URL_COLOR);
        }
    }
    size_t const vertex = layout.vertexOf(m_selected);
    if (m_selected != BookmarkNode::none && vertex != BookmarkNode::none)
    {
        float const r = radiusOf(nodes[m_selected]);
        addRing(rings, layout.vertices()[vertex].position, (r + 3.0f) * s,
                (r + 6.0f) * s, SELECTION_COLOR);
    }
    m_renderer.draw(discs);
    m_renderer.draw(rings);
}

//------------------------------------------------------------------------------
//! Island names always; community names and bookmark titles once zoomed in
//! enough to have room for them.
//------------------------------------------------------------------------------
void IslandedBrowserGUI::drawNames()
{
    if (!m_has_font || !m_show_names)
        return;
    Bookmarks const& bookmarks = m_island.bookmarks();
    ForceDirectedGraph const& layout = m_island.layout();
    sf::Vector2f const window(m_renderer.getSize());
    float const zoom = 1.0f / scale();
    sf::Text label;
    label.setFont(m_font);
    label.setOutlineColor(sf::Color::Black);

    auto write = [&](size_t node, sf::Vector2i pixel, unsigned size, sf::Color color,
                     bool centered) {
        sf::String title = utf8(bookmarks.nodes[node].title);
        if (title.getSize() > 40u)
            title = title.substring(0u, 39u) + sf::String(L"…");
        label.setString(title);
        label.setCharacterSize(size);
        label.setOutlineThickness(size >= CHARACTER_SIZE ? 2.0f : 1.5f);
        label.setFillColor(color);
        sf::FloatRect const box = label.getLocalBounds();
        float const radius = radiusOf(bookmarks.nodes[node]);
        if (centered)
            label.setPosition(std::round(float(pixel.x) - box.width / 2.0f),
                              std::round(float(pixel.y) - box.height - radius - 6.0f));
        else
            label.setPosition(std::round(float(pixel.x) + radius + 4.0f),
                              std::round(float(pixel.y) - float(size) * 0.65f));
        m_renderer.draw(label);
    };

    auto visible = [&](sf::Vector2i pixel) {
        return pixel.x > -200 && pixel.y > -50 && float(pixel.x) < window.x + 50.0f &&
               float(pixel.y) < window.y + 50.0f;
    };

    // Small labels first: the island names are written over them.
    bool const communities = (zoom >= COMMUNITY_LABEL_ZOOM);
    bool const titles = (zoom >= BOOKMARK_LABEL_ZOOM);
    if (communities)
    {
        for (ForceDirectedGraph::Vertex const& v: layout.vertices())
        {
            BookmarkNode const& node = bookmarks.nodes[v.node];
            bool const wanted = node.isBookmark() ? titles : (node.depth >= 2u);
            if (!wanted || dimmed(v.node) || v.node == m_selected)
                continue;
            sf::Vector2i const pixel = m_renderer.mapCoordsToPixel(v.position, m_world);
            if (!visible(pixel))
                continue;
            sf::Color color = node.isBookmark() ? TEXT_COLOR
                              : blend(m_palette[v.island % m_palette.size()],
                                      sf::Color::White, 0.4f);
            write(v.node, pixel, node.isBookmark() ? 13u : 15u, color, !node.isBookmark());
        }
    }

    for (size_t island: bookmarks.islands)
    {
        size_t const vertex = layout.vertexOf(island);
        if (vertex == BookmarkNode::none)
            continue;
        sf::Vector2i const pixel = m_renderer.mapCoordsToPixel(
            layout.vertices()[vertex].position, m_world);
        sf::Color color = m_palette[layout.vertices()[vertex].island % m_palette.size()];
        if (dimmed(island))
            color = blend(color, SEA_COLOR, 0.6f);
        write(island, pixel, CHARACTER_SIZE, color, true);
    }

    // The selection is always named, to find it on the map.
    size_t const vertex = layout.vertexOf(m_selected);
    if (m_selected != BookmarkNode::none && vertex != BookmarkNode::none &&
        bookmarks.nodes[m_selected].depth >= 2u)
    {
        sf::Vector2i const pixel = m_renderer.mapCoordsToPixel(
            layout.vertices()[vertex].position, m_world);
        write(m_selected, pixel, 15u, SELECTION_COLOR, !bookmarks.nodes[m_selected].isBookmark());
    }
}

//------------------------------------------------------------------------------
float IslandedBrowserGUI::text(sf::String const& string, sf::Vector2f position,
                               unsigned size, sf::Color color, bool bold)
{
    if (!m_has_font)
        return 0.0f;
    m_text.setString(string);
    m_text.setCharacterSize(size);
    m_text.setFillColor(color);
    m_text.setStyle(bold ? sf::Text::Bold : sf::Text::Regular);
    m_text.setPosition(std::round(position.x), std::round(position.y));
    m_renderer.draw(m_text);
    return m_text.getLocalBounds().width;
}

//------------------------------------------------------------------------------
float IslandedBrowserGUI::textWidth(sf::String const& string, unsigned size, bool bold)
{
    if (!m_has_font)
        return 0.0f;
    float width = 0.0f;
    for (sf::Uint32 c: string)
        width += m_font.getGlyph(c, size, bold).advance;
    return width;
}

//------------------------------------------------------------------------------
//! Greedy: cut at the last space of the line, or anywhere for long words
//! (URLs). An ellipsis ends the last line when the text does not fit.
//------------------------------------------------------------------------------
std::vector<sf::String> IslandedBrowserGUI::wrap(sf::String const& string, float width,
                                                 unsigned size, size_t max_lines)
{
    std::vector<sf::String> lines;
    if (!m_has_font)
        return lines;
    size_t const n = string.getSize();
    size_t i = 0u;
    while (i < n && lines.size() < max_lines)
    {
        while (i < n && string[i] == ' ')
            ++i;
        size_t const start = i;
        size_t last_space = BookmarkNode::none;
        float w = 0.0f;
        size_t j = i;
        for (; j < n; ++j)
        {
            float const advance = m_font.getGlyph(string[j], size, false).advance;
            if (w + advance > width && j > start)
                break;
            w += advance;
            if (string[j] == ' ')
                last_space = j;
        }
        if (j < n && last_space != BookmarkNode::none && last_space > start)
            j = last_space;
        lines.push_back(string.substring(start, j - start));
        i = j;
    }
    while (i < n && string[i] == ' ')
        ++i;
    if (i < n && !lines.empty())
    {
        sf::String& last = lines.back();
        sf::String const ellipsis(L"…");
        while (!last.isEmpty() && textWidth(last + ellipsis, size) > width)
            last.erase(last.getSize() - 1u);
        last += ellipsis;
    }
    return lines;
}

//------------------------------------------------------------------------------
void IslandedBrowserGUI::box(sf::FloatRect const& rect, sf::Color fill, sf::Color border)
{
    sf::RectangleShape shape(sf::Vector2f(std::round(rect.width), std::round(rect.height)));
    shape.setPosition(std::round(rect.left), std::round(rect.top));
    shape.setFillColor(fill);
    shape.setOutlineColor(border);
    shape.setOutlineThickness(border.a ? 1.0f : 0.0f);
    m_renderer.draw(shape);
}

//------------------------------------------------------------------------------
void IslandedBrowserGUI::drawTooltip()
{
    if (m_hovered == BookmarkNode::none || !m_has_font)
        return;
    Bookmarks const& bookmarks = m_island.bookmarks();
    BookmarkNode const& node = bookmarks.nodes[m_hovered];
    float const width = 380.0f;
    float const padding = 10.0f;

    struct Line { sf::String text; unsigned size; sf::Color color; bool bold; };
    std::vector<Line> lines;
    for (sf::String const& l: wrap(utf8(node.title), width, 17u, 2u))
        lines.push_back({ l, 17u, TEXT_COLOR, true });
    if (node.isBookmark())
    {
        for (sf::String const& l: wrap(utf8(node.url), width, 14u, 2u))
            lines.push_back({ l, 14u, URL_COLOR, false });
    }
    else
    {
        std::ostringstream info;
        size_t const count = m_island.bookmarkCount(m_hovered);
        info << count << (count > 1u ? " bookmarks" : " bookmark");
        // "ISLAND" -> "Island".
        sf::String kind = kindOf(m_hovered);
        for (size_t i = 1u; i < kind.getSize(); ++i)
        {
            if (kind[i] >= 'A' && kind[i] <= 'Z')
                kind[i] += 'a' - 'A';
        }
        lines.push_back({ kind + sf::String(L" \u00b7 ") + utf8(info.str()), 14u,
                          islandColor(m_hovered), false });
    }
    std::vector<size_t> const path = m_island.ancestors(m_hovered);
    if (!path.empty())
    {
        std::string joined;
        for (size_t p: path)
            joined += (joined.empty() ? "" : "  ›  ") + bookmarks.nodes[p].title;
        for (sf::String const& l: wrap(utf8(joined), width, 14u, 1u))
            lines.push_back({ l, 14u, DIM_TEXT_COLOR, false });
    }
    if (m_hovered == m_selected)
        lines.push_back({ node.isBookmark() ? "Double-click or Enter: open"
                                            : "Double-click: zoom    Enter: open its bookmarks",
                          13u, DIM_TEXT_COLOR, false });
    else
        lines.push_back({ node.isBookmark() ? "Click: details    Double-click: open"
                                            : "Click: details    Double-click: zoom    Middle-click: open",
                          13u, DIM_TEXT_COLOR, false });

    float height = 2.0f * padding;
    float content = 0.0f;
    for (Line const& l: lines)
    {
        height += lineHeight(l.size);
        content = std::max(content, textWidth(l.text, l.size, l.bold));
    }
    float const w = std::min(width, content) + 2.0f * padding;
    sf::Vector2f const window(m_renderer.getSize());
    sf::Vector2f p(float(m_mouse.x) + 18.0f, float(m_mouse.y) + 20.0f);
    if (p.x + w > window.x - 4.0f)
        p.x = float(m_mouse.x) - w - 12.0f;
    if (p.y + height > window.y - STATUSBAR_HEIGHT - 4.0f)
        p.y = float(m_mouse.y) - height - 12.0f;
    p.x = std::max(4.0f, p.x);
    p.y = std::max(4.0f, p.y);

    box(sf::FloatRect(p.x, p.y, w, height));
    sf::RectangleShape stripe(sf::Vector2f(3.0f, height));
    stripe.setPosition(p);
    stripe.setFillColor(islandColor(m_hovered));
    m_renderer.draw(stripe);
    float y = p.y + padding - 2.0f;
    for (Line const& l: lines)
    {
        text(l.text, sf::Vector2f(p.x + padding, y), l.size, l.color, l.bold);
        y += lineHeight(l.size);
    }
}

//------------------------------------------------------------------------------
//! Details of the selected node, on the right. The ancestors, the content
//! of a folder and the bookmarks similar to a bookmark are clickable.
//------------------------------------------------------------------------------
void IslandedBrowserGUI::drawPanel()
{
    m_rows.clear();
    m_panel_box = sf::FloatRect();
    if (m_selected == BookmarkNode::none || !m_has_font)
        return;

    Bookmarks const& bookmarks = m_island.bookmarks();
    BookmarkNode const& node = bookmarks.nodes[m_selected];
    sf::Vector2f const window(m_renderer.getSize());
    float const padding = 14.0f;
    float const inner = PANEL_WIDTH - 2.0f * padding;
    sf::Color const accent = islandColor(m_selected);

    struct Line
    {
        sf::String text;
        unsigned size;
        sf::Color color;
        bool bold;
        size_t node;
        float before;
        float indent;
    };
    std::vector<Line> lines;
    auto add = [&](sf::String const& t, unsigned size, sf::Color color, bool bold = false,
                   size_t target = BookmarkNode::none, float before = 0.0f, float indent = 0.0f) {
        lines.push_back({ t, size, color, bold, target, before, indent });
    };

    add(kindOf(m_selected), 13u, accent, true);
    for (sf::String const& l: wrap(utf8(node.title), inner, 20u, 3u))
        add(l, 20u, TEXT_COLOR, true);

    // Ancestors (clickable): where the node is.
    std::vector<size_t> const path = m_island.ancestors(m_selected);
    float indent = 0.0f;
    for (size_t i = 0u; i < path.size(); ++i)
    {
        sf::String const t = sf::String(L"› ") + utf8(bookmarks.nodes[path[i]].title);
        add(wrap(t, inner - indent, 14u, 1u).front(), 14u, DIM_TEXT_COLOR, false, path[i],
            i == 0u ? 4.0f : 0.0f, indent);
        indent += 12.0f;
    }

    if (node.isBookmark())
    {
        bool first = true;
        for (sf::String const& l: wrap(utf8(node.url), inner, 14u, 4u))
        {
            add(l, 14u, URL_COLOR, false, BookmarkNode::none, first ? 8.0f : 0.0f);
            first = false;
        }
        if (!m_related.empty())
        {
            add("Similar bookmarks", 14u, TEXT_COLOR, true, BookmarkNode::none, 12.0f);
            for (auto const& r: m_related)
            {
                std::ostringstream score;
                score << "  " << int(std::round(r.second * 100.0f)) << "%";
                sf::String const s = utf8(score.str());
                float const room = inner - textWidth(s, 14u) - 12.0f;
                sf::String const t = sf::String(L"• ") +
                                     utf8(bookmarks.nodes[r.first].title);
                add(wrap(t, room, 14u, 1u).front() + s, 14u, TEXT_COLOR, false, r.first);
            }
        }
    }
    else
    {
        size_t folders = 0u;
        for (size_t child: node.children)
            folders += bookmarks.nodes[child].isBookmark() ? 0u : 1u;
        std::ostringstream info;
        size_t const count = m_island.bookmarkCount(m_selected);
        info << count << (count > 1u ? " bookmarks" : " bookmark");
        if (folders > 0u)
            info << "  ·  " << folders << (folders > 1u ? " sub-folders" : " sub-folder");
        add(utf8(info.str()), 14u, TEXT_COLOR, false, BookmarkNode::none, 8.0f);

        // Content: the folders first, then the bookmarks.
        std::vector<size_t> children = node.children;
        std::stable_sort(children.begin(), children.end(), [&](size_t a, size_t b) {
            return !bookmarks.nodes[a].isBookmark() && bookmarks.nodes[b].isBookmark();
        });
        if (!children.empty())
            add("Content", 14u, TEXT_COLOR, true, BookmarkNode::none, 12.0f);
        size_t shown = 0u;
        for (size_t child: children)
        {
            if (shown++ >= PANEL_MAX_ROWS)
                break;
            BookmarkNode const& c = bookmarks.nodes[child];
            sf::String t;
            if (c.isBookmark())
            {
                t = sf::String(L"• ") + utf8(c.title);
            }
            else
            {
                std::ostringstream n;
                n << "  (" << m_island.bookmarkCount(child) << ")";
                sf::String const s = utf8(n.str());
                float const room = inner - textWidth(s, 14u) - 12.0f;
                t = wrap(sf::String(L"› ") + utf8(c.title), room, 14u, 1u).front() + s;
            }
            add(wrap(t, inner, 14u, 1u).front(), 14u,
                c.isBookmark() ? TEXT_COLOR : blend(accent, sf::Color::White, 0.3f),
                false, child);
        }
        if (children.size() > PANEL_MAX_ROWS)
        {
            std::ostringstream more;
            more << "… and " << (children.size() - PANEL_MAX_ROWS) << " more";
            add(utf8(more.str()), 14u, DIM_TEXT_COLOR);
        }
    }

    // What can be done.
    std::ostringstream hint;
    if (node.isBookmark())
    {
        hint << "Enter or double-click: open in " << BROWSER_NAME;
    }
    else
    {
        size_t const count = m_island.bookmarkCount(m_selected);
        hint << "Enter: open " << std::min<size_t>(count, MAX_TABS)
             << (count > MAX_TABS ? " first" : "") << " bookmarks in " << BROWSER_NAME;
    }
    add(utf8(hint.str()), 13u, DIM_TEXT_COLOR, false, BookmarkNode::none, 14.0f);
    add("Z: zoom on it    Click an entry to go to it    Esc: close", 13u, DIM_TEXT_COLOR);

    // Layout, cut at the status bar.
    float const top = 12.0f;
    float const bottom = window.y - STATUSBAR_HEIGHT - 12.0f;
    float height = 2.0f * padding;
    size_t count = 0u;
    for (Line const& l: lines)
    {
        float const h = l.before + lineHeight(l.size);
        if (top + height + h > bottom)
            break;
        height += h;
        ++count;
    }
    float const left = window.x - PANEL_WIDTH - 12.0f;
    m_panel_box = sf::FloatRect(left, top, PANEL_WIDTH, height);
    box(m_panel_box);
    sf::RectangleShape stripe(sf::Vector2f(PANEL_WIDTH, 3.0f));
    stripe.setPosition(left, top);
    stripe.setFillColor(accent);
    m_renderer.draw(stripe);

    float y = top + padding;
    for (size_t i = 0u; i < count; ++i)
    {
        Line const& l = lines[i];
        y += l.before;
        float const h = lineHeight(l.size);
        if (l.node != BookmarkNode::none)
        {
            sf::FloatRect const row(left + 4.0f, y - 1.0f, PANEL_WIDTH - 8.0f, h);
            m_rows.push_back({ row, l.node });
            if (row.contains(sf::Vector2f(m_mouse)))
                box(row, sf::Color(255, 255, 255, 28), sf::Color::Transparent);
        }
        text(l.text, sf::Vector2f(left + padding + l.indent, y), l.size, l.color, l.bold);
        y += h;
    }
}

//------------------------------------------------------------------------------
void IslandedBrowserGUI::drawSearch()
{
    m_search_box = sf::FloatRect();
    if (!m_searching || !m_has_font)
        return;
    sf::Vector2f const window(m_renderer.getSize());
    float const width = std::min(520.0f, window.x - 24.0f);
    m_search_box = sf::FloatRect(std::round((window.x - width) / 2.0f),
                                 12.0f, width, 40.0f);
    box(m_search_box, PANEL_COLOR, URL_COLOR);

    float const x = m_search_box.left + 12.0f;
    float const y = m_search_box.top + 9.0f;
    float w = text("Search:", sf::Vector2f(x, y), 17u, DIM_TEXT_COLOR);
    // The end of a long query stays visible.
    sf::String shown = m_query;
    float const room = width - w - 150.0f;
    while (shown.getSize() > 1u && textWidth(shown, 17u) > room)
        shown.erase(0u);
    float const q = text(shown, sf::Vector2f(x + w + 8.0f, y), 17u, TEXT_COLOR);
    if (int(m_caret_clock.getElapsedTime().asSeconds() * 2.0f) % 2 == 0)
    {
        sf::RectangleShape caret(sf::Vector2f(2.0f, 20.0f));
        caret.setPosition(std::round(x + w + 10.0f + q), y + 1.0f);
        caret.setFillColor(TEXT_COLOR);
        m_renderer.draw(caret);
    }

    std::ostringstream count;
    if (m_query.isEmpty())
        count << "titles and URLs";
    else if (m_matches.empty())
        count << "no match";
    else if (m_match < m_matches.size())
        count << (m_match + 1u) << " / " << m_matches.size();
    else
        count << m_matches.size() << (m_matches.size() > 1u ? " matches" : " match");
    sf::String const c = utf8(count.str());
    text(c, sf::Vector2f(m_search_box.left + width - 12.0f - textWidth(c, 15u), y + 2.0f),
         15u, m_matches.empty() && !m_query.isEmpty() ? sf::Color(255, 140, 120) : DIM_TEXT_COLOR);
}

//------------------------------------------------------------------------------
void IslandedBrowserGUI::drawStatusBar()
{
    if (!m_has_font)
        return;
    sf::Vector2f const window(m_renderer.getSize());
    float const top = window.y - STATUSBAR_HEIGHT;
    box(sf::FloatRect(0.0f, top, window.x, STATUSBAR_HEIGHT), PANEL_COLOR,
        sf::Color::Transparent);

    Bookmarks const& bookmarks = m_island.bookmarks();
    ForceDirectedGraph const& layout = m_island.layout();
    std::ostringstream left;
    left << m_file_name << "   ·   " << bookmarks.bookmark_count << " bookmarks   ·   "
         << bookmarks.islands.size() << (bookmarks.links.empty() ? " folders" : " islands")
         << "   ·   layout "
         << (!layout.isRunning() ? "stable" : m_paused ? "paused (Space)" : "settling…");
    float const y = top + 4.0f;
    text(utf8(left.str()), sf::Vector2f(10.0f, y), SMALL_CHARACTER_SIZE, DIM_TEXT_COLOR);

    sf::String hint;
    if (m_show_help)
        hint = "H or Esc: close the help";
    else if (m_searching)
        hint = L"Type to search    Enter / ↓: next    ↑: previous    Ctrl + Enter: open    Esc: close";
    else if (m_selected != BookmarkNode::none)
        hint = "Enter: open    Z: zoom    Esc: unselect    /: search    H: help";
    else
        hint = "Click: select    Double-click: open    Drag: move    Wheel: zoom    "
               "/: search    H: help";
    text(hint, sf::Vector2f(window.x - 10.0f - textWidth(hint, SMALL_CHARACTER_SIZE), y),
         SMALL_CHARACTER_SIZE, TEXT_COLOR);
}

//------------------------------------------------------------------------------
void IslandedBrowserGUI::toast(std::string const& message)
{
    m_toast = utf8(message);
    m_toast_clock.restart();
}

//------------------------------------------------------------------------------
//! Short notification above the status bar, faded out after a while.
//------------------------------------------------------------------------------
void IslandedBrowserGUI::drawToast()
{
    if (m_toast.isEmpty() || !m_has_font)
        return;
    float const t = m_toast_clock.getElapsedTime().asSeconds();
    float const fade = 0.4f;
    if (t >= TOAST_DURATION + fade)
    {
        m_toast.clear();
        return;
    }
    // Fade in quickly, stay, fade out.
    float const alpha = std::min(std::min(t / 0.15f, 1.0f),
                                 std::max(0.0f, 1.0f - (t - TOAST_DURATION) / fade));
    auto faded = [alpha](sf::Color c) {
        c.a = sf::Uint8(float(c.a) * alpha);
        return c;
    };

    sf::Vector2f const window(m_renderer.getSize());
    float const padding = 16.0f;
    sf::String const message = wrap(m_toast, std::min(700.0f, window.x - 80.0f),
                                    SMALL_CHARACTER_SIZE + 1u, 1u).front();
    float const width = textWidth(message, SMALL_CHARACTER_SIZE + 1u) + 2.0f * padding;
    float const height = 36.0f;
    // Slides up a little while appearing.
    float const lift = 8.0f * (1.0f - std::min(t / 0.15f, 1.0f));
    sf::FloatRect const rect(std::round((window.x - width) / 2.0f),
                             window.y - STATUSBAR_HEIGHT - height - 16.0f + lift,
                             width, height);
    box(rect, faded(PANEL_COLOR), faded(PANEL_BORDER_COLOR));
    sf::RectangleShape stripe(sf::Vector2f(3.0f, height));
    stripe.setPosition(rect.left, rect.top);
    stripe.setFillColor(faded(URL_COLOR));
    m_renderer.draw(stripe);
    text(message, sf::Vector2f(rect.left + padding, rect.top + 8.0f),
         SMALL_CHARACTER_SIZE + 1u, faded(TEXT_COLOR));
}

//------------------------------------------------------------------------------
void IslandedBrowserGUI::drawHelp()
{
    if (!m_show_help || !m_has_font)
        return;
    sf::Vector2f const window(m_renderer.getSize());
    box(sf::FloatRect(0.0f, 0.0f, window.x, window.y), sf::Color(0, 0, 0, 120),
        sf::Color::Transparent);

    std::ostringstream tabs;
    tabs << "open in " << BROWSER_NAME << " (a folder: its " << MAX_TABS << " first bookmarks)";
    struct Entry { sf::String key; sf::String action; };
    std::vector<std::pair<sf::String, std::vector<Entry>>> const sections = {
        { "Mouse", {
            { "Hover", "title, URL; its community is highlighted" },
            { "Left click", "select: details in the right panel" },
            { "Double click", "open a bookmark, zoom on a folder" },
            { "Middle or Ctrl + click", utf8(tabs.str()) },
            { "Left or right drag", "move the map" },
            { "Wheel", "zoom around the cursor" },
            { "Click on the sea", "unselect" } } },
        { "Keyboard", {
            { "/  or  Ctrl + F", "search the titles and URLs" },
            { "Enter", "open the selection" },
            { "Z  /  F", "zoom on the selection  /  fit the whole map" },
            { "Arrows,  +  -", "move, zoom" },
            { "Space  /  R", "pause  /  restart the layout" },
            { "L  /  N", "show the links and rivers  /  the names" },
            { "S", "screenshot in islanded-browser.png" },
            { "Esc  /  Q", "close, unselect, then quit  /  quit" } } },
        { "Map", {
            { "Big disc", "island (a subject)" },
            { "Medium disc", "community or folder" },
            { "Small disc", "bookmark" },
            { "Blue bands", "rivers: islands with similar bookmarks" } } }
    };

    float const column = 200.0f;
    float const width = std::min(window.x - 24.0f, 760.0f);
    // Title, sections (header, entries, gap), footer.
    float height = 18.0f + 40.0f + 36.0f;
    for (auto const& section: sections)
        height += 30.0f + lineHeight(16u) * float(section.second.size()) + 4.0f;
    sf::Vector2f const p(std::round((window.x - width) / 2.0f),
                         std::round(std::max(12.0f, (window.y - height) / 2.0f)));
    box(sf::FloatRect(p.x, p.y, width, height), PANEL_COLOR, URL_COLOR);

    float y = p.y + 18.0f;
    text("Islanded Browser", sf::Vector2f(p.x + 24.0f, y), 24u, TEXT_COLOR, true);
    y += 40.0f;
    for (auto const& section: sections)
    {
        text(section.first, sf::Vector2f(p.x + 24.0f, y), 17u, URL_COLOR, true);
        y += 30.0f;
        for (Entry const& e: section.second)
        {
            text(e.key, sf::Vector2f(p.x + 36.0f, y), 16u, SELECTION_COLOR);
            text(e.action, sf::Vector2f(p.x + 36.0f + column, y), 16u, TEXT_COLOR);
            y += lineHeight(16u);
        }
        y += 4.0f;
    }
    text("Press H, Esc or click to close", sf::Vector2f(p.x + 24.0f, p.y + height - 30.0f),
         14u, DIM_TEXT_COLOR);
}

//------------------------------------------------------------------------------
void IslandedBrowserGUI::draw()
{
    if (!m_renderer.isOpen())
        return;

    m_renderer.setView(m_world);
    drawRivers();
    drawEdges();
    drawNodes();

    m_renderer.setView(m_interface);
    drawNames();
    if (m_screenshot.empty())
    {
        drawPanel();
        drawSearch();
        drawStatusBar();
        drawTooltip();
        drawToast();
        drawHelp();
    }

    if (!m_screenshot.empty() && !m_island.layout().isRunning())
    {
        saveScreenshot(m_screenshot);
        m_screenshot.clear();
        quit();
    }
}
