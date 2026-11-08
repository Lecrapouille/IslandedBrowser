/* *****************************************************************************
** MIT License
**
** Copyright (c) 2022 Quentin Quadrat
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

#ifndef SETTINGS_HPP
#  define SETTINGS_HPP

//! \brief Windows dimensions
#  define WINDOWS_WIDTH  1920
//! \brief Windows dimensions
#  define WINDOWS_HEIGHT 1080
//! \brief Text size
#  define CHARACTER_SIZE 20
//! \brief If set draw arrow heads for graph edges
#  undef DRAW_ARROW_HEAD
//! \brief Arrow head size for drawin graph edges
#  define ARROW_HEAD_SIZE 10.0f
//! \brief Arrow tail size for drawin graph edges
#  define ARROW_TAIL_SIZE 8.5f
//! \brief Arrow head offset
#  define ARROW_HEAD_OFFSET 8.5f
//! \brief Radius of the circle depicting graph nodes
#  define NODE_RADIUS 5.0f
//! \brief Number of points composing the circle for each nodes
#  define CIRCLE_COUNT_POINTS 8
//! \brief Layout border
#  define LAYOUT_BORDER_X NODE_RADIUS
//! \brief Layout border
#  define LAYOUT_BORDER_Y NODE_RADIUS
//! \brief Layout color
#  define LAYOUT_COLOR sf::Color::Red
//! \brief Color of folder nodes
#  define FOLDER_COLOR sf::Color::Red
//! \brief Color of bookmark nodes
#  define BOOKMARK_COLOR sf::Color::Blue
//! \brief The name of your favorite browser (searched in the PATH)
#  define BROWSER_NAME "firefox"
//! \brief Maximum number of tabs opened by a click on a folder
#  define MAX_TABS 15
//! \brief Radius (in pixels at zoom 1) of bookmark, community and island nodes
#  define BOOKMARK_RADIUS 4.0f
#  define FOLDER_RADIUS 8.0f
#  define ISLAND_RADIUS 14.0f
//! \brief Distance in pixels under which the mouse points a node
#  define PICK_DISTANCE 10.0f
//! \brief Background color (sea)
#  define SEA_COLOR sf::Color(18, 32, 48)
//! \brief Tree edges and similarity links
#  define TREE_EDGE_COLOR sf::Color(200, 200, 200, 70)
#  define SEA_ROUTE_COLOR sf::Color(90, 170, 255, 60)
//! \brief Size of the small texts (tooltip, panel, status bar)
#  define SMALL_CHARACTER_SIZE 15
//! \brief Width in pixels of the panel describing the selected node
#  define PANEL_WIDTH 420.0f
//! \brief Height in pixels of the status bar at the bottom of the window
#  define STATUSBAR_HEIGHT 26.0f
//! \brief Background of the panels (tooltip, details, help, status bar)
#  define PANEL_COLOR sf::Color(10, 18, 28, 225)
#  define PANEL_BORDER_COLOR sf::Color(90, 120, 150, 160)
//! \brief Texts of the panels
#  define TEXT_COLOR sf::Color(235, 235, 235)
#  define DIM_TEXT_COLOR sf::Color(150, 160, 175)
#  define URL_COLOR sf::Color(120, 190, 255)
//! \brief Seconds a notification stays before fading out
#  define TOAST_DURATION 3.0f
//! \brief Ring drawn around the selected node
#  define SELECTION_COLOR sf::Color(255, 220, 80)
//! \brief How much the nodes out of the focus fade into the sea (0..1)
#  define DIM_FACTOR 0.75f
//! \brief Maximum delay in seconds between the two clicks of a double click
#  define DOUBLE_CLICK_DELAY 0.4f
//! \brief Mouse moves (pixels) under which a press is a click, not a drag
#  define DRAG_THRESHOLD 4
//! \brief Zoom (pixels per world unit) from which the community names, then
//! the bookmark titles, are written
#  define COMMUNITY_LABEL_ZOOM 1.2f
#  define BOOKMARK_LABEL_ZOOM 3.0f
//! \brief Maximum number of children or similar bookmarks listed in the panel
#  define PANEL_MAX_ROWS 16

#endif
