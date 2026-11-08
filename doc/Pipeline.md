# 🔀 Bookmark pipeline

This document details the four steps that turn unorganized Firefox bookmarks into an archipelago: export, sorting, drawing, and import back into Firefox. The [README](../README.md) gives the short version.

```
     1 export              2 sort                   3 look
Firefox ───> bookmarks.json ───> organization.json ───> IslandedBrowser
   ▲                                    │
   └──── organized.html / .json <───────┘
                    4 import
```

## 🔒 Files and privacy

All the files of a run go in one folder, always with the same names. By default it is `private/`, at the root of the project: the files hold personal data (URLs, titles, visit counts), so git ignores this folder and nothing in it can be committed by mistake. Delete it to start from scratch.

Every program takes `-d DIR` to use another folder. `examples/` holds the same files made from 118 public bookmarks, so any command of this document works on the example with `-d examples`. `-i FILE` and `-o FILE` replace one file of the folder.

| File | Written by | Read by | Content |
|------|-----------|---------|---------|
| `bookmarks.json` | `firefox_export.py` | `organize.py`, `IslandedBrowser` | Firefox bookmark tree, as is |
| `organization.json` | `organize.py` | `firefox_import.py`, `IslandedBrowser`, `organize.py` (corrections) | islands, communities, similarity links, constraints |
| `layout.json` | `IslandedBrowser --layout` | Compages (`10e_Archipelago`) | position of every node |
| `organized.html` | `firefox_import.py` | Firefox (import) | the organization, added in one folder |
| `organized.json` | `firefox_import.py` | Firefox (restore) | the organization, replacing all bookmarks |
| `.cache/pages.json` | `organize.py --fetch` | `organize.py --fetch` | page descriptions downloaded once |

The scripts need Python 3 only (standard library). Each one accepts `--help`.

## 📤 1. Export: `scripts/firefox_export.py`

```
./scripts/firefox_export.py
./scripts/firefox_export.py --profile ~/.mozilla/firefox/xxxxxxxx.default-release
```

- The default Firefox profile is found through `profiles.ini` (classic, Snap and Flatpak installations). Give `--profile` when it is not the one you use.
- `places.sqlite` is copied to a temporary folder before being read, so Firefox can stay open and the original database is never modified.
- The output has the shape of a Firefox backup, plus the visit count, the page description and the tags known by Firefox.

A backup saved by hand also works: Firefox library (Ctrl+Shift+O) > *Import and Backup* > *Backup...*, saved as `private/bookmarks.json`.

## 🗂️ 2. Sorting: `scripts/organize.py`

```
./scripts/organize.py
./scripts/organize.py --fetch          # also download missing page descriptions
./scripts/organize.py -d examples      # the example
```

### ⚙️ How the groups are built

1. Each bookmark becomes a weighted bag of words: title (×3), tags (×3), page description, words of the URL path, site (except large platforms such as GitHub or YouTube, whose domain says nothing about the subject), repository for GitHub/GitLab pages, and the name of the original folder. Generic root folders ("Barre personnelle", "Bookmarks Menu") are ignored.
2. TF-IDF weighting: a word shared by everything weighs nothing, a word shared by a few bookmarks weighs a lot. Two bookmarks are compared with the cosine similarity.
3. Each bookmark is linked to its 10 nearest neighbours (when the similarity is at least 0.12). This keeps the graph connected without a global threshold.
4. Louvain community detection runs twice: a coarse pass gives the **islands**, a fine pass inside each island gives the **communities**. Communities larger than 40 bookmarks are split again, communities smaller than 3 are merged into their closest neighbour.
5. Bookmarks without any neighbour go to the *Orphelins / À explorer* island. A lonely bookmark is legitimate: it is not forced into a group.
6. The same URL saved several times is kept once (tags are merged), unless `--keep-duplicates` is given.

The result is deterministic: the same input always gives the same groups.

### 🔍 Reviewing `private/organization.json`

