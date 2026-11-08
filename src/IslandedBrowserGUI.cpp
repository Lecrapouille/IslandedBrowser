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
#include <iostream>
#include <sstream>

namespace {

//------------------------------------------------------------------------------
//! \brief The file and its folder, which tells private/ from examples/:
//! "/home/me/IslandedBrowser/examples/organization.json" ->
//! "examples/organization.json".
//------------------------------------------------------------------------------
std::string fileName(std::string const& path)
{
    size_t const slash = path.find_last_of('/');
    if (slash == std::string::npos || slash == 0u)
        return path;
    size_t const before = path.find_last_of('/', slash - 1u);
    return (before == std::string::npos) ? path : path.substr(before + 1u);
}

} // namespace

//------------------------------------------------------------------------------
IslandedBrowserGUI::IslandedBrowserGUI(Application& application, const char* name,
                                       std::string const& bookmarks_path,
                                       std::string const& screenshot,
                                       bool terrain)
    : Application::GUI(application, name, SEA_COLOR),
      m_island(bookmarks_path),
      m_painter(m_renderer, "data/font.ttf"),
      m_camera(m_renderer),
      m_palette(m_island),
      m_routes(m_island),
      m_selection(m_island),
      m_map(m_renderer, m_island, m_camera, m_palette, m_selection, m_routes, m_painter),
      m_tooltip(m_painter, m_island, m_palette),
      m_panel(m_painter, m_island, m_palette),
      m_search(m_painter, m_island),
      m_status(m_painter, m_island, fileName(bookmarks_path)),
      m_toast(m_painter),
      m_help(m_painter),
      m_cursor(m_renderer),
      m_terrain_painter(m_terrain, m_palette),
      m_show_terrain(terrain),
      m_screenshot(screenshot)
{
    Bookmarks const& bookmarks = m_island.bookmarks();
    std::cout << bookmarks_path << ": " << bookmarks.bookmark_count << " bookmarks, "
              << bookmarks.islands.size() << " islands, " << bookmarks.links.size()
              << " similarity links" << std::endl;
    fitView();
    if (m_screenshot.empty())
    {
        m_toast.show("Click a node to see its details, double-click to open it, "
                     "/ to search, H for help");
    }
}

//------------------------------------------------------------------------------
void IslandedBrowserGUI::activate()
{
    m_renderer.setView(m_camera.world());
}

// =============================================================================
// Questions
// =============================================================================

//------------------------------------------------------------------------------
float IslandedBrowserGUI::hiddenWidth() const
{
    return (m_selection.selected() != BookmarkNode::none) ? PANEL_WIDTH + 12.0f : 0.0f;
}

//------------------------------------------------------------------------------
bool IslandedBrowserGUI::onPanel(sf::Vector2i pixel) const
{
    if (float(pixel.y) >= float(m_renderer.getSize().y) - STATUSBAR_HEIGHT)
        return true;
    if (m_selection.selected() != BookmarkNode::none && m_panel.contains(pixel))
        return true;
    return m_search.contains(pixel);
}

//------------------------------------------------------------------------------
size_t IslandedBrowserGUI::pickAt(sf::Vector2i pixel) const
{
    sf::Vector2u const window = m_renderer.getSize();
    if (m_help.visible() || pixel.x < 0 || pixel.y < 0 || pixel.x >= int(window.x) ||
        pixel.y >= int(window.y) || onPanel(pixel))
        return BookmarkNode::none;
    return m_map.nodeAt(pixel);
}

//------------------------------------------------------------------------------
StatusBar::Mode IslandedBrowserGUI::mode() const
{
    if (m_help.visible())
        return StatusBar::Mode::Help;
    if (m_search.isOpen())
        return StatusBar::Mode::Searching;
    if (m_selection.selected() != BookmarkNode::none)
        return StatusBar::Mode::Selected;
    return StatusBar::Mode::Browsing;
}

// =============================================================================
// Actions
// =============================================================================

//------------------------------------------------------------------------------
void IslandedBrowserGUI::fitView()
{
    sf::FloatRect area = m_island.layout().bounds();
    // The land can reach beyond the nodes: show it whole.
    if (m_show_terrain && m_terrain.ready())
    {
        sf::FloatRect const land = m_terrain.landBounds();
        float const right = std::max(area.left + area.width, land.left + land.width);
        float const bottom = std::max(area.top + area.height, land.top + land.height);
        area.left = std::min(area.left, land.left);
        area.top = std::min(area.top, land.top);
        area.width = right - area.left;
        area.height = bottom - area.top;
    }
    m_camera.fit(area);
}

