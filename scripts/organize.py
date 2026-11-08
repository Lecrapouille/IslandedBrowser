#!/usr/bin/env python3
###############################################################################
### Step 2: build a provisional thematic organization of the bookmarks.
###
### Input: a Firefox backup tree (DIR/bookmarks.json, written by
### firefox_export.py or by "Backup..." in the Firefox library window).
### Output: DIR/organization.json, read by the C++ renderer and by
### firefox_import.py.
###
### Pipeline:
###   1. Each bookmark becomes a weighted bag of words: title, tags, page
###      description, URL path, site, repository and the original folder.
###   2. TF-IDF + cosine similarity. Each bookmark is linked to its k nearest
###      neighbours: the graph stays connected without a global threshold.
###   3. Louvain community detection, run at two resolutions:
###      islands (coarse) then communities inside each island (fine).
###   4. Names, keywords, confidence, neighbours and outliers are computed so
###      that a human can review the result.
###
### Human validation: edit the "constraints" section of organization.json
### (or rename a community in place) and run this script again. Bookmarks are
### referenced by their Firefox guid, which survives exports and restores.
###
### Only the Python standard library is used. Nothing is sent over the
### network unless --fetch is given.
###############################################################################

import argparse, json, math, random, re, ssl, sys
import urllib.request
from collections import Counter, defaultdict
from concurrent.futures import ThreadPoolExecutor, as_completed
from datetime import datetime, timezone
from html import unescape
from pathlib import Path
from urllib.parse import urlparse, unquote

import bookmarklib

FORMAT = "islanded-browser-organization"
VERSION = 2

# Similarity graph.
NEIGHBORS = 10          # k nearest neighbours kept per bookmark
MIN_SIMILARITY = 0.12   # below this cosine, two bookmarks are not linked
MAX_DF_RATIO = 0.12     # a token present in more bookmarks says nothing

# Community sizes.
ISLAND_RESOLUTION = 0.6
COMMUNITY_RESOLUTION = 1.4
MAX_COMMUNITY = 40
MIN_COMMUNITY = 3

# Weights of the fields of a bookmark in its bag of words.
FIELD_WEIGHTS = {
    "title": 3.0,
    "tags": 3.0,
    "description": 1.5,
    "path": 1.0,
    "folder": 1.5,
    "site": 2.0,
    "repo": 3.0,
}

ORPHANS = "orphans"
ORPHANS_NAME = "Orphelins / À explorer"

EMPTY_CONSTRAINTS = {
    "_help": [
        "Bookmarks are referenced by their Firefox guid (field 'guid').",
        "isolated: [guid, ...] never linked to anything, kept in the orphans.",
        "must_link: [[guid, guid, ...], ...] always in the same community.",
        "cannot_link: [[guid, guid], ...] never linked directly.",
        "moves: [{'bookmark': guid, 'community': 'Name'}] forced placement.",
        "names: [{'name': 'Name', 'bookmarks': [guid, ...]}] renaming. A",
        "community renamed directly in this file is added here on the next run.",
    ],
    "isolated": [],
    "must_link": [],
    "cannot_link": [],
    "moves": [],
    "names": [],
}

PATH_NOISE = re.compile(r"^[0-9a-f]{6,}$|^\d+$")
CAMEL = re.compile(r"(?<=[a-z])(?=[A-Z])")
META_TAG = re.compile(r"<meta\b[^>]*>", re.IGNORECASE)
ATTRS = re.compile(r"""([A-Za-z_:][-A-Za-z0-9_:.]*)\s*=\s*["'](.*?)["']""", re.DOTALL)
REPO_HOSTS = {"github.com", "gitlab.com", "bitbucket.org", "codeberg.org"}

# Root folders: their title is not a subject.
ROOT_FOLDERS = {bookmarklib.fold(title) for title in list(bookmarklib.GUID_TITLE.values()) + [
    "Bookmarks Menu", "Bookmarks Toolbar", "Other Bookmarks", "Mobile Bookmarks",
    "Menu des marque-pages", "Barre personnelle", "Autres marque-pages",
    "Marque-pages mobiles", "menu", "toolbar", "unfiled", "mobile"]}

