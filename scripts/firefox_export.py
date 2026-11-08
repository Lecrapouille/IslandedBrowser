#!/usr/bin/env python3
###############################################################################
### Step 1: copy the Firefox bookmark tree into DIR/bookmarks.json (-d DIR,
### private/ by default).
###
### The file has the same shape as the backup Firefox writes from the library
### window, plus the visit counts, descriptions and tags already stored in
### places.sqlite. Nothing is sent over the network. The original database is
### left untouched.
###############################################################################

import argparse, json, sys
from pathlib import Path

import bookmarklib


###############################################################################
### Entry point.
### -d, --dir: folder of the files (default: private/).
### -o, --output: JSON file to write (default: DIR/bookmarks.json).
### --profile: Firefox profile directory when autodetection fails.
###############################################################################
def main():
    parser = argparse.ArgumentParser(
        description="Step 1: copy the Firefox bookmark tree into DIR/bookmarks.json.")
    bookmarklib.add_dir_argument(parser)
    parser.add_argument(
        "-o", "--output", type=Path, default=None,
        help="JSON written (default: DIR/bookmarks.json)")
    parser.add_argument(
        "--profile", type=Path, default=None,
        help="Firefox profile directory, when the automatic detection fails")
    args = parser.parse_args()
    args.output = bookmarklib.file_in(args.output, args.dir, "bookmarks.json")

    profile = bookmarklib.find_firefox_profile(args.profile)
    connection, temporary = bookmarklib.open_places(profile)
    try:
        by_id, by_parent = bookmarklib.load_rows(connection)
        tags = bookmarklib.tags_by_url(by_id, by_parent)
        tree = bookmarklib.build_firefox_tree(by_id, by_parent, tags)
    finally:
        bookmarklib.close_places(connection, temporary)

    if tree is None:
        sys.exit("This profile has no bookmark root.")

    urls = 0
    folders = 0

    def count(node):
        nonlocal urls, folders
        children = node.get("children")
        if children is not None and node.get("typeCode") != 1:
            folders += 1
            for child in children:
                count(child)
        elif node.get("uri"):
            urls += 1

    count(tree)
    args.output.parent.mkdir(parents=True, exist_ok=True)
    args.output.write_text(
        json.dumps(tree, ensure_ascii=False, indent=2) + "\n",
        encoding="utf-8")
    print(f"Profile: {profile}")
    print(f"Written: {args.output}")
    print(f"{urls} bookmarks, {folders} folders")


if __name__ == '__main__':
    main()
