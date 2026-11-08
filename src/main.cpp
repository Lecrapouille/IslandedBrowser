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


#include "IslandedBrowserGUI.hpp"
#include "LayoutExport.hpp"
#include <csignal>
#include <fstream>
#include <iostream>
#include <stdexcept>
#include <string>

namespace {

//------------------------------------------------------------------------------
void usage(char const* program)
{
    std::cout
        << "Usage: " << program << " [bookmarks.json] [--screenshot file.png] [--layout file.json]\n"
        << "  bookmarks.json  private/organization.json (bookmarks/organize.py)\n"
        << "                  or a Firefox backup (bookmarks/firefox_export.py).\n"
        << "                  Default: the first existing of private/organization.json,\n"
        << "                  private/bookmarks.json, bookmarks/example.json.\n"
        << "  --screenshot    save the window once the layout has converged, then quit.\n"
        << "  --layout        compute the layout without a window, write the position\n"
        << "                  of every node in file.json (for Compages 10e_Archipelago),\n"
        << "                  then quit.\n"
        << "Mouse: hover shows a node, click selects it, double-click opens a bookmark\n"
        << "       or zooms on a folder, middle click opens, drag pans, wheel zooms.\n"
        << "Keys: H help, / search, Enter open, Z zoom, F fit, Space pause, R restart,\n"
        << "      L links, N names, S screenshot, Esc unselect then quit, Q quit.\n";
}

//------------------------------------------------------------------------------
std::string defaultBookmarks()
{
    char const* const candidates[] = {
        "private/organization.json", "private/bookmarks.json", "bookmarks/example.json"
    };
    for (char const* path: candidates)
    {
        if (std::ifstream(path).good())
            return path;
    }
    throw std::runtime_error("no bookmark file given and none found in private/ or bookmarks/");
}

} // namespace

//------------------------------------------------------------------------------
int main(int argc, char* argv[])
{
    std::string path;
    std::string screenshot;
    std::string layout;
    for (int i = 1; i < argc; ++i)
    {
        std::string const arg(argv[i]);
        if (arg == "-h" || arg == "--help")
        {
            usage(argv[0]);
            return 0;
        }
        if (arg == "--screenshot" && i + 1 < argc)
            screenshot = argv[++i];
        else if (arg == "--layout" && i + 1 < argc)
            layout = argv[++i];
        else if (!arg.empty() && arg[0] != '-' && path.empty())
            path = arg;
        else
        {
            usage(argv[0]);
            return 1;
        }
    }

    // Browsers started by a click are not waited for: let the kernel reap them.
    std::signal(SIGCHLD, SIG_IGN);

    try
    {
        if (path.empty())
            path = defaultBookmarks();
        if (!layout.empty())
        {
            Bookmarks const bookmarks = Bookmarks::load(path);
            ForceDirectedGraph graph;
            graph.reset(bookmarks);
            size_t const steps = graph.run();
            writeLayout(layout, path, bookmarks, graph);
            std::cout << layout << ": " << graph.vertices().size() << " nodes after "
                      << steps << " steps" << std::endl;
            return 0;
        }
        Application app(WINDOWS_WIDTH, WINDOWS_HEIGHT, "Islanded Browser");
        IslandedBrowserGUI& gui = app.create<IslandedBrowserGUI>(
            "IslandedBrowserGUI", path, screenshot);
        app.loop(gui);
    }
    catch (std::exception const& error)
    {
        std::cerr << "IslandedBrowser: " << error.what() << std::endl;
        return 1;
    }
    return 0;
}