# Words of URL paths that say nothing about the subject.
PATH_STOPWORDS = {
    "master", "main", "blob", "tree", "raw", "wiki", "issue", "issues", "pull",
    "pulls", "commit", "commits", "src", "lib", "include", "readme", "watch",
    "tag", "tags", "category", "categories", "article", "articles", "post",
    "posts", "question", "questions", "users", "user", "en", "fr", "us", "item",
    "view", "show", "detail", "details", "product", "products", "p", "dp", "s",
    "file", "files", "content", "contents", "news", "forum", "topic", "viewtopic",
    "channel", "playlist", "video", "videos", "status", "embed", "share",
}


###############################################################################
### Entry point.
###############################################################################
def main():
    parser = argparse.ArgumentParser(
        description="Step 2: sort the bookmarks of DIR/bookmarks.json into islands "
                    "and communities, written in DIR/organization.json.")
    bookmarklib.add_dir_argument(parser)
    parser.add_argument("-i", "--input", type=Path, default=None,
                        help="Firefox bookmarks (default: DIR/bookmarks.json)")
    parser.add_argument("-o", "--output", type=Path, default=None,
                        help="organization written (default: DIR/organization.json)")
    parser.add_argument("--neighbors", type=int, default=NEIGHBORS,
                        help="nearest neighbours linked to each bookmark")
    parser.add_argument("--min-similarity", type=float, default=MIN_SIMILARITY,
                        help="minimum cosine similarity of a link")
    parser.add_argument("--island-resolution", type=float, default=ISLAND_RESOLUTION,
                        help="Louvain resolution of islands (lower = fewer, larger islands)")
    parser.add_argument("--community-resolution", type=float, default=COMMUNITY_RESOLUTION,
                        help="Louvain resolution of communities inside an island")
    parser.add_argument("--keep-duplicates", action="store_true",
                        help="keep several bookmarks pointing to the same URL")
    parser.add_argument("--fetch", action="store_true",
                        help="download pages without description (cached in DIR/.cache)")
    parser.add_argument("--fetch-limit", type=int, default=0,
                        help="maximum number of pages to download (0 = all missing)")
    parser.add_argument("--seed", type=int, default=0, help="random seed of Louvain")
    args = parser.parse_args()
    args.input = bookmarklib.file_in(args.input, args.dir, "bookmarks.json")
    args.output = bookmarklib.file_in(args.output, args.dir, "organization.json")

    if not args.input.is_file():
        sys.exit(f"{args.input} is missing. Run scripts/firefox_export.py first.")

    constraints = load_constraints(args.output)
    tree = json.loads(args.input.read_text(encoding="utf-8"))
    items, duplicates = collect_bookmarks(tree, args.keep_duplicates)
    if not items:
        sys.exit(f"No bookmark in {args.input}.")
    if args.fetch:
        fetch_descriptions(items, args.dir / ".cache" / "pages.json", args.fetch_limit)

    surfaces = defaultdict(Counter)
    vectors = vectorize(items, surfaces)
    graph = knn_graph(items, vectors, args.neighbors, args.min_similarity, constraints)
    islands, communities = detect(items, graph, args)
    apply_moves(items, communities, islands, constraints)
    describe_all(items, vectors, graph, islands, communities, surfaces)
    apply_names(communities, islands, items, constraints)

    document = {
        "format": FORMAT,
        "version": VERSION,
        "generated": datetime.now(timezone.utc).isoformat(timespec="seconds"),
        "source": str(args.input),
        "parameters": {
            "neighbors": args.neighbors,
            "min_similarity": args.min_similarity,
            "island_resolution": args.island_resolution,
            "community_resolution": args.community_resolution,
        },
        "duplicates_removed": duplicates,
        "communities": [public_group(group) for group in islands + communities],
        "bookmarks": [public_bookmark(item) for item in items],
        "links": public_links(graph),
        "constraints": constraints,
    }
    args.output.parent.mkdir(parents=True, exist_ok=True)
    args.output.write_text(json.dumps(document, ensure_ascii=False, indent=1) + "\n",
                           encoding="utf-8")
    report(args.output, items, islands, communities, duplicates)


