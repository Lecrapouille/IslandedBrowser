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
#  include "Camera.hpp"
#  include "Colors.hpp"
#  include "DetailsPanel.hpp"
#  include "HelpScreen.hpp"
#  include "IslandedBrowser.hpp"
#  include "MapPainter.hpp"
#  include "MouseCursor.hpp"
#  include "MouseTracker.hpp"
#  include "Rivers.hpp"
#  include "SearchBox.hpp"
#  include "Selection.hpp"
#  include "StatusBar.hpp"
#  include "TextPainter.hpp"
#  include "Toast.hpp"
#  include "Tooltip.hpp"
#  include <string>

// ****************************************************************************
//! \brief The window showing the bookmark map.
//!
//! This class is the conductor: it receives the mouse and keyboard events,
//! decides what they mean, and asks the other classes to do the work (the
//! Camera moves the view, the MapPainter draws the map, the panels draw
//! themselves...).
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

private: // Called by Application at each frame

    virtual void handleInput() override;
    virtual void update(const float dt) override;
    virtual void draw() override;
    virtual bool running() const override { return m_running; }
    virtual void activate() override;
    virtual void deactivate() override {}
    virtual void create() override {}
    virtual void release() override {}

private: // What the events mean

    void onResized(sf::Vector2u size);
    void onMousePressed(sf::Event::MouseButtonEvent const& button);
    void onMouseReleased(sf::Event::MouseButtonEvent const& button);
    void onMouseMoved(sf::Vector2i pixel);
    //! \brief A left click (not a drag) ended at this pixel.
    void onClick(sf::Vector2i pixel, bool control);
    //! \brief Keys when the search field is closed.
    void onKeyPressed(sf::Event::KeyEvent const& key);
    //! \brief Keys while the search field is open.
    void onSearchKey(sf::Event::KeyEvent const& key);
    //! \brief Typed characters: the search text, or the shortcuts that
    //! depend on the keyboard layout (/ + - ?).
    void onTextEntered(sf::Uint32 unicode);
    //! \brief Find what is under the mouse, then update the highlight and
    //! the pointer.
    void updatePointer();

private: // Actions

    void select(size_t node);
    //! \brief Open a bookmark in the web browser; for a folder, its first
    //! MAX_TABS bookmarks.
    void open(size_t node);
    //! \brief Glide and zoom so the node and its content fill the window (a
    //! bookmark is shown with its community).
    void zoomOn(size_t node);
    //! \brief Glide to the node without changing the zoom.
    void centerOn(size_t node);
    //! \brief Visit the next (direction > 0) or previous search result.
    void nextMatch(int direction);
    //! \brief Show the whole map.
    void fitView();
    void saveScreenshot(std::string const& path);
    void quit();

private: // Questions

    //! \brief Node under the pixel; none when the pixel is on a panel, on the
    //! help or outside the window.
    size_t pickAt(sf::Vector2i pixel) const;
    //! \brief True when the pixel is on the status bar, the details panel or
    //! the search field: the map is not under the mouse there.
    bool onPanel(sf::Vector2i pixel) const;
    //! \brief Width in pixels hidden on the right by the details panel.
    float hiddenWidth() const;
    StatusBar::Mode mode() const;

private:

    //! \brief The bookmarks and their layout.
    IslandedBrowser m_island;
    TextPainter m_painter;
    Camera m_camera;
    Palette m_palette;
    Rivers m_rivers;
    Selection m_selection;
    MapPainter m_map;
    Tooltip m_tooltip;
    DetailsPanel m_panel;
    SearchBox m_search;
    StatusBar m_status;
    Toast m_toast;
    HelpScreen m_help;
    MouseTracker m_mouse;
    MouseCursor m_cursor;
    //! \brief The user stopped the layout (Space).
    bool m_paused = false;
    //! \brief See the constructor; cleared once the screenshot is saved.
    std::string m_screenshot;
    bool m_running = true;
};

#endif
