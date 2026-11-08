#!/usr/bin/env python3
###############################################################################
### Step 4: turn organization.json back into files Firefox can load.
###
### Two files are written, choose one:
###
### private/organized.html  (safe, additive)
###   Firefox > Library (Ctrl+Shift+O) > Import and Backup >
###   Import Bookmarks from HTML... Everything lands in ONE new folder
###   "IslandedBrowser <date>" next to your existing bookmarks. Delete the old
###   folders by hand once you are happy.
###
### private/organized.json  (replaces everything)
###   Firefox > Library > Import and Backup > Restore > Choose File...
###   Firefox ERASES all current bookmarks (menu, toolbar, others) and loads
###   this file. Make a backup first (Import and Backup > Backup...). The
###   islands go in the bookmarks menu and the toolbar is left empty (every
###   bookmark already sits in an island). Bookmark guids are kept, so the
###   constraints of organization.json still match after the next export.
###
### Only the fields Firefox knows are written; tags are comma separated, as in
### Firefox backups.
###############################################################################

import argparse, json, sys, time
from datetime import datetime
from html import escape
from pathlib import Path

import bookmarklib

FORMAT = "islanded-browser-organization"
GUID_ALPHABET = set("ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789-_")


###############################################################################
### Entry point.
###############################################################################
def main():
    parser = argparse.ArgumentParser(
        description="Step 4: write organization.json as Firefox bookmark files.")
    parser.add_argument("-i", "--input", type=Path, default=bookmarklib.PRIVATE_DIR / "organization.json",
                        help="organization (default: private/organization.json)")
    parser.add_argument("--json", type=Path, default=bookmarklib.PRIVATE_DIR / "organized.json",
                        help="Firefox backup to restore (replaces all bookmarks)")
    parser.add_argument("--html", type=Path, default=bookmarklib.PRIVATE_DIR / "organized.html",
                        help="Netscape HTML file to import (adds one folder)")
    parser.add_argument("--keywords", type=int, default=0,
                        help="also tag each bookmark with its N first keywords")
    args = parser.parse_args()

    organization = load(args.input)
    folders = build_folders(organization, args.keywords)

    count = sum(len(island["children_bookmarks"]) for island in walk(folders))
    args.json.write_text(json.dumps(firefox_backup(folders), ensure_ascii=False,
                                    indent=1) + "\n", encoding="utf-8")
    args.html.write_text(netscape_html(folders), encoding="utf-8")
    print(f"{count} bookmarks in {len(folders)} islands")
    print(f"Additive import : {args.html}")
    print("    Firefox > Library > Import and Backup > Import Bookmarks from HTML...")
    print(f"Full replacement: {args.json}")
    print("    Firefox > Library > Import and Backup > Restore > Choose File...")
    print("    (erases all current bookmarks: make a backup first)")


###############################################################################
### Read and check organization.json.
###############################################################################
def load(path):
    if not path.is_file():
        sys.exit(f"{path} is missing. Run bookmarks/organize.py first.")
    try:
        document = json.loads(path.read_text(encoding="utf-8"))
    except json.JSONDecodeError as error:
        sys.exit(f"{path} is not valid JSON: {error}")
    if document.get("format") != FORMAT:
        sys.exit(f"{path} was not written by organize.py.")
    return document


###############################################################################
### Folder tree: island > community > bookmarks. Each folder is
### {"title", "folders": [...], "children_bookmarks": [...]}.
###############################################################################
def build_folders(organization, keyword_count):
    groups = {group["id"]: group for group in organization["communities"]}
    bookmarks = {item["id"]: item for item in organization["bookmarks"]}
    placed = set()

    def folder(group):
        node = {"title": group["name"] or group["auto_name"] or group["id"],
                "folders": [], "children_bookmarks": []}
        for child in group.get("children") or []:
            if child in groups:
                node["folders"].append(folder(groups[child]))
        for identifier in group.get("bookmarks") or []:
            item = bookmarks.get(identifier)
            if item is None or identifier in placed:
                continue
            placed.add(identifier)
            tags = list(item.get("tags") or [])
            tags += (item.get("keywords") or [])[:keyword_count]
            node["children_bookmarks"].append({
                "title": item.get("title") or item["url"],
                "url": item["url"],
                "guid": item.get("guid") or "",
                "tags": bookmarklib.join_tags(tags),
                "dateAdded": int(item.get("date_added") or 0),
                "lastModified": int(item.get("last_modified") or 0),
            })
        return node

    roots = [group for group in organization["communities"] if not group.get("parent")]
    folders = [folder(group) for group in roots]
    missing = [item for identifier, item in bookmarks.items() if identifier not in placed]
    if missing:
        # A bookmark that lost its community (hand edit) is never dropped.
        folders.append({"title": "Non classés", "folders": [], "children_bookmarks": [
            {"title": item.get("title") or item["url"], "url": item["url"],
             "guid": item.get("guid") or "", "tags": bookmarklib.join_tags(item.get("tags") or []),
             "dateAdded": int(item.get("date_added") or 0),
             "lastModified": int(item.get("last_modified") or 0)} for item in missing]})
    return folders