###############################################################################
### Constraints of a previous run. A community whose name was edited by hand
### (name != auto_name) becomes a naming constraint.
###############################################################################
def load_constraints(path):
    constraints = json.loads(json.dumps(EMPTY_CONSTRAINTS))
    if not path.is_file():
        return constraints
    try:
        previous = json.loads(path.read_text(encoding="utf-8"))
    except json.JSONDecodeError as error:
        sys.exit(f"{path} is not valid JSON ({error}). Fix it or delete it.")
    found = previous.get("constraints") or {}
    for key in constraints:
        if key != "_help" and isinstance(found.get(key), list):
            constraints[key] = found[key]

    if previous.get("format") != FORMAT:
        # Older file: its names were never edited against an auto_name.
        return constraints
    guid_of = {item.get("id"): item.get("guid") for item in previous.get("bookmarks") or []}
    named = {entry.get("name") for entry in constraints["names"]}
    for group in previous.get("communities") or []:
        name = (group.get("name") or "").strip()
        if not name or name == group.get("auto_name") or name in named:
            continue
        members = [guid_of.get(identifier) for identifier in group.get("bookmarks") or []]
        members = [guid for guid in members if guid]
        if members:
            constraints["names"].append({"name": name, "bookmarks": members})
            named.add(name)
    return constraints


###############################################################################
### Flatten a Firefox backup tree into bookmark records.
###############################################################################
def collect_bookmarks(tree, keep_duplicates):
    found = []
    seen = {}
    duplicates = 0

    def sink(node, folder):
        nonlocal duplicates
        url = node["uri"]
        if not url.startswith(("http://", "https://", "ftp://", "file://")):
            # place:, javascript: and about: entries are not documents.
            return
        title = node.get("title") or ""
        tags = bookmarklib.tags_of(node)
        if not keep_duplicates and url in seen:
            duplicates += 1
            first = seen[url]
            first["tags"] = list(dict.fromkeys(first["tags"] + tags))
            return
        item = {
            "id": len(found),
            "guid": node.get("guid") or "",
            "url": url,
            "original_title": title,
            "title": bookmarklib.clean_title(title) or url,
            "folder": "/".join(part for part in folder if part),
            "description": (node.get("description") or "").strip(),
            "tags": tags,
            "date_added": node.get("dateAdded") or 0,
            "last_modified": node.get("lastModified") or 0,
            "visit_count": int(node.get("visitCount") or 0),
        }
        seen[url] = item
        found.append(item)

    bookmarklib.walk_firefox_tree(tree, [], sink)
    return found, duplicates


###############################################################################
### Download the pages whose description Firefox did not keep.
###############################################################################
def fetch_descriptions(items, cache_path, limit):
    cache = {}
    if cache_path.is_file():
        try:
            cache = json.loads(cache_path.read_text(encoding="utf-8"))
        except json.JSONDecodeError:
            cache = {}
    missing = [item for item in items
               if not item["description"] and item["url"].startswith("http")
               and item["url"] not in cache]
    if limit > 0:
        missing = missing[:limit]
    if missing:
        print(f"Downloading {len(missing)} pages...")
        with ThreadPoolExecutor(max_workers=8) as pool:
            futures = {pool.submit(fetch_page, item["url"]): item["url"] for item in missing}
            for done, future in enumerate(as_completed(futures), start=1):
                cache[futures[future]] = future.result()
                if done % 50 == 0:
                    print(f"  {done}/{len(missing)}")
        cache_path.parent.mkdir(parents=True, exist_ok=True)
        cache_path.write_text(json.dumps(cache, ensure_ascii=False) + "\n", encoding="utf-8")
    for item in items:
        description = (cache.get(item["url"]) or {}).get("description")
        if not item["description"] and description:
            item["description"] = description


###############################################################################
### Download one HTML page and keep its meta description.
###############################################################################
def fetch_page(url):
    request = urllib.request.Request(url, headers={
        "User-Agent": "IslandedBrowser/0.2 (bookmark organizer)",
        "Accept": "text/html",
    })
    try:
        with urllib.request.urlopen(request, timeout=8,
                                    context=ssl.create_default_context()) as response:
            if "html" not in response.headers.get("Content-Type", "html"):
                return {"description": ""}
            charset = response.headers.get_content_charset() or "utf-8"
            html = response.read(250_000).decode(charset, errors="replace")
    except (OSError, ValueError):  # URLError and timeouts are OSError
        return {"description": ""}
    for tag in META_TAG.findall(html):
        attributes = {key.lower(): unescape(value) for key, value in ATTRS.findall(tag)}
        name = (attributes.get("name") or attributes.get("property") or "").lower()
        if name in ("description", "og:description") and attributes.get("content"):
            return {"description": " ".join(attributes["content"].split())[:500]}
    return {"description": ""}