//------------------------------------------------------------------------------
void IslandedBrowserGUI::centerOn(size_t node)
{
    ForceDirectedGraph const& layout = m_island.layout();
    size_t const vertex = layout.vertexOf(node);
    if (vertex != BookmarkNode::none)
        m_camera.glideTo(layout.vertices()[vertex].position, hiddenWidth());
}

//------------------------------------------------------------------------------
void IslandedBrowserGUI::zoomOn(size_t node)
{
    // Smallest rectangle holding every disc of the group.
    ForceDirectedGraph const& layout = m_island.layout();
    bool empty = true;
    sf::Vector2f low, high;
    for (size_t i: m_island.subtree(m_island.community(node)))
    {
        size_t const vertex = layout.vertexOf(i);
        if (vertex == BookmarkNode::none)
            continue;
        sf::Vector2f const p = layout.vertices()[vertex].position;
        if (empty)
        {
            low = high = p;
            empty = false;
        }
        low.x = std::min(low.x, p.x);
        low.y = std::min(low.y, p.y);
        high.x = std::max(high.x, p.x);
        high.y = std::max(high.y, p.y);
    }
    if (empty)
        return;

    // A tiny group (a single bookmark) would be zoomed far too much: keep
    // a minimum size around it.
    sf::FloatRect area(low, high - low);
    float const minimum = 4.0f * layout.params.ideal_distance;
    if (area.width < minimum)
    {
        area.left -= (minimum - area.width) / 2.0f;
        area.width = minimum;
    }
    if (area.height < minimum)
    {
        area.top -= (minimum - area.height) / 2.0f;
        area.height = minimum;
    }
    m_camera.glideOver(area, hiddenWidth());
}

//------------------------------------------------------------------------------
void IslandedBrowserGUI::select(size_t node)
{
    m_selection.select(node);
    m_panel.clear();
}

//------------------------------------------------------------------------------
void IslandedBrowserGUI::open(size_t node)
{
    if (node == BookmarkNode::none)
        return;
    size_t total = 0u;
    std::vector<std::string> const urls = m_island.urls(node, MAX_TABS, total);
    std::string const& title = m_island.bookmarks().nodes[node].title;
    std::ostringstream message;
    if (urls.empty())
        message << "No bookmark in " << title;
    else if (!IslandedBrowser::openInBrowser(BROWSER_NAME, urls))
        message << "Cannot start " << BROWSER_NAME;
    else if (total == 1u)
        message << "Opened " << title;
    else
        message << "Opened " << urls.size() << " of " << total << " bookmarks of " << title;
    std::cout << message.str() << std::endl;
    m_toast.show(message.str());
}

//------------------------------------------------------------------------------
void IslandedBrowserGUI::nextMatch(int direction)
{
    size_t const node = m_search.next(direction);
    if (node == BookmarkNode::none)
        return;
    select(node);
    centerOn(node);
}

//------------------------------------------------------------------------------
void IslandedBrowserGUI::saveScreenshot(std::string const& path)
{
    // Copy what the window shows into an image, then write it to the disk.
    sf::Texture texture;
    sf::Vector2u const size = m_renderer.getSize();
    if (texture.create(size.x, size.y))
    {
        texture.update(m_renderer);
        if (texture.copyToImage().saveToFile(path))
        {
            std::cout << "Screenshot: " << path << std::endl;
            m_toast.show("Screenshot saved in " + path);
            return;
        }
    }
    std::cerr << "Cannot save " << path << std::endl;
    m_toast.show("Cannot save " + path);
}

//------------------------------------------------------------------------------
void IslandedBrowserGUI::updateTerrain()
{
    if (!m_show_terrain || m_terrain.ready() || m_island.layout().isRunning())
        return;
    sf::Clock clock;
    m_terrain.build(m_island);
    m_terrain_painter.update();
    std::cout << "Terrain: " << m_terrain.rivers().size() << " rivers in "
              << clock.getElapsedTime().asMilliseconds() << " ms" << std::endl;
    if (m_camera.following())
        fitView();
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
        // --screenshot: a mouse or a key must not move the map before the
        // picture is taken.
        if (!m_screenshot.empty() && event.type != sf::Event::Closed &&
            event.type != sf::Event::Resized)
            continue;
        switch (event.type)
        {
        case sf::Event::Closed:
            quit();
            return;
        case sf::Event::Resized:
            onResized(sf::Vector2u(event.size.width, event.size.height));
            break;
        case sf::Event::LostFocus:
            m_mouse.cancel();
            break;
        case sf::Event::KeyPressed:
            if (m_search.isOpen())
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
            if (!m_help.visible() && !onPanel(pixel))
                m_camera.zoom(event.mouseWheelScroll.delta > 0.0f ? 0.8f : 1.25f, pixel);
            break;
        }
        case sf::Event::MouseButtonPressed:
            onMousePressed(event.mouseButton);
            break;
        case sf::Event::MouseButtonReleased:
            onMouseReleased(event.mouseButton);
            break;
        case sf::Event::MouseMoved:
            onMouseMoved(sf::Vector2i(event.mouseMove.x, event.mouseMove.y));
            break;
        default:
            break;
        }
    }
    if (m_renderer.isOpen())
        updatePointer();
}

