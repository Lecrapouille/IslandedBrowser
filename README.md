# 🏝️ Islanded Browser

[Islanded Browser](https://github.com/Lecrapouille/IslandedBrowser) turns your browser bookmarks (Firefox) into a 2D map of islands: one island per subject, regions per sub-subject, and sea routes between related islands. It started as a prototype for [Stigmee](https://github.com/stigmee), a 3D browser project, now discontinued. Here is a screenshot:

![islanded_browser](doc/islanded_browser.png)

The idea comes from [Chréage](https://chreage.befuse.com/content/WEBGL/ChreageAlpha0-3/index.html), the "mindscape" of Idriss J. Aberkane's PhD thesis, [Mind Ergonomy for the Knowledge Economy](https://chreage.befuse.com/content/WEBGL/ChreageAlpha0-3/index.html) (2016, chapter 11). Mind ergonomy is about shaping knowledge to fit the way our brain works, instead of forcing the brain to fit lists, folders and menus. Our memory is far better at places than at lists: we easily remember where things are in a town or a landscape, which is the trick of the "method of loci" (the memory palace). So Chréage draws knowledge as an archipelago to explore: a theme is an island, a topic is a region of it, and contents are laid out "as loci along a strand", as places along rivers. The islands follow Amit Patel's polygonal map generation, as the thesis does. A WebGL proof of concept was made by Gabriel Uribe (first with Unity), then taken up by Stigmee. Both projects are discontinued.

This project applies the idea to a real pile of knowledge: browser bookmarks. Many people (me first) keep hundreds of bookmarks saved without any folder. Islanded Browser sorts them automatically into islands > regions > bookmarks, draws the archipelago, and can write the result back into Firefox.

- This project has only been tested with Firefox bookmarks (Chrome should work the same way, but it is not done).
- Since Stigmee is a dead project, the 3D version lives in my renderer [Compages](https://github.com/Lecrapouille/Compages) (demo `10e_Archipelago`), fed by this project but used for testing my 3d renderer. Here is a screenshot:

![Compages](doc/3d.png)

## 📸 What it looks like

The same 118 public bookmarks of [examples/bookmarks.json](examples/bookmarks.json), drawn three ways:

| 1. Firefox folders | 2. Islands | 3. Archipelago |
|:---:|:---:|:---:|
| ![Bookmarks in their original Firefox folders](doc/folders.png) | ![Bookmarks sorted into islands](doc/islands.png) | ![Islands with their land: relief, regions and rivers](doc/archipelago.png) |

1. **Firefox folders**: the bookmarks as they are in Firefox, here a few catch-all folders.
2. **Islands**: the same bookmarks sorted by subject. Each island has its color, communities are the medium discs, and blue bands across the sea (sea routes) join related islands.
3. **Archipelago**: the same, with real land grown under the graph: coasts, relief whose summits are the big nodes, one region of cells per community, and rivers flowing down from the summits.

## 🚀 Try it (no Firefox needed)

Prerequisites: Python 3, GNU Make, g++, OpenMP and SFML 2.6. Run every command from the project folder.

```sh
git clone --recursive https://github.com/Lecrapouille/IslandedBrowser.git
cd IslandedBrowser
make

alias IslandedBrowser=./build/linux-gcc-normal/bin/IslandedBrowser

IslandedBrowser -i examples/bookmarks.json   # picture 1: the Firefox folders
IslandedBrowser -d examples                  # picture 2: the islands
IslandedBrowser -d examples --terrain        # picture 3: the archipelago
```

In the window, hover a node to see it, click it to see its details, double-click to open it in Firefox, and press `H` for all the controls.

## 🦊 With your own bookmarks: 4 steps

Each step reads the file written by the previous one. Your files go in `private/`, which git ignores because they hold your personal data. `examples/` holds the same files, with the same names, made from public bookmarks.

```
     1 export              2 sort                   3 look
Firefox ───> bookmarks.json ───> organization.json ───> IslandedBrowser
   ▲                                    │
   └──── organized.html / .json <───────┘
                    4 import
```

| Step | Command | Reads | Writes |
|------|---------|-------|--------|
| 1. Export | `./scripts/firefox_export.py` | Firefox (it can stay open) | `private/bookmarks.json` |
| 2. Sort | `./scripts/organize.py` | `private/bookmarks.json` | `private/organization.json` |
| 3. Look | `IslandedBrowser` | `private/organization.json` | (nothing) |
| 4. Import | `./scripts/firefox_import.py` | `private/organization.json` | `private/organized.html`, `private/organized.json` |

```sh
./scripts/firefox_export.py
./scripts/organize.py
IslandedBrowser
./scripts/firefox_import.py
```

**Fixing the sorting by hand.** Step 2 sorts the bookmarks automatically, so it makes mistakes: a bookmark can land in the wrong island, or a group can get a strange name made of keywords. You can correct it: open `private/organization.json`, change the `name` of a group to rename it, or edit the `constraints` section to move a bookmark, keep two bookmarks together or apart. Then run step 2 again: it redoes the sorting but keeps your corrections, even after a new export. The details are in *Correcting the result* in [doc/Pipeline.md](doc/Pipeline.md).

**Step 4, loading the result into Firefox** (library window, `Ctrl+Shift+O`, *Import and Backup*):

- **safe**: *Import Bookmarks from HTML...* > `private/organized.html`. Everything is added in one new folder, and your current bookmarks are kept.
- **replace**: *Backup...* first, then *Restore* > *Choose File...* > `private/organized.json`. This erases all your bookmarks and loads the islands.

## ⌨️ Command line

The four programs share the same options. Each one also has `--help`.

| Option | Meaning | Used by |
|--------|---------|---------|
| `-d DIR` | folder of the files (default: `private/`). Use `-d examples` for the example | all |
| `-i FILE` | read this file instead of the usual one of `DIR` | `organize.py`, `firefox_import.py`, `IslandedBrowser` |
| `-o FILE` | write this file instead of the usual one of `DIR` | `firefox_export.py`, `organize.py` |

The usual files of a folder:

| File | Written by | Content |
|------|------------|---------|
| `bookmarks.json` | `firefox_export.py` | your Firefox bookmarks, as they are |
| `organization.json` | `organize.py` | the islands, communities and similarity links, plus your corrections |
| `layout.json` | `IslandedBrowser --layout` | the position of every node, for the 3D archipelago of Compages |
| `organized.html`, `organized.json` | `firefox_import.py` | the islands as files Firefox can load |

`IslandedBrowser` draws `DIR/organization.json`, or `DIR/bookmarks.json` when there is no organization yet. Without `-d`, it uses `private/`, or `examples/` when `private/` is empty. Its own options:

| Option | Effect |
|--------|--------|
| `--terrain` | grow land under the graph (key `T` in the window) |
| `--screenshot FILE` | save the window as a PNG once the map stops moving, then quit |
| `--layout [FILE]` | no window: write the position of every node in `DIR/layout.json` (or `FILE`), then quit |

For example, the three pictures above and the example layout were made with:

```sh
./scripts/organize.py -d examples
IslandedBrowser -d examples --layout
IslandedBrowser -i examples/bookmarks.json --screenshot doc/folders.png
IslandedBrowser -d examples --screenshot doc/islands.png
IslandedBrowser -d examples --terrain --screenshot doc/archipelago.png
```

The options to tune the sorting (`--island-resolution`, `--fetch`...), the file formats and a troubleshooting guide are in [doc/Pipeline.md](doc/Pipeline.md).

## 🖱️ In the window

| Mouse | Action |
|-------|--------|
| hover | title and URL; the community lights up, the rest of the map fades |
| click | select: details in the right panel (its lines can be clicked) |
| double click | open a bookmark in Firefox, zoom on a folder |
| middle click, `Ctrl` + click | open in Firefox (a folder: its first 15 bookmarks) |
| drag, wheel | move, zoom |

| Key | Action |
|-----|--------|
| `H` | help (all the controls) |
| `/` | search the titles and URLs |
| `Enter` | open the selection in Firefox |
| `Z`, `F` | zoom on the selection, show the whole map |
| `T`, `L`, `N` | show or hide the terrain, the links, the names |
| `Space`, `R` | pause, restart the layout |
| `S` | screenshot (`islanded-browser.png`) |
| `Esc` | close, unselect, then quit |

## 🧮 Algorithms

This section explains, without math, how a pile of bookmarks becomes a map. The two big steps are **sorting** the bookmarks into subjects (Python, `scripts/organize.py`) and **drawing** them (C++, `src/`).

### 1. Sorting the bookmarks into subjects (`organize.py`)

**Turning a bookmark into words ("bag of words").** The script cannot read web pages the way we do, so it describes each bookmark as a list of words taken from its title, its tags, the page description, the URL, the site name and the folder where you saved it. Some sources count more than others: a word of the title counts 3 times, a word of the URL path only once. Words are reduced to a common form, so "shaders" and "shader" are the same word.

**Weighting the words (TF-IDF).** Not all words are useful. A word found in almost every bookmark ("http", "home") says nothing about the subject, so it weighs nothing. A word found in only a few bookmarks ("aikido", "Petri") is very telling, so it weighs a lot. This is called TF-IDF: *term frequency* (how often the word is in this bookmark) times *inverse document frequency* (how rare it is among all bookmarks).

**Comparing two bookmarks (cosine similarity).** Two bookmarks are similar when they share the same important words. The similarity goes from 0 (nothing in common) to 1 (same words with the same weights).

**Building a network ("k nearest neighbours").** Each bookmark is linked to its 10 most similar bookmarks, as long as they are similar enough (at least 0.12). This creates a network where related bookmarks hold hands. Keeping "the 10 closest" instead of "everything above a threshold" keeps the network connected even for subjects with little text.

**Finding the groups (Louvain).** In this network, some bookmarks are much more connected to each other than to the rest: these are the subjects. The Louvain method finds them like this: every bookmark starts alone in its own group, then each one moves to the neighbouring group where it has the most links, again and again until nobody wants to move. Each group then becomes a single "super bookmark" and the process starts again, which merges small groups into larger ones. The *resolution* setting decides how large the groups are allowed to grow. Louvain runs twice:

- with a low resolution over all the bookmarks, which gives the **islands** (big subjects);
- with a high resolution inside each island, which gives the **communities** (sub-subjects).

Then communities with more than 40 bookmarks are split again, and communities with fewer than 3 bookmarks join the group they have the most links with. Bookmarks with no similar bookmark at all go to the *Orphelins / À explorer* island. The same input always gives the same result.

### 2. Placing the islands on the map (`ForceDirectedGraph`)

**Springs and magnets (force-directed layout, Fruchterman and Reingold).** Picture every bookmark as a small magnet that pushes all the others away, so nothing overlaps. Then add springs: each bookmark is tied to its community, each community to its island, and similar bookmarks are tied by weaker springs. Springs pull their ends together, magnets push everything apart, and a light gravity keeps the islands from drifting away. At each step every node moves a little in the direction it is pushed or pulled, until the forces balance out. A spring between two different islands is 30 times weaker than a spring inside an island: related islands come closer to each other but do not merge.

**Cooling down (simulated annealing).** At first the nodes may jump far, so the map can untangle quickly. Each step, the largest allowed jump gets a little smaller (the "temperature" cools down). When it is tiny, the map stops moving: the status bar then shows *layout stable*.

**A good start (sunflower spiral).** Starting from random positions would give a messy map. Instead, each island starts in its own disc, and the discs are laid out like the seeds of a sunflower (each one turned by the "golden angle" from the previous one), biggest islands in the center. Islands are therefore apart from the very first step.

**Speed.** Every node is pushed by every other node, so the work grows with the square of the number of bookmarks. The computation is shared among all the processor cores (OpenMP): with 2500 bookmarks, the layout settles in about 0.6 s.

### 3. Drawing the map (`MapPainter`, `SeaRoutes`, `Palette`)

**Sea routes (`SeaRoutes`).** Some similar bookmarks are on different islands. For every pair of islands, the strengths of the similarity links between them are added up. Only the strongest pairs are kept (about two per island) and drawn as blue bands across the sea: the wider the band, the more the two islands have in common. They are not rivers: the rivers are on the land (see 5).

**Colors (`Palette`).** Each island gets its own color. Colors are picked around the color wheel in steps of 0.618 of a turn (the golden ratio). Two islands that follow each other therefore never get similar colors, whatever their number. Bookmarks use a lighter shade of their island color, like sand around a hill.

**Fast drawing (`Meshes`).** A graphics card draws triangles very fast, but each separate drawing order is slow. So every disc, ring and band is cut into triangles, all of them are put into one big list, and that list is drawn in a single order.

**Constant sizes.** The discs and the texts keep the same size on the screen, whatever the zoom: zooming in spreads the nodes apart instead of making them bigger. Island names are always written. Community names, then bookmark titles, only appear once the zoom leaves enough room for them.

**Highlighting (`Selection`).** When the mouse points at a node, its whole community and the folders above it stay bright, and everything else is tinted with the color of the sea, so it seems to sink. The same happens with the search results, or else with the selected node.

### 4. Interacting with the map

**What is under the mouse (`MapPainter::nodeAt`).** The program measures the distance between the mouse and every disc. A disc can be caught from a few pixels away, so small bookmarks are not too hard to point at. When discs overlap, the biggest one wins.

**Click, double click or drag (`MouseTracker`).** A press followed by a release is a click, unless the mouse moved by more than a few pixels in between: then it was a drag of the map. Two clicks on the same node within 0.4 s make a double click.

**Smooth camera (`Camera`).** When the program moves the view (zoom on a folder, go to a search result), it does not jump: at each frame, the camera covers a fixed part of the remaining distance. It moves fast at first and slows down as it arrives. Zooming with the wheel keeps the point under the mouse in place, so you zoom where you are looking.

**Cutting long texts (`TextPainter::wrap`).** Letters are added to a line until it is full, then the line is cut after its last space so words stay whole. A word longer than a whole line (an URL) is cut anywhere. If the text still does not fit in the allowed number of lines, it ends with "…".

### 5. Growing land under the graph (`Terrain`, `TerrainPainter`)

With `--terrain` or the `T` key, the graph gets real land under it, once the layout has stopped moving (a fraction of a second). The method is the one of the 3D archipelago of [Compages](https://github.com/Lecrapouille/Compages) (`10e_Archipelago`), itself after A. Patel's *Polygonal Map Generation*, here in 2D and without moving the nodes:

1. **Cells (`VoronoiGrid`).** The map is cut into a few tens of thousands of cells, like the plots of a land register. A cell is the area closer to its center (its "site") than to any other site: a Voronoi diagram. The sites are placed on a grid and moved a little at random, so the cells look natural, and finding the cell of a point only takes nine distances.
2. **Land or sea.** A round patch of land is laid on each node and a strip along each edge of the tree (island > community > bookmark). The layout keeps the islands far apart, so each island also grows into the free sea around it, up to a bit less than halfway to its closest neighbour: a strait always remains. Positions are bent by smooth random noise (`Noise`), so the coasts wave instead of being perfect circles.
3. **Sea.** The sea is the water reached from the edges of the map, like paint poured from the borders. Water closed in by land would be a lake: it is filled.
4. **Regions.** Each land cell joins the community of the closest node of its island, so the communities become regions side by side, separated by a light line.
5. **Relief (`Erosion`).** The big nodes become the summits: the island node is the highest one, each community node a smaller peak, higher when the community holds more bookmarks. The ground slowly rises (most under the summits, a little everywhere inland) while the water digs it, faster where it carries more water and where it is steeper. Repeated over a few million simulated years, this draws ridges and valleys around the summits. A hollow with no way out is filled up to its lowest pass, so that all the water reaches the sea.
6. **Rivers.** As the thesis asks, contents lie along rivers: a river springs from each summit. Like a drop of water, it always goes to the neighbouring cell down the steepest slope (it follows the gradient of the ground) until it reaches the sea. The island rivers are traced first; a community river that meets one of them flows into it and becomes its tributary. The zigzag through the cells is rounded into a smooth curve, and a river widens downstream, where it carries the water of a larger area.
7. **Painting (`TerrainPainter`).** The sea gets lighter near the coast; the land goes from sand to grass, forest, rock and summits with the height, tinted by the color of its island. The relief is lit from the top left (hillshading): slopes facing the light are brighter, so hills look raised. The borders of the cells are drawn, firmer around the cell of a bookmark and bold between two regions.

The same bookmarks always give the same land.

## 🗂️ Source code

| File | Role |
|------|------|
| `scripts/firefox_export.py`, `organize.py`, `firefox_import.py` | steps 1, 2 and 4 (Python, standard library only) |
| `scripts/bookmarklib.py` | what the scripts share: reading Firefox, the `-d` option... |
| `main.cpp` | reads the command line, opens the window or writes the layout file |
| `Application` | window and main loop (events, update, draw) |
| `Bookmarks`, `Json` | load a Firefox backup or an `organization.json` |
| `IslandedBrowser` | bookmarks + layout, and the actions on them (search, open in the browser...) |
| `ForceDirectedGraph` | the springs and magnets layout |
| `LayoutExport` | `--layout` output for Compages |
| `IslandedBrowserGUI` | the conductor: turns mouse and keyboard events into actions |
| `Camera` | visible part of the map, zoom, smooth moves |
| `Selection` | hovered and selected nodes, what stays bright |
| `MapPainter` | sea routes, links, discs, names; finds the node under the mouse |
| `SeaRoutes`, `Palette`, `Meshes`, `NodeStyle` | sea routes between islands, island colors, triangles of the shapes, disc sizes and node kinds |
| `TextPainter` | writes texts, paints panel backgrounds, cuts long texts |
| `Tooltip`, `DetailsPanel`, `SearchBox`, `StatusBar`, `Toast`, `HelpScreen` | the interface widgets |
| `MouseTracker`, `MouseCursor` | click / double click / drag detection, pointer shape |
| `Terrain`, `TerrainPainter` | land, regions, relief and rivers under the graph; their picture |
| `VoronoiGrid`, `Erosion`, `Noise`, `Maths` | cells of the map, uplift and river erosion, smooth randomness, small helpers |

Next steps: 3D rendering (see the `10e_Archipelago` demo of [Compages](https://github.com/Lecrapouille/Compages)).

## 📚 References

- Philipp Kindermann Youtube videos [Force-Directed Drawings (1/3)](https://youtu.be/WWm-g2nLHds)
- https://github.com/qdHe/Parallelized-Force-directed-Graph-Drawing