###############################################################################
### Reduce a token to a crude common form (plural, case) so that "shaders"
### and "shader" meet.
###############################################################################
def stem(token):
    if len(token) > 4 and token.endswith("s") and not token.endswith("ss"):
        return token[:-1]
    return token


###############################################################################
### True when a token can appear in the name of a group (no number, no
### two letters word, no site name).
###############################################################################
def nameable(token):
    if token.isdigit() or any(char.isdigit() for char in token[:1]) and len(token) > 2 and token[1:].isdigit():
        return False
    if len(token) <= 2 and token not in bookmarklib.SHORT_TOKENS:
        return False
    return token not in PATH_STOPWORDS


###############################################################################
### Weighted bag of words of one bookmark: {token: weight}.
###############################################################################
def bag_of_words(item, surfaces):
    bag = Counter()

    # Tokens allowed in the names of groups: what a human wrote about the page.
    item["name_tokens"] = set()

    def add(text, field):
        bookmarklib.remember_surfaces(text, surfaces)
        for token in bookmarklib.tokenize(text):
            if field == "path" and token in PATH_STOPWORDS:
                continue
            token = stem(token)
            bag[token] += FIELD_WEIGHTS[field]
            if field in ("title", "tags", "description") and nameable(token):
                item["name_tokens"].add(token)

    add(item["title"], "title")
    add(" ".join(item["tags"]), "tags")
    add(item["description"], "description")

    parsed = urlparse(item["url"])
    host = (parsed.hostname or "").lower()
    domain = bookmarklib.registrable_domain(host)
    item["domain"] = domain
    item["site"] = bookmarklib.site_label(domain)
    segments = [unquote(part) for part in parsed.path.split("/") if part]
    words = []
    for segment in segments:
        segment = re.sub(r"\.(html?|php|aspx?|md|pdf)$", "", segment, flags=re.IGNORECASE)
        for word in re.split(r"[-_.+~]", CAMEL.sub(" ", segment)):
            if word and not PATH_NOISE.match(word.lower()):
                words.append(word)
    add(" ".join(words), "path")

    # The site says a lot about small sites and nothing about large platforms.
    if domain and domain not in bookmarklib.HUB_DOMAINS:
        bag["site:" + domain] += FIELD_WEIGHTS["site"]
    # On a forge, the repository is the subject.
    if domain in REPO_HOSTS and len(segments) >= 2:
        bag["repo:" + "/".join(segments[:2]).lower()] += FIELD_WEIGHTS["repo"]
        bag["owner:" + segments[0].lower()] += 1.0

    folders = [part for part in item["folder"].split("/")
               if part and bookmarklib.fold(part) not in ROOT_FOLDERS]
    if folders:
        add(folders[-1], "folder")
        bag["dir:" + "/".join(folders)] += FIELD_WEIGHTS["folder"]
    return bag


###############################################################################
### TF-IDF vectors, L2 normalized: list of {token: weight}.
###############################################################################
def vectorize(items, surfaces):
    bags = [bag_of_words(item, surfaces) for item in items]
    df = Counter()
    for bag in bags:
        df.update(bag.keys())
    count = len(items)
    max_df = max(3, int(count * MAX_DF_RATIO))
    idf = {token: math.log(count / freq) for token, freq in df.items()
           if 2 <= freq <= max_df}
    vectors = []
    for bag in bags:
        vector = {token: (1.0 + math.log(weight)) * idf[token]
                  for token, weight in bag.items() if token in idf and weight > 0}
        norm = math.sqrt(sum(value * value for value in vector.values()))
        if norm > 0:
            vector = {token: value / norm for token, value in vector.items()}
        vectors.append(vector)
    return vectors