//------------------------------------------------------------------------------
void IslandedBrowserGUI::updatePointer()
{
    m_mouse.moveTo(sf::Mouse::getPosition(m_renderer));
    sf::Vector2i const pixel = m_mouse.position();
    bool const dragging = m_mouse.dragging();

    // A screenshot shows the graph only, wherever the mouse is.
    size_t const on_map = (m_screenshot.empty() && !dragging) ? pickAt(pixel)
                                                              : BookmarkNode::none;
    size_t const in_panel = (!m_help.visible() && !dragging) ? m_panel.rowAt(pixel)
                                                             : BookmarkNode::none;
    m_selection.hover(on_map, in_panel);
    m_selection.updateFocus(m_search.matches());

    if (dragging)
        m_cursor.show(MouseCursor::Shape::Move);
    else if (on_map != BookmarkNode::none || in_panel != BookmarkNode::none)
        m_cursor.show(MouseCursor::Shape::Hand);
    else
        m_cursor.show(MouseCursor::Shape::Arrow);
}

//------------------------------------------------------------------------------
void IslandedBrowserGUI::onResized(sf::Vector2u size)
{
    m_camera.resize(size);
    if (m_camera.following())
        fitView();
}

//------------------------------------------------------------------------------
void IslandedBrowserGUI::onMousePressed(sf::Event::MouseButtonEvent const& button)
{
    sf::Vector2i const pixel(button.x, button.y);
    if (button.button == sf::Mouse::Middle)
    {
        // Open at once what is under the mouse, on the map or in the panel.
        size_t const node = pickAt(pixel);
        open(node != BookmarkNode::none ? node : m_selection.hoveredRow());
        return;
    }
    m_mouse.press(button.button, pixel);
}

//------------------------------------------------------------------------------
void IslandedBrowserGUI::onMouseReleased(sf::Event::MouseButtonEvent const& button)
{
    if (m_mouse.release(button.button))
    {
        bool const control = sf::Keyboard::isKeyPressed(sf::Keyboard::LControl) ||
                             sf::Keyboard::isKeyPressed(sf::Keyboard::RControl);
        onClick(sf::Vector2i(button.x, button.y), control);
    }
}

//------------------------------------------------------------------------------
void IslandedBrowserGUI::onMouseMoved(sf::Vector2i pixel)
{
    // The map is not dragged when the press started on the help or a panel.
    bool const allowed = !m_help.visible() && !onPanel(m_mouse.pressedAt());
    sf::Vector2i previous;
    if (m_mouse.drag(pixel, allowed, previous))
        m_camera.drag(previous, pixel);
}

//------------------------------------------------------------------------------
void IslandedBrowserGUI::onClick(sf::Vector2i pixel, bool control)
{
    // A click anywhere closes the help.
    if (m_help.visible())
    {
        m_help.hide();
        return;
    }

    // A click on a line of the details panel goes to its node.
    if (onPanel(pixel))
    {
        size_t const row = m_panel.rowAt(pixel);
        if (row == BookmarkNode::none)
            return;
        if (control)
        {
            open(row);
        }
        else
        {
            select(row);
            centerOn(row);
        }
        return;
    }

    size_t const node = pickAt(pixel);
    bool const double_click = m_mouse.doubleClick(node);
    if (node == BookmarkNode::none)
        select(BookmarkNode::none); // Click on the sea.
    else if (control)
        open(node);
    else if (!double_click)
        select(node);
    else if (m_island.bookmarks().nodes[node].isBookmark())
        open(node);
    else
        zoomOn(node);
}

