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
//! \brief What the command line asks.
//------------------------------------------------------------------------------
struct Options
{
    //! \brief Folder of the files (-d), file to draw (-i).
    std::string dir;
    std::string input;
    //! \brief --screenshot FILE, --layout [FILE].
    std::string screenshot;
    std::string layout;
    bool write_layout = false;
    bool terrain = false;
};

//------------------------------------------------------------------------------
void usage(char const* program)
{
    std::cout
        << "Usage: " << program << " [-d DIR] [-i FILE] [--terrain] [--screenshot FILE] [--layout [FILE]]\n"
        << "Draw DIR/organization.json as islands, or DIR/bookmarks.json as its Firefox\n"
        << "folders when there is no organization yet.\n"
        << "  -d, --dir DIR      folder of the files (default: private/, or examples/\n"
        << "                     when private/ is empty)\n"
        << "  -i, --input FILE   draw this file instead (an organization.json or a\n"
        << "                     Firefox bookmarks.json); FILE alone works too\n"
        << "  --terrain          draw the land under the graph: relief, Voronoi cells,\n"
        << "                     rivers (key T)\n"
        << "  --screenshot FILE  save the window as a PNG once the layout is stable, quit\n"
        << "  --layout [FILE]    no window: compute the layout and write the position of\n"
        << "                     every node in FILE (default: DIR/layout.json), for the\n"
        << "                     3D archipelago of Compages, then quit\n"
        << "Mouse: hover shows a node, click selects it, double-click opens a bookmark\n"
        << "       or zooms on a folder, middle click opens, drag pans, wheel zooms.\n"
        << "Keys: H help, / search, Enter open, Z zoom, F fit, Space pause, R restart,\n"
        << "      L links, N names, T terrain, S screenshot, Esc unselect then quit, Q quit.\n";
}

//------------------------------------------------------------------------------
bool exists(std::string const& path)
{
    return std::ifstream(path).good();
}

//------------------------------------------------------------------------------
//! \brief Read the command line. Return false on a mistake.
//------------------------------------------------------------------------------
bool parse(int argc, char* argv[], Options& options)
{
    for (int i = 1; i < argc; ++i)
    {
        std::string const arg(argv[i]);
        bool const has_value = (i + 1 < argc);
        bool const next_is_value = has_value && argv[i + 1][0] != '-';
        if ((arg == "-d" || arg == "--dir") && has_value)
            options.dir = argv[++i];
        else if ((arg == "-i" || arg == "--input") && has_value)
            options.input = argv[++i];
        else if (arg == "--terrain")
            options.terrain = true;
        else if (arg == "--screenshot" && has_value)
            options.screenshot = argv[++i];
        else if (arg == "--layout")
        {
            options.write_layout = true;
            if (next_is_value)
                options.layout = argv[++i];
        }
        else if (!arg.empty() && arg[0] != '-' && options.input.empty())
            options.input = arg;
        else
            return false;
    }
    return true;
}

//------------------------------------------------------------------------------
//! \brief Fill what was not given. The folder is the one of -d, else the one
//! of the input file, else private/ (examples/ when private/ is empty). The
//! input is the organization of the folder, else its Firefox bookmarks.
//------------------------------------------------------------------------------
void complete(Options& options)
{
    if (options.dir.empty() && !options.input.empty())
    {
        size_t const slash = options.input.find_last_of('/');
        options.dir = (slash == std::string::npos) ? "." : options.input.substr(0u, slash);
    }
    if (options.dir.empty())
    {
        bool const empty = !exists("private/organization.json") && !exists("private/bookmarks.json");
        options.dir = empty ? "examples" : "private";
        if (empty)
            std::cout << "private/ is empty: drawing the example of examples/" << std::endl;
    }
    if (options.input.empty())
    {
        options.input = options.dir + "/organization.json";
        if (!exists(options.input))
            options.input = options.dir + "/bookmarks.json";
        if (!exists(options.input))
            throw std::runtime_error("neither organization.json nor bookmarks.json in " + options.dir +
                                     "/: run scripts/firefox_export.py, or use -d examples");
    }
    if (options.write_layout && options.layout.empty())
        options.layout = options.dir + "/layout.json";
}

} // namespace

//------------------------------------------------------------------------------
int main(int argc, char* argv[])
{
    for (int i = 1; i < argc; ++i)
    {
        std::string const arg(argv[i]);
        if (arg == "-h" || arg == "--help")
        {
            usage(argv[0]);
            return 0;
        }
    }
    Options options;
    if (!parse(argc, argv, options))
    {
        usage(argv[0]);
        return 1;
    }

    // Browsers started by a click are not waited for: let the kernel reap them.
    std::signal(SIGCHLD, SIG_IGN);

    try
    {
        complete(options);
        if (options.write_layout)
        {
            Bookmarks const bookmarks = Bookmarks::load(options.input);
            ForceDirectedGraph graph;
            graph.reset(bookmarks);
            size_t const steps = graph.run();
            writeLayout(options.layout, options.input, bookmarks, graph);
            std::cout << options.layout << ": " << graph.vertices().size() << " nodes after "
                      << steps << " steps" << std::endl;
            return 0;
        }
        Application app(WINDOWS_WIDTH, WINDOWS_HEIGHT, "Islanded Browser");
        IslandedBrowserGUI& gui = app.create<IslandedBrowserGUI>(
            "IslandedBrowserGUI", options.input, options.screenshot, options.terrain);
        app.loop(gui);
    }
    catch (std::exception const& error)
    {
        std::cerr << "IslandedBrowser: " << error.what() << std::endl;
        return 1;
    }
    return 0;
}