###############################################################################
### Symmetric k nearest neighbour graph: adjacency[i] = {j: similarity}.
###############################################################################
def knn_graph(items, vectors, k, min_similarity, constraints):
    postings = defaultdict(list)
    for index, vector in enumerate(vectors):
        for token, value in vector.items():
            postings[token].append((index, value))

    by_guid = {item["guid"]: item["id"] for item in items if item["guid"]}
    isolated = {by_guid[guid] for guid in constraints["isolated"] if guid in by_guid}
    blocked = set()
    for pair in constraints["cannot_link"]:
        ids = [by_guid[guid] for guid in pair if guid in by_guid]
        for a in ids:
            for b in ids:
                if a != b:
                    blocked.add((a, b))

    adjacency = [dict() for _ in items]
    for index, vector in enumerate(vectors):
        if index in isolated:
            continue
        scores = defaultdict(float)
        for token, value in vector.items():
            for other, other_value in postings[token]:
                if other != index:
                    scores[other] += value * other_value
        best = sorted(((score, other) for other, score in scores.items()
                       if score >= min_similarity and other not in isolated
                       and (index, other) not in blocked), reverse=True)[:k]
        for score, other in best:
            score = round(min(score, 1.0), 4)
            adjacency[index][other] = max(adjacency[index].get(other, 0.0), score)
            adjacency[other][index] = adjacency[index][other]

    for group in constraints["must_link"]:
        ids = [by_guid[guid] for guid in group if guid in by_guid]
        for a in ids:
            for b in ids:
                if a != b:
                    adjacency[a][b] = 1.0
    return adjacency


###############################################################################
### Louvain community detection on a weighted undirected graph, with the
### aggregation phase. Return {node: community index} for the final level.
### param nodes: subset of the graph to partition.
### param resolution: > 1 gives smaller communities, < 1 larger ones.
###############################################################################
def louvain(nodes, adjacency, resolution, seed):
    index = {node: position for position, node in enumerate(nodes)}
    # Current (possibly aggregated) graph in local indices.
    edges = [defaultdict(float) for _ in nodes]
    for node in nodes:
        for neighbor, weight in adjacency[node].items():
            if neighbor in index:
                edges[index[node]][index[neighbor]] += weight
    membership = list(range(len(nodes)))   # original node -> current super node
    rng = random.Random(seed)

    while True:
        size = len(edges)
        degree = [sum(edge.values()) for edge in edges]
        total = sum(degree)                 # 2m
        if total <= 0:
            break
        community = list(range(size))
        sigma = list(degree)
        order = list(range(size))
        moved_any = False
        for _ in range(30):
            rng.shuffle(order)
            moved = False
            for node in order:
                current = community[node]
                links = defaultdict(float)
                for neighbor, weight in edges[node].items():
                    if neighbor != node:
                        links[community[neighbor]] += weight
                sigma[current] -= degree[node]
                best = current
                best_gain = links.get(current, 0.0) - resolution * sigma[current] * degree[node] / total
                for candidate, weight in links.items():
                    gain = weight - resolution * sigma[candidate] * degree[node] / total
                    if gain > best_gain + 1e-12:
                        best, best_gain = candidate, gain
                sigma[best] += degree[node]
                if best != current:
                    community[node] = best
                    moved = moved_any = True
            if not moved:
                break
        if not moved_any:
            break
        # Aggregation: one super node per community.
        renumber = {}
        for value in community:
            renumber.setdefault(value, len(renumber))
        if len(renumber) == size:
            break
        aggregated = [defaultdict(float) for _ in renumber]
        for node in range(size):
            source = renumber[community[node]]
            for neighbor, weight in edges[node].items():
                aggregated[source][renumber[community[neighbor]]] += weight
        membership = [renumber[community[super_node]] for super_node in membership]
        edges = aggregated

    return {node: membership[index[node]] for node in nodes}


###############################################################################
### Group nodes by their community index, largest groups first.
###############################################################################
def groups_of(partition):
    groups = defaultdict(list)
    for node, label in partition.items():
        groups[label].append(node)
    return sorted(groups.values(), key=len, reverse=True)