//------------------------------------------------------------------------------
void IslandedBrowserGUI::onKeyPressed(sf::Event::KeyEvent const& key)
{
    sf::Vector2f const window(m_renderer.getSize());
    sf::Vector2i const center(window / 2.0f);
    switch (key.code)
    {
    case sf::Keyboard::Escape:
        // Close what is open, one thing at a time, then quit.
        if (m_help.visible())
            m_help.hide();
        else if (m_selection.selected() != BookmarkNode::none)
            select(BookmarkNode::none);
        else
            quit();
        break;
    case sf::Keyboard::Q:
        quit();
        break;
    case sf::Keyboard::H:
    case sf::Keyboard::F1:
        m_help.toggle();
        break;
    case sf::Keyboard::F:
        if (key.control)
        {
            m_search.open();
            m_help.hide();
        }
        else
        {
            m_camera.follow();
            fitView();
        }
        break;
    case sf::Keyboard::Enter:
    case sf::Keyboard::O:
        open(m_selection.selected());
        break;
    case sf::Keyboard::Z:
        if (m_selection.selected() != BookmarkNode::none)
            zoomOn(m_selection.selected());
        break;
    case sf::Keyboard::Space:
        m_paused = !m_paused;
        m_toast.show(m_paused ? "Layout paused" : "Layout resumed");
        break;
    case sf::Keyboard::R:
        // The land was made around the old positions.
        m_terrain.clear();
        m_island.layout().restart();
        m_camera.follow();
        m_paused = false;
        break;
    case sf::Keyboard::L:
        m_map.toggleLinks();
        break;
    case sf::Keyboard::N:
        m_map.toggleNames();
        break;
    case sf::Keyboard::T:
        m_show_terrain = !m_show_terrain;
        if (m_show_terrain && m_island.layout().isRunning())
            m_toast.show("The terrain appears once the layout is stable");
        break;
    case sf::Keyboard::S:
        saveScreenshot("islanded-browser.png");
        break;
    // Arrows move the map by a tenth of the window.
    case sf::Keyboard::Left:
        m_camera.pan(sf::Vector2f(-0.1f * window.x, 0.0f));
        break;
    case sf::Keyboard::Right:
        m_camera.pan(sf::Vector2f(0.1f * window.x, 0.0f));
        break;
    case sf::Keyboard::Up:
        m_camera.pan(sf::Vector2f(0.0f, -0.1f * window.y));
        break;
    case sf::Keyboard::Down:
        m_camera.pan(sf::Vector2f(0.0f, 0.1f * window.y));
        break;
    case sf::Keyboard::Add:
        m_camera.zoom(0.8f, center);
        break;
    case sf::Keyboard::Subtract:
        m_camera.zoom(1.25f, center);
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
        m_search.close();
        break;
    case sf::Keyboard::Enter:
        if (key.control)
            open(m_selection.selected());
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
void IslandedBrowserGUI::onTextEntered(sf::Uint32 unicode)
{
    if (m_search.isOpen())
    {
        m_search.type(unicode);
        return;
    }

    // These characters are at different places depending on the keyboard
    // (QWERTY, AZERTY...): they are read as typed text, not as keys.
    sf::Vector2i const center(sf::Vector2f(m_renderer.getSize()) / 2.0f);
    switch (unicode)
    {
    case '/':
        m_search.open();
        m_help.hide();
        break;
    case '+':
    case '=':
        m_camera.zoom(0.8f, center);
        break;
    case '-':
        m_camera.zoom(1.25f, center);
        break;
    case '?':
        m_help.toggle();
        break;
    default:
        break;
    }
}

// =============================================================================
// Frame
// =============================================================================

//------------------------------------------------------------------------------
void IslandedBrowserGUI::update(const float dt)
{
    ForceDirectedGraph& layout = m_island.layout();
    if (!m_paused)
        layout.update();
    if (m_camera.following() && layout.isRunning())
        fitView();
    m_camera.update(dt);
    updateTerrain();
}

//------------------------------------------------------------------------------
void IslandedBrowserGUI::draw()
{
    if (!m_renderer.isOpen())
        return;

    // The map moves and zooms with the camera...
    m_renderer.setView(m_camera.world());
    if (m_show_terrain)
        m_terrain_painter.draw(m_renderer);
    m_map.drawMap(m_search.matches());

    // ...the texts and the panels stay in place on the screen.
    m_renderer.setView(m_camera.screen());
    m_map.drawNames();
    if (m_screenshot.empty())
    {
        size_t const hovered = m_selection.hovered();
        m_panel.draw(m_selection, m_mouse.position());
        m_search.draw();
        m_status.draw(mode(), m_paused);
        m_tooltip.draw(hovered, hovered == m_selection.selected(), m_mouse.position());
        m_toast.draw();
        m_help.draw();
    }

    // --screenshot: wait until the map stops moving, save it, quit.
    if (!m_screenshot.empty() && !m_island.layout().isRunning())
    {
        saveScreenshot(m_screenshot);
        m_screenshot.clear();
        quit();
    }
}
