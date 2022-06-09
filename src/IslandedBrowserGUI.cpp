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
//
// For more information, please refer to <https://unlicense.org>

#include "IslandedBrowserGUI.hpp"
#include "Drawable.hpp"
#include <iostream>
#include <cstdlib>

//------------------------------------------------------------------------------
IslandedBrowserGUI::IslandedBrowserGUI(Application& application, const char* name)
    : Application::GUI(application, name, sf::Color::White),
      m_island(sf::Vector2f(renderer().getSize()))
{
    // Ideally make two views: one for the scene and another view for the interface.
    m_view = renderer().getDefaultView();
    m_view.setSize(float(application.width()), float(application.height()));
    m_message_bar.font("data/font.ttf");
}

//------------------------------------------------------------------------------
void IslandedBrowserGUI::activate()
{
    m_renderer.setView(m_view);
}

//------------------------------------------------------------------------------
void IslandedBrowserGUI::deactivate()
{}

//------------------------------------------------------------------------------
void IslandedBrowserGUI::create()
{}

//------------------------------------------------------------------------------
void IslandedBrowserGUI::release()
{}

//------------------------------------------------------------------------------
void IslandedBrowserGUI::handleInput()
{
    sf::Event event;

    m_mouse = sf::Mouse::getPosition(renderer());

    // Show the URL of the node pointed by the mouse cursor
    std::string const& title = m_island.getTitle(m_mouse);
    if (!title.empty())
    {
        m_message_bar.entry(title, MESSAGEBAR_COLOR);
    }

    while (m_renderer.pollEvent(event))
    {
        switch (event.type)
        {
        case sf::Event::Closed:
            m_renderer.close();
            break;
        case sf::Event::KeyPressed:
            if (event.key.code == sf::Keyboard::Escape)
            {
                m_renderer.close();
            }
            break;
        case sf::Event::MouseButtonPressed:
            {
                // TODO call several times Firefox because
                std::string const& url = m_island.getURL(m_mouse);
                if (!url.empty())
                {
                    std::string command = BROWSER_NAME + url;
                    std::cout << "command: " << command << std::endl;
                    system(command.c_str());
                }
            }
            break;
        default:
            break;
        }
    }
}

//------------------------------------------------------------------------------
void IslandedBrowserGUI::update(const float dt)
{
    //std::cout << "FPS:" << 1.0f / dt << std::endl;
    m_island.forceDirectedGraph();
}

//------------------------------------------------------------------------------
void IslandedBrowserGUI::draw()
{
    // Scene view
    for (auto const& it: m_island.vertices())
    {
        Circle circle(it.position, NODE_RADIUS, it.color);
        renderer().draw(circle);

        for (auto const& n: it.neighbors)
        {
            Arrow arrow(it.position.x, it.position.y,
                        n.position->x, n.position->y);
            renderer().draw(arrow);
        }
    }

    // Interface view
    m_message_bar.size(m_renderer.getSize());
    m_renderer.draw(m_message_bar);
}