###############################################################################
### Merge the groups smaller than min_size into the group they are the most
### linked to. Groups without any link are returned apart.
###############################################################################
def absorb_small(groups, adjacency, min_size):
    owner = {}
    for position, group in enumerate(groups):
        for node in group:
            owner[node] = position
    alive = [list(group) for group in groups]
    left = []
    for position in sorted(range(len(groups)), key=lambda p: len(groups[p])):
        group = alive[position]
        if not group or len(group) >= min_size:
            continue
        scores = Counter()
        for node in group:
            for neighbor, weight in adjacency[node].items():
                other = owner.get(neighbor)
                if other is not None and other != position and alive[other]:
                    scores[other] += weight
        if not scores:
            continue
        target = scores.most_common(1)[0][0]
        for node in group:
            owner[node] = target
        alive[target].extend(group)
        alive[position] = []
    for group in alive:
        if group:
            left.append(group)
    return sorted(left, key=len, reverse=True)


###############################################################################
### Islands (coarse Louvain) then communities (fine Louvain inside each island).
###############################################################################
def detect(items, adjacency, args):
    linked = [item["id"] for item in items if adjacency[item["id"]]]
    orphans = [item["id"] for item in items if not adjacency[item["id"]]]

    island_groups = groups_of(louvain(linked, adjacency, args.island_resolution, args.seed))
    island_groups = absorb_small(island_groups, adjacency, MIN_COMMUNITY * 2)

    islands = []
    communities = []
    for island_members in island_groups:
        island = new_group(f"i{len(islands) + 1}", None)
        islands.append(island)
        fine = groups_of(louvain(island_members, adjacency,
                                 args.community_resolution, args.seed))
        fine = split_large(fine, adjacency, args.community_resolution, args.seed)
        fine = absorb_small(fine, adjacency, MIN_COMMUNITY)
        for members in fine:
            community = new_group(f"c{len(communities) + 1}", island["id"])
            community["bookmarks"] = sorted(members)
            communities.append(community)
            island["children"].append(community["id"])

    if orphans:
        island = new_group(ORPHANS, None)
        island["name"] = island["auto_name"] = ORPHANS_NAME
        community = new_group(ORPHANS + "-all", ORPHANS)
        community["name"] = community["auto_name"] = ORPHANS_NAME
        community["bookmarks"] = sorted(orphans)
        island["children"].append(community["id"])
        islands.append(island)
        communities.append(community)

    for community in communities:
        for identifier in community["bookmarks"]:
            items[identifier]["community"] = community["id"]
    return islands, communities


###############################################################################
### Split the communities larger than MAX_COMMUNITY with a higher resolution.
###############################################################################
def split_large(groups, adjacency, resolution, seed, depth=0):
    result = []
    for group in groups:
        if len(group) <= MAX_COMMUNITY or depth >= 3:
            result.append(group)
            continue
        parts = groups_of(louvain(group, adjacency, resolution * 2.0, seed))
        if len(parts) == 1:
            result.append(group)
        else:
            result.extend(split_large(parts, adjacency, resolution * 2.0, seed, depth + 1))
    return sorted(result, key=len, reverse=True)


###############################################################################
### Empty group record (island or community).
###############################################################################
def new_group(identifier, parent):
    return {
        "id": identifier,
        "name": "",
        "auto_name": "",
        "parent": parent,
        "children": [],
        "bookmarks": [],
        "keywords": [],
        "confidence": 0.0,
        "neighbors": [],
        "representatives": [],
        "outliers": [],
    }


###############################################################################
### Forced placements: {"bookmark": guid, "community": name}.
###############################################################################
def apply_moves(items, communities, islands, constraints):
    by_guid = {item["guid"]: item for item in items if item["guid"]}
    by_id = {community["id"]: community for community in communities}
    for move in constraints["moves"]:
        item = by_guid.get(move.get("bookmark"))
        name = (move.get("community") or "").strip()
        if item is None or not name:
            continue
        target = next((c for c in communities if c["name"] == name), None)
        if target is None:
            island = next((i for i in islands if i["id"] == "manual"), None)
            if island is None:
                island = new_group("manual", None)
                island["name"] = island["auto_name"] = "Classement manuel"
                islands.append(island)
            target = new_group(f"m{len(communities) + 1}", island["id"])
            target["name"] = target["auto_name"] = name
            target["manual"] = True
            island["children"].append(target["id"])
            communities.append(target)
            by_id[target["id"]] = target
        source = by_id.get(item.get("community"))
        if source is not None and item["id"] in source["bookmarks"]:
            source["bookmarks"].remove(item["id"])
        target["bookmarks"].append(item["id"])
        item["community"] = target["id"]


