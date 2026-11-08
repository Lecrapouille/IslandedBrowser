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

    for (size_t i = 0u; i < bookmarks.islands.size(); ++i)
        m_palette.push_back(hue(i));
    computeRivers();

    sf::Vector2f const size(renderer().getSize());
    m_interface = sf::View(sf::FloatRect(0.0f, 0.0f, size.x, size.y));
    m_world = m_interface;
    m_has_font = m_font.loadFromFile("data/font.ttf");
    if (m_has_font)
        m_message_bar.font("data/font.ttf");
    else
        std::cerr << "data/font.ttf not found: texts are not displayed" << std::endl;
    fitView();
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

//------------------------------------------------------------------------------
void IslandedBrowserGUI::fitView()
{
    sf::FloatRect box = m_island.layout().bounds();
    sf::Vector2f const window(renderer().getSize());
    float const margin = 0.05f * std::max(box.width, box.height) + 20.0f;
    box.left -= margin;
    box.top -= margin;
    box.width += 2.0f * margin;
    box.height += 2.0f * margin;
    // Keep the aspect ratio of the window.
    float const scale = std::max(box.width / window.x, box.height / window.y);
    m_world.setCenter(box.left + box.width / 2.0f, box.top + box.height / 2.0f);
    m_world.setSize(window.x * scale, window.y * scale);
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
}

//------------------------------------------------------------------------------
void IslandedBrowserGUI::click(sf::Vector2i pixel)
{
    sf::Vector2f const world = m_renderer.mapPixelToCoords(pixel, m_world);
    float const scale = m_world.getSize().x / float(renderer().getSize().x);
    size_t const node = m_island.pick(world, PICK_DISTANCE * scale);
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
        message << "Opened " << urls.size() << " of " << total << " bookmarks";
    }
    else
    {
        message << "Cannot start " << BROWSER_NAME;
    }
    std::cout << message.str() << std::endl;
    m_message_bar.entry(message.str(), MESSAGEBAR_COLOR);
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
            return;
        }
    }
    std::cerr << "Cannot save " << path << std::endl;
}

//------------------------------------------------------------------------------
void IslandedBrowserGUI::handleInput()
{
    sf::Vector2i const mouse = sf::Mouse::getPosition(renderer());
    sf::Vector2f const world = m_renderer.mapPixelToCoords(mouse, m_world);
    float const scale = m_world.getSize().x / float(renderer().getSize().x);
    // A screenshot shows the graph only, wherever the mouse is.
    m_hovered = m_screenshot.empty() ? m_island.pick(world, PICK_DISTANCE * scale)
                                     : BookmarkNode::none;
    if (m_hovered != BookmarkNode::none)
    {
        BookmarkNode const& node = m_island.bookmarks().nodes[m_hovered];
        m_message_bar.entry(node.isBookmark() ? node.title + "  " + node.url : node.title,
                            MESSAGEBAR_COLOR);
    }

    sf::Event event;
    while (m_renderer.pollEvent(event))
    {
        switch (event.type)
        {
        case sf::Event::Closed:
            m_running = false;
            m_renderer.close();
            break;
        case sf::Event::Resized:
            m_interface = sf::View(sf::FloatRect(0.0f, 0.0f, float(event.size.width),
                                                 float(event.size.height)));
            fitView();
            break;
        case sf::Event::KeyPressed:
            switch (event.key.code)
            {
            case sf::Keyboard::Escape:
                m_running = false;
                m_renderer.close();
                break;
            case sf::Keyboard::Space:
                m_paused = !m_paused;
                break;
            case sf::Keyboard::R:
                m_island.layout().restart();
                m_auto_fit = true;
                break;
            case sf::Keyboard::F:
                m_auto_fit = true;
                fitView();
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
            default:
                break;
            }
            break;
        case sf::Event::MouseWheelScrolled:
            zoom(event.mouseWheelScroll.delta > 0.0f ? 0.8f : 1.25f,
                 sf::Vector2i(event.mouseWheelScroll.x, event.mouseWheelScroll.y));
            break;
        case sf::Event::MouseButtonPressed:
            if (event.mouseButton.button == sf::Mouse::Left)
            {
                click(sf::Vector2i(event.mouseButton.x, event.mouseButton.y));
            }
            else if (event.mouseButton.button == sf::Mouse::Right)
            {
                m_dragging = true;
                m_drag_origin = sf::Vector2i(event.mouseButton.x, event.mouseButton.y);
            }
            break;
        case sf::Event::MouseButtonReleased:
            if (event.mouseButton.button == sf::Mouse::Right)
                m_dragging = false;
            break;
        case sf::Event::MouseMoved:
            if (m_dragging)
            {
                sf::Vector2i const now(event.mouseMove.x, event.mouseMove.y);
                m_world.move(m_renderer.mapPixelToCoords(m_drag_origin, m_world) -
                             m_renderer.mapPixelToCoords(now, m_world));
                m_drag_origin = now;
                m_auto_fit = false;
            }
            break;
        default:
            break;
        }
    }
}