Each entry of `communities` is an island (`parent` is `null`) or a community (`parent` is the island id):

| Field | Meaning |
|-------|---------|
| `name` | name shown everywhere; edit it to rename the group |
| `auto_name` | name computed by the script (do not edit) |
| `keywords` | most distinctive words of the group |
| `confidence` | mean similarity inside the group, from 0 to 1 |
| `neighbors` | closest groups |
| `representatives` | bookmark ids that best represent the group |
| `outliers` | bookmark ids that have no link inside the group (probably misplaced) |
| `bookmarks` | bookmark ids of a community |

Each entry of `bookmarks` has its `guid` (Firefox identifier), `community` (primary community), `also` (other strongly related communities), `keywords` and the original `folder`. `links` holds the similarity graph as `[id, id, similarity]` triplets.

### ✏️ Correcting the result

Corrections are kept from one run to the next. Edit the file, then run `organize.py` again:

- **Rename** a group: change its `name`. The script notices that `name` differs from `auto_name` and records the renaming in `constraints.names`.
- **Other corrections**: edit `constraints`. Bookmarks are referenced by their `guid`, which does not change across exports and restores (unlike `id`, which is only a position in the file).

```json
"constraints": {
  "isolated":    ["guid", "..."],
  "must_link":   [["guid", "guid", "..."]],
  "cannot_link": [["guid", "guid"]],
  "moves":       [{"bookmark": "guid", "community": "Robotics"}],
  "names":       [{"name": "Robotics", "bookmarks": ["guid", "..."]}]
}
```

| Constraint | Effect |
|------------|--------|
| `isolated` | the bookmark is never linked and stays in the orphans |
| `must_link` | the bookmarks are always linked together |
| `cannot_link` | the bookmarks are never linked directly |
| `moves` | the bookmark goes in the named community (created in a *Classement manuel* island when it does not exist) |
| `names` | the group sharing most of these bookmarks takes the name |

### 🎛️ Tuning

| Option | Default | Effect |
|--------|---------|--------|
| `--island-resolution` | 0.6 | lower: fewer, larger islands |
| `--community-resolution` | 1.4 | higher: smaller communities |
| `--neighbors` | 10 | links per bookmark; more gives larger groups |
| `--min-similarity` | 0.12 | higher: more orphans, cleaner groups |
| `--fetch`, `--fetch-limit N` | off | download the descriptions of pages Firefox does not know |

## 🏝️ 3. Drawing: `IslandedBrowser`

```
make
alias IslandedBrowser=./build/linux-gcc-normal/bin/IslandedBrowser
IslandedBrowser                             # private/organization.json: the islands
IslandedBrowser -i private/bookmarks.json   # your original Firefox folders
IslandedBrowser -d examples --terrain       # the example, with its land
```

`IslandedBrowser` draws `DIR/organization.json`, or `DIR/bookmarks.json` when there is no organization yet. Without `-d`, `DIR` is `private/`, or `examples/` when `private/` is empty. A file given alone (`IslandedBrowser some/file.json`) is the same as `-i some/file.json`. Firefox roots without any bookmark (often *mobile* or *other bookmarks*) are not drawn. Drawing `bookmarks.json` shows the original folders, which is useful to compare with the organization.