###############################################################################
### Names, keywords, confidence, neighbours, representatives and outliers.
###############################################################################
def describe_all(items, vectors, adjacency, islands, communities, surfaces):
    community_of = {item["id"]: item["community"] for item in items}
    island_of = {community["id"]: community["parent"] for community in communities}
    global_weight = Counter()
    for vector in vectors:
        global_weight.update(vector)

    for community in communities:
        members = community["bookmarks"]
        keywords = top_terms(members, items, vectors, global_weight, surfaces, 6)
        community["keywords"] = keywords
        if not community["auto_name"]:
            community["auto_name"] = make_name(members, items, keywords)
            community["name"] = community["auto_name"]
        community["confidence"] = round(internal_density(members, adjacency), 3)
        community["representatives"] = representatives(members, items, adjacency)
        community["outliers"] = outliers(members, adjacency)
        community["neighbors"] = neighbor_groups(members, adjacency, community_of,
                                                 community["id"])

    for island in islands:
        members = [identifier for community in communities
                   if community["parent"] == island["id"]
                   for identifier in community["bookmarks"]]
        island["size"] = len(members)
        keywords = top_terms(members, items, vectors, global_weight, surfaces, 6)
        island["keywords"] = keywords
        if not island["auto_name"]:
            island["auto_name"] = make_name(members, items, keywords)
            island["name"] = island["auto_name"]
        island["confidence"] = round(internal_density(members, adjacency), 3)
        island_members = {i: island_of.get(c) for i, c in community_of.items()}
        island["neighbors"] = neighbor_groups(members, adjacency, island_members,
                                              island["id"])

    for item in items:
        # Secondary communities: strongly linked ones other than the primary.
        scores = Counter()
        for neighbor, weight in adjacency[item["id"]].items():
            other = community_of.get(neighbor)
            if other and other != item["community"]:
                scores[other] += weight
        item["also"] = [other for other, score in scores.most_common(2) if score >= 0.6]
        vector = vectors[item["id"]]
        best = sorted((value, token) for token, value in vector.items()
                      if token in item["name_tokens"])
        item["keywords"] = [bookmarklib.preferred_surface(token, surfaces)
                            for _, token in reversed(best[-8:])]


###############################################################################
### Most distinctive terms of a group (summed TF-IDF, normalized by the
### frequency of the term in the whole collection).
###############################################################################
def top_terms(members, items, vectors, global_weight, surfaces, count):
    weight = Counter()
    for identifier in members:
        allowed = items[identifier]["name_tokens"]
        for token, value in vectors[identifier].items():
            if token in allowed:
                weight[token] += value
    support = Counter()
    for identifier in members:
        support.update(token for token in vectors[identifier] if token in weight)
    # Shared by many members, and specific to the group.
    ranked = sorted(weight.items(),
                    key=lambda pair: support[pair[0]] * pair[1] / global_weight[pair[0]],
                    reverse=True)
    minimum = 2 if len(members) >= 4 else 1
    terms = []
    for token, value in ranked:
        if support[token] < minimum:
            continue
        terms.append(bookmarklib.preferred_surface(token, surfaces))
        if len(terms) == count:
            break
    return terms


###############################################################################
### Provisional name: dominant site when there is one, otherwise top terms.
###############################################################################
def make_name(members, items, keywords):
    sites = Counter(items[i]["site"] for i in members
                    if items[i]["domain"] and items[i]["domain"] not in bookmarklib.HUB_DOMAINS)
    terms = keywords[:3]
    if sites:
        site, count = sites.most_common(1)[0]
        if count >= max(2, 0.6 * len(members)):
            extra = [t for t in terms if bookmarklib.fold(t) not in bookmarklib.fold(site)]
            return " · ".join([site] + extra[:1])
    return " · ".join(terms) or "Sans nom"


###############################################################################
### Mean weight of the links inside a group, edges counted once.
###############################################################################
def internal_density(members, adjacency):
    keep = set(members)
    weights = [weight for node in members for neighbor, weight in adjacency[node].items()
               if neighbor in keep and node < neighbor]
    return sum(weights) / len(weights) if weights else 0.0