###############################################################################
### Every folder of the tree, depth first.
###############################################################################
def walk(folders):
    for node in folders:
        yield node
        yield from walk(node["folders"])


###############################################################################
### Firefox backup document (the format of "Backup..." in the library).
###############################################################################
def firefox_backup(folders):
    now = int(time.time() * 1_000_000)
    used_guids = set()

    def guid(value):
        # Keep the original guid: Firefox then keeps the identity of the
        # bookmark. Drop invalid or repeated ones, Firefox makes a new one.
        if len(value) == 12 and set(value) <= GUID_ALPHABET and value not in used_guids:
            used_guids.add(value)
            return {"guid": value}
        return {}

    def bookmark(item, index):
        node = {"title": item["title"], "index": index, "typeCode": 1,
                "type": "text/x-moz-place", "uri": item["url"]}
        node.update(guid(item["guid"]))
        if item["dateAdded"]:
            node["dateAdded"] = item["dateAdded"]
            node["lastModified"] = max(item["lastModified"], item["dateAdded"])
        if item["tags"]:
            node["tags"] = item["tags"]
        return node

    def folder(node, index):
        children = [folder(child, i) for i, child in enumerate(node["folders"])]
        children += [bookmark(item, len(children) + i)
                     for i, item in enumerate(node["children_bookmarks"])]
        return {"title": node["title"], "index": index, "dateAdded": now,
                "lastModified": now, "typeCode": 2,
                "type": "text/x-moz-place-container", "children": children}

    def root(guid_value, root_name, title, index, children):
        return {"guid": guid_value, "title": title, "index": index, "dateAdded": now,
                "lastModified": now, "typeCode": 2, "type": "text/x-moz-place-container",
                "root": root_name, "children": children}

    menu = [folder(node, index) for index, node in enumerate(folders)]
    return root("root________", "placesRoot", "", 0, [
        root("menu________", "bookmarksMenuFolder", "menu", 0, menu),
        root("toolbar_____", "toolbarFolder", "toolbar", 1, []),
        root("unfiled_____", "unfiledBookmarksFolder", "unfiled", 3, []),
        root("mobile______", "mobileFolder", "mobile", 4, []),
    ])


###############################################################################
### Netscape bookmark file, the format of "Import Bookmarks from HTML".
###############################################################################
def netscape_html(folders):
    now = int(time.time())
    lines = [
        "<!DOCTYPE NETSCAPE-Bookmark-file-1>",
        "<!-- Written by IslandedBrowser bookmarks/firefox_import.py -->",
        '<META HTTP-EQUIV="Content-Type" CONTENT="text/html; charset=UTF-8">',
        "<TITLE>Bookmarks</TITLE>",
        "<H1>Bookmarks</H1>",
        "<DL><p>",
        f'    <DT><H3 ADD_DATE="{now}" LAST_MODIFIED="{now}">'
        f'IslandedBrowser {datetime.now():%Y-%m-%d %H:%M}</H3>',
        "    <DL><p>",
    ]

    def emit(node, depth):
        pad = "    " * depth
        lines.append(f'{pad}<DT><H3 ADD_DATE="{now}" LAST_MODIFIED="{now}">'
                     f'{escape(node["title"], quote=False)}</H3>')
        lines.append(f"{pad}<DL><p>")
        for child in node["folders"]:
            emit(child, depth + 1)
        for item in node["children_bookmarks"]:
            added = item["dateAdded"] // 1_000_000 if item["dateAdded"] else now
            attributes = f'HREF="{escape(item["url"])}" ADD_DATE="{added}"'
            if item["tags"]:
                attributes += f' TAGS="{escape(item["tags"])}"'
            lines.append(f'{pad}    <DT><A {attributes}>{escape(item["title"], quote=False)}</A>')
        lines.append(f"{pad}</DL><p>")

    for node in folders:
        emit(node, 2)
    lines += ["    </DL><p>", "</DL><p>", ""]
    return "\n".join(lines)


if __name__ == "__main__":
    main()
