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

#ifndef ISLANDED_BROWSER_GUI_HPP
#  define ISLANDED_BROWSER_GUI_HPP

#  include "Application.hpp"
#  include "Drawable.hpp"
#  include "IslandedBrowser.hpp"
#  include <string>
#  include <vector>

// ****************************************************************************
//! \brief Window showing the bookmark graph.
//!
//! Mouse:
//! - hover: tooltip near the cursor, the community of the node is
//!   highlighted and the rest of the map fades;
//! - left click: select the node, its details are shown in the right panel
//!   (the entries of the panel can be clicked too); click on the sea to
//!   unselect;
//! - double click: open a bookmark in the browser, zoom on a folder;
//! - middle click or Ctrl + click: open in the browser (a folder opens its
//!   first MAX_TABS bookmarks);
//! - left or right drag: pan; wheel: zoom around the cursor.
//! Keys: H help, / search, Enter open, Z zoom on the selection, F fit,
//! arrows pan, +/- zoom, Space pause, R restart the layout, L links and
//! rivers, N names, S screenshot, Esc close/unselect then quit.
// ****************************************************************************
class IslandedBrowserGUI: public Application::GUI
{
public:

    //-------------------------------------------------------------------------
    //! \param[in] bookmarks_path organization.json or Firefox backup.
    //! \param[in] screenshot when not empty, save the window to this PNG
    //! file once the layout has converged, then quit.
    //-------------------------------------------------------------------------
    IslandedBrowserGUI(Application& application, const char* name,
                       std::string const& bookmarks_path,
                       std::string const& screenshot = std::string());

private: // Derived from Application::GUI

    virtual void draw() override;
    virtual void update(const float dt) override;
    virtual void handleInput() override;
    virtual bool running() const override { return m_running; }
    virtual void activate() override;
    virtual void deactivate() override {}
    virtual void create() override {}
    virtual void release() override {}

private: // Events

    void onKeyPressed(sf::Event::KeyEvent const& key);
    void onSearchKey(sf::Event::KeyEvent const& key);
    void onTextEntered(sf::Uint32 unicode);
    void onLeftReleased(sf::Vector2i pixel, bool control);
    void quit();

private: // Actions

    void select(size_t node);
    void open(size_t node);
    void zoomOn(size_t node);
    void centerOn(size_t node);
    void nextMatch(int direction);
    void updateMatches();
    void updateFocus();
    void updateCursor();

private: // View

    void fitView();
    sf::Vector2f fitSize(sf::FloatRect box) const;
    void zoom(float factor, sf::Vector2i pixel);
    void pan(sf::Vector2f pixels);
    //! \brief World units per pixel.
    float scale() const;
    //! \brief Node under the pixel, none when the pixel is on a panel.
    size_t pickAt(sf::Vector2i pixel) const;
    bool onPanel(sf::Vector2i pixel) const;
    void saveScreenshot(std::string const& path);

private: // Drawing

    void drawEdges();
    void drawNodes();
    void drawNames();
    void drawRivers();
    void drawTooltip();
    void drawPanel();
    void drawSearch();
    void drawStatusBar();
    void drawHelp();
    void drawToast();
    void toast(std::string const& message);
    void computeRivers();
    sf::Color colorOf(ForceDirectedGraph::Vertex const& vertex) const;
    sf::Color islandColor(size_t node) const;
    bool dimmed(size_t node) const;
    sf::String kindOf(size_t node) const;
    //! \brief Write a text in interface coordinates, return its width.
    float text(sf::String const& string, sf::Vector2f position, unsigned size,
               sf::Color color, bool bold = false);
    float textWidth(sf::String const& string, unsigned size, bool bold = false);
    //! \brief Cut a text in lines no wider than width pixels.
    std::vector<sf::String> wrap(sf::String const& string, float width,
                                 unsigned size, size_t max_lines);
    void box(sf::FloatRect const& rect, sf::Color fill = PANEL_COLOR,
             sf::Color border = PANEL_BORDER_COLOR);

    //! \brief Aggregated similarity between two islands (vertex indices of
    //! the island nodes), strength normalized in ]0, 1].
    struct River
    {
        size_t a;
        size_t b;
        float strength;
    };

    //! \brief Clickable line of the panel.
    struct Row
    {
        sf::FloatRect box;
        size_t node;
    };

private:

    IslandedBrowser m_island;
    //! \brief World view (graph) and interface view (texts).
    sf::View m_world;
    sf::View m_interface;
    //! \brief Short notification (opened tabs, screenshot...).
    sf::String m_toast;
    sf::Clock m_toast_clock;
    sf::Font m_font;
    sf::Text m_text;
    bool m_has_font = false;
    //! \brief One color per island.
    std::vector<sf::Color> m_palette;
    //! \brief Strongest relations between islands.
    std::vector<River> m_rivers;

    //! \brief Node under the mouse, selected node.
    size_t m_hovered = BookmarkNode::none;
    size_t m_selected = BookmarkNode::none;
    //! \brief Bookmarks similar to the selected one.
    std::vector<std::pair<size_t, float>> m_related;
    //! \brief Nodes kept bright (the others fade): indexed by node.
    std::vector<char> m_focus;
    bool m_has_focus = false;
    //! \brief Clickable lines of the panel, its box (interface coordinates).
    std::vector<Row> m_rows;
    sf::FloatRect m_panel_box;
    size_t m_hovered_row = BookmarkNode::none;

    //! \brief Mouse buttons.
    sf::Vector2i m_mouse;
    sf::Vector2i m_press_pixel;
    sf::Vector2i m_drag_last;
    bool m_left_pressed = false;
    bool m_right_pressed = false;
    bool m_dragging = false;
    sf::Clock m_click_clock;
    size_t m_last_click = BookmarkNode::none;
    sf::Cursor m_arrow_cursor;
    sf::Cursor m_hand_cursor;
    sf::Cursor m_move_cursor;
    int m_cursor = -1;
    bool m_has_cursors = false;

    //! \brief The view follows the layout until the user zooms or pans.
    bool m_auto_fit = true;
    //! \brief Smooth move of the view toward a target.
    bool m_animating = false;
    sf::Vector2f m_target_center;
    sf::Vector2f m_target_size;

    //! \brief Search box.
    bool m_searching = false;
    bool m_skip_text = false;
    sf::String m_query;
    std::vector<size_t> m_matches;
    size_t m_match = 0u;
    sf::FloatRect m_search_box;
    sf::Clock m_caret_clock;

    bool m_paused = false;
    bool m_show_links = true;
    bool m_show_names = true;
    bool m_show_help = false;
    std::string m_file_name;
    std::string m_screenshot;
    bool m_running = true;
};

#endif