###############################################################################
### Bookmarks the most linked inside their group.
###############################################################################
def representatives(members, items, adjacency):
    keep = set(members)
    return sorted(members, key=lambda i: (
        sum(w for n, w in adjacency[i].items() if n in keep), items[i]["visit_count"]),
        reverse=True)[:5]


###############################################################################
### Bookmarks that have no link inside their group.
###############################################################################
def outliers(members, adjacency):
    if len(members) < 4:
        return []
    keep = set(members)
    return [i for i in members if not any(n in keep for n in adjacency[i])][:10]


###############################################################################
### Groups sharing the strongest links with this one.
###############################################################################
def neighbor_groups(members, adjacency, group_of, own):
    scores = Counter()
    for node in members:
        for neighbor, weight in adjacency[node].items():
            other = group_of.get(neighbor)
            if other and other != own:
                scores[other] += weight
    return [other for other, _ in scores.most_common(4)]


###############################################################################
### Manual names: the community (or island) sharing most of the given
### bookmarks takes the name.
###############################################################################
def apply_names(communities, islands, items, constraints):
    by_guid = {item["guid"]: item["id"] for item in items if item["guid"]}
    for entry in constraints["names"]:
        wanted = {by_guid[g] for g in entry.get("bookmarks") or [] if g in by_guid}
        name = (entry.get("name") or "").strip()
        if not wanted or not name:
            continue
        best, best_score = None, 0.0
        for group in communities + islands:
            members = set(group["bookmarks"]) if group["parent"] else {
                i for c in communities if c["parent"] == group["id"] for i in c["bookmarks"]}
            if not members:
                continue
            common = len(wanted & members)
            score = common / len(wanted | members)
            if score > best_score:
                best, best_score = group, score
        if best is not None and best_score >= 0.5:
            best["name"] = name


###############################################################################
### Records written to organization.json.
###############################################################################
def public_group(group):
    record = {key: group[key] for key in (
        "id", "name", "auto_name", "parent", "children", "bookmarks", "keywords",
        "confidence", "neighbors", "representatives", "outliers")}
    if "size" in group:
        record["size"] = group["size"]
    return record


def public_bookmark(item):
    return {
        "id": item["id"],
        "guid": item["guid"],
        "url": item["url"],
        "title": item["original_title"] or item["title"],
        "folder": item["folder"],
        "domain": item["domain"],
        "tags": item["tags"],
        "keywords": item["keywords"],
        "community": item["community"],
        "also": item["also"],
        "date_added": item["date_added"],
        "last_modified": item["last_modified"],
        "visit_count": item["visit_count"],
    }


def public_links(adjacency):
    return [[node, neighbor, weight]
            for node, edges in enumerate(adjacency)
            for neighbor, weight in sorted(edges.items()) if node < neighbor]


###############################################################################
### Console summary for the human review.
###############################################################################
def report(path, items, islands, communities, duplicates):
    orphans = sum(len(c["bookmarks"]) for c in communities if c["parent"] == ORPHANS)
    sizes = sorted((len(c["bookmarks"]) for c in communities if c["parent"] != ORPHANS),
                   reverse=True)
    print(f"Written: {path}")
    print(f"{len(items)} bookmarks ({duplicates} duplicate URLs merged), "
          f"{len(islands)} islands, {len(communities)} communities, {orphans} orphans")
    if sizes:
        print(f"community size: max {sizes[0]}, median {sizes[len(sizes) // 2]}, "
              f"min {sizes[-1]}")
    print()
    by_island = defaultdict(list)
    for community in communities:
        by_island[community["parent"]].append(community)
    for island in sorted(islands, key=lambda i: i.get("size", 0), reverse=True):
        print(f"[{island['id']}] {island['name']}  ({island.get('size', 0)} bookmarks)")
        for community in sorted(by_island[island["id"]],
                                key=lambda c: len(c["bookmarks"]), reverse=True)[:6]:
            print(f"    {community['name']}  ({len(community['bookmarks'])}, "
                  f"confidence {community['confidence']})")


if __name__ == "__main__":
    main()