//------------------------------------------------------------------------------
void IslandedBrowserGUI::update(const float /*dt*/)
{
    ForceDirectedGraph& layout = m_island.layout();
    if (!m_paused)
        layout.update();
    if (m_auto_fit && layout.isRunning())
        fitView();
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
        color.r = sf::Uint8((color.r + 255) / 2);
        color.g = sf::Uint8((color.g + 255) / 2);
        color.b = sf::Uint8((color.b + 255) / 2);
    }
    if (vertex.node == m_hovered)
        color = sf::Color::White;
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
        if (spring.tree)
        {
            addLine(tree, a, b, TREE_EDGE_COLOR);
        }
        else if (m_show_links && vertices[spring.a].island == vertices[spring.b].island)
        {
            // Links inside an island, tinted by the island. Links between
            // islands are summarized by the rivers.
            sf::Color color = colorOf(vertices[spring.a]);
            color.a = 25;
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
    float const scale = m_world.getSize().x / float(renderer().getSize().x);
    auto const& vertices = m_island.layout().vertices();
    sf::VertexArray triangles(sf::Triangles);
    for (River const& river: m_rivers)
    {
        sf::Color color = RIVER_COLOR;
        color.a = sf::Uint8(40.0f + 120.0f * river.strength);
        addThickLine(triangles, vertices[river.a].position, vertices[river.b].position,
                     (1.0f + 7.0f * river.strength) * scale, color);
    }
    m_renderer.draw(triangles);
}

//------------------------------------------------------------------------------
void IslandedBrowserGUI::drawNodes()
{
    // Constant size on screen, whatever the zoom.
    float const scale = m_world.getSize().x / float(renderer().getSize().x);
    sf::VertexArray discs(sf::Triangles);
    auto const& nodes = m_island.bookmarks().nodes;
    for (ForceDirectedGraph::Vertex const& v: m_island.layout().vertices())
    {
        BookmarkNode const& node = nodes[v.node];
        float radius = BOOKMARK_RADIUS;
        if (!node.isBookmark())
            radius = (node.depth <= 1u) ? ISLAND_RADIUS : FOLDER_RADIUS;
        addDisc(discs, v.position, radius * scale, colorOf(v));
    }
    m_renderer.draw(discs);
}

//------------------------------------------------------------------------------
void IslandedBrowserGUI::drawNames()
{
    if (!m_has_font || !m_show_names)
        return;
    Bookmarks const& bookmarks = m_island.bookmarks();
    ForceDirectedGraph const& layout = m_island.layout();
    sf::Text text;
    text.setFont(m_font);
    text.setCharacterSize(CHARACTER_SIZE);
    text.setOutlineColor(sf::Color::Black);
    text.setOutlineThickness(2.0f);
    for (size_t island: bookmarks.islands)
    {
        size_t const vertex = layout.vertexOf(island);
        if (vertex == BookmarkNode::none)
            continue;
        sf::Vector2i const pixel = m_renderer.mapCoordsToPixel(
            layout.vertices()[vertex].position, m_world);
        text.setString(sf::String::fromUtf8(bookmarks.nodes[island].title.begin(),
                                            bookmarks.nodes[island].title.end()));
        sf::FloatRect const box = text.getLocalBounds();
        text.setPosition(std::round(float(pixel.x) - box.width / 2.0f),
                         std::round(float(pixel.y) - box.height - ISLAND_RADIUS - 4.0f));
        text.setFillColor(m_palette[layout.vertices()[vertex].island % m_palette.size()]);
        m_renderer.draw(text);
    }
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
        m_message_bar.size(m_renderer.getSize());
        m_renderer.draw(m_message_bar);
    }

    if (!m_screenshot.empty() && !m_island.layout().isRunning())
    {
        saveScreenshot(m_screenshot);
        m_screenshot.clear();
        m_running = false;
        m_renderer.close();
    }
}