| Input | Action |
|-------|--------|
| hover | tooltip (title, URL, island and community); the community is highlighted, the rest of the map fades |
| left click | select: the right panel shows the details (where it is, URL, content of a folder, similar bookmarks); its entries can be clicked |
| click on the sea | unselect |
| double click | open a bookmark in Firefox; zoom on a folder |
| middle click, `Ctrl` + click | open in Firefox; on a folder, its first 15 bookmarks |
| left or right drag | pan |
| wheel | zoom around the cursor |
| `H`, `F1`, `?` | show or hide the help |
| `/`, `Ctrl` + `F` | search the titles and URLs: `Enter`/`↓` next result, `↑` previous, `Ctrl` + `Enter` open, `Esc` close |
| `Enter`, `O` | open the selection in Firefox |
| `Z` | zoom on the selection |
| `F` | fit the view to the graph |
| arrows, `+`, `-` | pan, zoom |
| `Space` | pause or resume the layout |
| `R` | restart the layout |
| `L` | show or hide links and sea routes |
| `N` | show or hide the names (community and bookmark names appear when zoomed in) |
| `T` | show or hide the terrain: coasts, relief with the big nodes as summits, Voronoi cells, regions, rivers flowing down from the summits (made once the layout is stable) |
| `S` | save `islanded-browser.png` |
| `Esc` | close the help, unselect, then quit |
| `Q` | quit |

The status bar, at the bottom, recalls the file, the number of bookmarks and islands, the state of the layout and the shortcuts available at the moment.

`--terrain` shows the terrain from the start (same as the `T` key).

`--screenshot FILE` saves the window as a PNG once the layout has converged, then quits; the mouse and the keyboard are ignored meanwhile. With `--terrain`, the picture includes the land: `IslandedBrowser -d examples --terrain --screenshot doc/archipelago.png`.

`--layout [FILE]` computes the layout without a window, writes the position of every node in `DIR/layout.json` (or `FILE`), then quits. The [10e_Archipelago](https://github.com/Lecrapouille/Compages) demo of Compages builds a 3D archipelago around it: one island per island, one region per community, the bookmarks along the rivers. `examples/layout.json` was made with:

```
./scripts/organize.py -d examples
IslandedBrowser -d examples --layout
```

The file is `{"format": "islanded-browser-layout", "version": 1, "source", "nodes", "links"}`; a node is `{"id", "parent"` (-1 for an island)`, "kind"` (`folder` or `bookmark`)`, "title", "url", "x", "y"}`; a link is `[id, id, similarity]`.

Each island has its own color: large discs are islands, medium discs communities (or folders), small light dots bookmarks. Faint lines inside an island are similarity links. Blue bands across the sea are sea routes: the strongest relations between two islands, wider when the islands share more similar bookmarks. With the terrain (`T`), the rivers are on the land: each one springs from a big node and runs down the slope to the sea.

## 📥 4. Import into Firefox: `scripts/firefox_import.py`

```
./scripts/firefox_import.py
./scripts/firefox_import.py --keywords 3   # also tag each bookmark with 3 keywords
```

Two files are written; use one of them.

**`private/organized.html`, safe.** Firefox library (Ctrl+Shift+O) > *Import and Backup* > *Import Bookmarks from HTML...*. Everything is added in one new folder `IslandedBrowser <date>`, next to your current bookmarks. Nothing is deleted: compare, then delete the old folders by hand.

**`private/organized.json`, replaces everything.** First make a backup: *Import and Backup* > *Backup...*. Then *Import and Backup* > *Restore* > *Choose File...*. Firefox **erases all bookmarks** (menu, toolbar, other bookmarks) and loads the islands into the bookmarks menu; the toolbar is left empty. Bookmark guids are kept, so the constraints of `organization.json` still apply after the next export.

Original tags are kept and written in the Firefox format (one comma separated string).

## 🛠️ Troubleshooting

| Message | Cause | Fix |
|---------|-------|-----|
| `Firefox profile not found` | profile outside the usual folders | `firefox_export.py --profile PATH` |
| `private/bookmarks.json is missing` | step 1 not done | run `firefox_export.py` |
| `organization written by an older script` | file from a previous version | run `organize.py` again |
| `neither organization.json nor bookmarks.json in DIR/` | wrong `-d`, or nothing exported yet | run step 1, or use `-d examples` |
| `data/font.ttf not found` | program started outside the project folder | run it from the project root |
| too many orphans | similarity threshold too high | lower `--min-similarity`, or use `--fetch` |
| one huge island | resolution too low | raise `--island-resolution` |
