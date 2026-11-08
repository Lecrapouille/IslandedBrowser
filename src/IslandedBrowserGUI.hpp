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
//! Mouse: hover shows the title, left click opens the bookmark (or the
//! bookmarks of a folder), wheel zooms, right button drag pans.
//! Keys: Space pause, R restart the layout, F fit the view, L similarity
//! links and rivers, N island names, S screenshot, Escape quit.
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

private:

    void fitView();
    void zoom(float factor, sf::Vector2i pixel);
    void click(sf::Vector2i pixel);
    void saveScreenshot(std::string const& path);
    void drawEdges();
    void drawNodes();
    void drawNames();
    void drawRivers();
    void computeRivers();
    sf::Color colorOf(ForceDirectedGraph::Vertex const& vertex) const;

    //! \brief Aggregated similarity between two islands (vertex indices of
    //! the island nodes), strength normalized in ]0, 1].
    struct River
    {
        size_t a;
        size_t b;
        float strength;
    };

private:

    IslandedBrowser m_island;
    //! \brief World view (graph) and interface view (texts).
    sf::View m_world;
    sf::View m_interface;
    MessageBar m_message_bar;
    sf::Font m_font;
    bool m_has_font = false;
    //! \brief One color per island.
    std::vector<sf::Color> m_palette;
    //! \brief Strongest relations between islands.
    std::vector<River> m_rivers;
    //! \brief Node under the mouse.
    size_t m_hovered = BookmarkNode::none;
    sf::Vector2i m_drag_origin;
    bool m_dragging = false;
    //! \brief The view follows the layout until the user zooms or pans.
    bool m_auto_fit = true;
    bool m_paused = false;
    bool m_show_links = true;
    bool m_show_names = true;
    std::string m_screenshot;
    bool m_running = true;
};

#endif
