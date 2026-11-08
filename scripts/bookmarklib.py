#!/usr/bin/env python3
###############################################################################
### Shared helpers for the Firefox export and the thematic classification.
###
### The original bookmark record is never rewritten in place. Computed fields
### are metadata carried beside it.
###############################################################################

import re, shutil, sqlite3, tempfile, unicodedata
from collections import defaultdict
from datetime import datetime, timezone
from pathlib import Path

# Generated files hold personal data (URLs, titles, visit counts): they all go
# in private/ at the root of the project, which git ignores. examples/ holds
# the same files, with the same names, made from public bookmarks.
PRIVATE_DIR = Path(__file__).resolve().parent.parent / "private"


###############################################################################
### The option shared by every program of the project: -d DIR, the folder of
### the files (bookmarks.json, organization.json, layout.json...).
###############################################################################
def add_dir_argument(parser):
    parser.add_argument("-d", "--dir", type=Path, default=PRIVATE_DIR,
                        help="folder of the files (default: private/; "
                             "examples/ holds the public example)")


###############################################################################
### A file given with -i/-o, or else its usual name in the folder of -d.
###############################################################################
def file_in(given, directory, name):
    return given if given is not None else directory / name

# Platforms where the domain says almost nothing about the subject.
HUB_DOMAINS = {
    "github.com", "gist.github.com", "gitlab.com", "bitbucket.org",
    "codeberg.org", "sourceforge.net",
    "youtube.com", "youtu.be", "m.youtube.com",
    "wikipedia.org", "mediawiki.org",
    "stackoverflow.com", "stackexchange.com", "superuser.com",
    "serverfault.com", "askubuntu.com",
    "reddit.com", "old.reddit.com",
    "medium.com", "dev.to",
    "twitter.com", "x.com", "facebook.com", "instagram.com", "linkedin.com",
    "google.com", "sites.google.com", "docs.google.com", "drive.google.com",
    "amazon.com", "amazon.fr", "amazon.co.uk",
    "aliexpress.com", "fr.aliexpress.com",
    "arxiv.org", "researchgate.net", "scholar.google.com",
    "npmjs.com", "pypi.org", "crates.io", "rubygems.org",
}

# Tokens that appear in titles without saying what the page is about.
TOKEN_NOISE = {
    "video", "youtube", "watch", "html", "htm", "php", "asp", "aspx",
    "index", "default", "home", "page", "www", "http", "https", "pdf",
    "amp", "utm", "ref", "source", "official", "online", "free", "full",
    "part", "episode", "doc", "docs", "documentation", "wiki", "blog",
    "post", "thread", "comment", "comments", "question", "answer",
    "github", "gitlab", "stackoverflow", "wikipedia", "reddit", "medium",
    "google", "amazon", "stable", "latest", "download", "version",
    "release", "guide", "intro", "introduction", "example", "examples",
    "simple", "based", "overview", "holder", "document", "untitled",
    "english", "french", "francais", "deutsch", "spanish",
}

# Kept even though they are shorter than three characters.
SHORT_TOKENS = {
    "js", "ai", "ml", "ui", "ux", "vr", "ar", "ik", "os", "qt", "gl",
    "tf", "np", "cv", "rl", "3d",
}

STOPWORDS = {
    "the", "and", "for", "with", "from", "that", "this", "you", "your",
    "in", "to", "of", "on", "by", "do", "or", "an", "at", "as", "is",
    "it", "be", "we", "my", "no", "so", "up", "if", "le", "la", "de",
    "du", "un", "et", "en", "au", "ce", "sa", "se", "ne", "ou", "il",
    "je", "tu", "ma", "me", "te", "fr", "com", "org", "net",
    "are", "was", "were", "have", "has", "not", "but", "can", "how",
    "what", "when", "where", "which", "who", "why", "into", "onto",
    "about", "over", "under", "than", "then", "them", "they", "their",
    "its", "it's", "using", "use", "used", "via", "per", "out", "all",
    "any", "more", "most", "some", "such", "only", "just", "new", "one",
    "two", "get", "got", "let", "make", "made", "like", "dont", "does",
    "did", "will", "would", "should", "could", "les", "des", "une", "est",
    "pas", "que", "qui", "dans", "pour", "sur", "avec", "par", "plus",
    "son", "ses", "aux", "une", "dun", "dune", "chez", "cette", "ces",
    "comme", "mais", "ou", "où", "donc", "car", "sans", "sous", "entre",
    "après", "avant", "tout", "tous", "toute", "toutes", "leur", "leurs",
    "elle", "elles", "nous", "vous", "ils", "elles", "été", "etre", "être",
    "fait", "faire", "aussi", "très", "tres", "peut", "peuvent", "autre",
    "autres", "même", "meme", "ainsi", "alors", "depuis", "encore",
    "comment", "pourquoi", "quand", "vers", "chez",
}

TITLE_SUFFIXES = (
    " - youtube", " - stack overflow", " - github", " - gitlab",
    " - google docs", " - wikipedia", " — wikipédia", " - wikipédia",
    " - reddit", " - medium", " - amazon.fr", " - amazon.com",
)

GUID_TITLE = {
    "menu________": "Menu des marque-pages",
    "toolbar_____": "Barre personnelle",
    "unfiled_____": "Autres marque-pages",
    "mobile______": "Marque-pages mobiles",
    "tags________": "Étiquettes",
}

GUID_ROOT = {
    "root________": "placesRoot",
    "menu________": "bookmarksMenuFolder",
    "toolbar_____": "toolbarFolder",
    "unfiled_____": "unfiledBookmarksFolder",
    "mobile______": "mobileFolder",
    "tags________": "tagsFolder",
}

MULTI_TLDS = {
    "co.uk", "org.uk", "ac.uk", "gov.uk",
    "co.jp", "ne.jp", "or.jp",
    "com.au", "net.au", "org.au",
    "co.nz", "com.br", "com.tr", "com.mx",
    "github.io", "gitlab.io", "blogspot.com", "appspot.com",
    "wordpress.com", "free.fr", "ovh.net", "ovh.org",
    "netlify.app", "vercel.app", "pages.dev", "cloudfront.net",
}

SPECIAL_TOKENS = re.compile(r"c\+\+|c#|\.net\b", re.IGNORECASE)
WORD_TOKENS = re.compile(r"[a-z0-9]{2,}")


###############################################################################
### Directories where a Firefox profile may live.
###############################################################################
def firefox_bases():
    home = Path.home()
    return [
        home / ".mozilla" / "firefox",
        home / "snap" / "firefox" / "common" / ".mozilla" / "firefox",
        home / ".var" / "app" / "org.mozilla.firefox" / ".mozilla" / "firefox",
    ]


###############################################################################
### Parse a Firefox profiles.ini file into sections.
###############################################################################
def _ini_sections(path):
    sections = []
    current = {}
    for line in path.read_text(encoding="utf-8", errors="replace").splitlines():
        line = line.strip()
        if not line or line.startswith(("#", ";")):
            continue
        if line.startswith("[") and line.endswith("]"):
            current = {}
            sections.append(current)
            continue
        if "=" in line:
            key, value = line.split("=", 1)
            current[key.strip()] = value.strip()
    return sections


###############################################################################
### Return the profile directory that contains places.sqlite.
### param explicit: profile directory given on the command line.
###############################################################################
def find_firefox_profile(explicit=None):
    if explicit:
        profile = Path(explicit).expanduser()
        if (profile / "places.sqlite").is_file():
            return profile
        raise SystemExit(f"No places.sqlite in {profile}")

    for base in firefox_bases():
        ini = base / "profiles.ini"
        if not ini.is_file():
            continue
        for section in _ini_sections(ini):
            if section.get("Default") == "1" and section.get("Path"):
                path = Path(section["Path"])
                profile = path if path.is_absolute() else base / path
                if (profile / "places.sqlite").is_file():
                    return profile
        for section in _ini_sections(ini):
            default = section.get("Default")
            if default and "Path" not in section:
                path = Path(default)
                profile = path if path.is_absolute() else base / path
                if (profile / "places.sqlite").is_file():
                    return profile
    raise SystemExit(
        "Firefox profile not found. Give it with --profile PATH.")


###############################################################################
### Open a private read-only snapshot of places.sqlite.
### Firefox keeps the live database locked. The caller deletes the
### temporary directory returned with the connection.
###############################################################################
def open_places(profile):
    source = Path(profile) / "places.sqlite"
    temporary = Path(tempfile.mkdtemp(prefix="islanded-places-"))
    target = temporary / "places.sqlite"
    for suffix in ("", "-wal", "-shm"):
        origin = Path(str(source) + suffix)
        if origin.is_file():
            shutil.copy2(origin, Path(str(target) + suffix))
    connection = sqlite3.connect(f"file:{target}?mode=ro", uri=True)
    connection.row_factory = sqlite3.Row
    return connection, temporary


###############################################################################
### Close the snapshot and delete its temporary copy.
###############################################################################
def close_places(connection, temporary):
    connection.close()
    shutil.rmtree(temporary, ignore_errors=True)


###############################################################################
### Convert a Firefox PRTime, in microseconds, to an ISO date.
###############################################################################
def prtime_to_iso(value):
    if not value:
        return None
    try:
        instant = datetime.fromtimestamp(float(value) / 1_000_000, timezone.utc)
    except (OverflowError, OSError, ValueError):
        return None
    return instant.isoformat()


###############################################################################
### Lower-case a string and strip its accents.
###############################################################################
def fold(text):
    normalized = unicodedata.normalize("NFKD", text or "")
    without_marks = "".join(
        char for char in normalized if not unicodedata.combining(char))
    return without_marks.lower()


###############################################################################
### Drop the site suffix Firefox appends to a bookmark title.
###############################################################################
def clean_title(title):
    text = " ".join((title or "").split())
    folded = fold(text)
    for suffix in TITLE_SUFFIXES:
        if folded.endswith(suffix):
            text = text[:len(text) - len(suffix)].rstrip()
            folded = fold(text)
    return text


###############################################################################
### Split a text into keywords, ignoring stopwords and noise.
###############################################################################
def tokenize(text):
    folded = fold(text)
    tokens = {token.lower() for token in SPECIAL_TOKENS.findall(folded)}
    for token in WORD_TOKENS.findall(folded):
        if len(token) >= 3 or token in SHORT_TOKENS:
            tokens.add(token)
    return {token for token in tokens if token not in STOPWORDS and token not in TOKEN_NOISE}


###############################################################################
### Registrable domain of a hostname.
###############################################################################
def registrable_domain(hostname):
    host = (hostname or "").lower().strip(".").removeprefix("www.")
    if not host or _is_ip(host):
        return host
    labels = host.split(".")
    if len(labels) <= 2:
        return host
    tail2 = ".".join(labels[-2:])
    tail3 = ".".join(labels[-3:])
    if tail2 in MULTI_TLDS and len(labels) >= 3:
        return ".".join(labels[-3:])
    if tail3 in MULTI_TLDS and len(labels) >= 4:
        return ".".join(labels[-4:])
    return tail2


###############################################################################
### True when the host is an IPv4 address.
###############################################################################
def _is_ip(host):
    parts = host.split(".")
    return len(parts) == 4 and all(part.isdigit() for part in parts)


###############################################################################
### Human readable name of a domain.
###############################################################################
def site_label(domain):
    if not domain:
        return ""
    known = {
        "github.com": "GitHub",
        "gist.github.com": "Gist",
        "gitlab.com": "GitLab",
        "bitbucket.org": "Bitbucket",
        "youtube.com": "YouTube",
        "youtu.be": "YouTube",
        "stackoverflow.com": "Stack Overflow",
        "wikipedia.org": "Wikipédia",
        "arxiv.org": "arXiv",
        "docs.google.com": "Google Docs",
        "godotengine.org": "Godot",
        "codeproject.com": "CodeProject",
        "thingiverse.com": "Thingiverse",
        "mixcloud.com": "Mixcloud",
        "college-de-france.fr": "Collège de France",
    }
    if domain in known:
        return known[domain]
    label = domain.split(".")[0]
    return label[:1].upper() + label[1:]


###############################################################################
### Visit the URL bookmarks of a Firefox backup tree.
### param sink: called as sink(node, folder_path) for each URL.
### The tags folder is not walked as bookmarks; its names are collected.
###############################################################################
def walk_firefox_tree(node, folder, sink, harvest_tags=None):
    root = node.get("root") or ""
    guid = node.get("guid") or ""
    if root == "tagsFolder" or guid == "tags________":
        if harvest_tags is not None:
            _harvest_tags(node, harvest_tags)
        return

    children = node.get("children")
    is_folder = children is not None and node.get("typeCode") != 1
    if is_folder:
        title = node.get("title") or ""
        path = folder
        if title and root != "placesRoot":
            path = folder + [title]
        for child in children:
            walk_firefox_tree(child, path, sink, harvest_tags)
        return

    if node.get("uri"):
        sink(node, folder)


###############################################################################
### Collect the tag names stored under the tags folder.
###############################################################################
def _harvest_tags(folder, found):
    for tag_folder in folder.get("children") or []:
        name = tag_folder.get("title") or ""
        if not name:
            continue
        for bookmark in tag_folder.get("children") or []:
            uri = bookmark.get("uri")
            if uri:
                found[uri].append(name)


###############################################################################
### Load bookmark rows and index them by identifier and parent.
###############################################################################
def load_rows(connection):
    view_time = dict(connection.execute(
        "SELECT place_id, SUM(total_view_time) "
        "FROM moz_places_metadata GROUP BY place_id"))
    rows = connection.execute(
        """
        SELECT b.id, b.type, b.parent, b.position, b.title, b.dateAdded,
               b.lastModified, b.guid, b.fk,
               p.url, p.title AS place_title, p.visit_count, p.frecency,
               p.last_visit_date, p.description
        FROM moz_bookmarks b
        LEFT JOIN moz_places p ON p.id = b.fk
        ORDER BY b.parent, b.position, b.id
        """).fetchall()
    by_parent = defaultdict(list)
    by_id = {}
    for row in rows:
        record = dict(row)
        record["view_time"] = view_time.get(record["fk"]) or 0
        by_id[record["id"]] = record
        by_parent[record["parent"]].append(record)
    return by_id, by_parent


###############################################################################
### Map each URL to the Firefox tags that point at it.
###############################################################################
def tags_by_url(by_id, by_parent):
    tags_root = next(
        (row for row in by_id.values() if row["guid"] == "tags________"), None)
    found = defaultdict(list)
    if tags_root is None:
        return found
    for tag_folder in by_parent.get(tags_root["id"], []):
        if tag_folder["type"] != 2:
            continue
        name = tag_folder["title"] or ""
        if not name:
            continue
        for bookmark in by_parent.get(tag_folder["id"], []):
            url = bookmark.get("url")
            if url:
                found[url].append(name)
    return found


###############################################################################
### Build one bookmark node of a Firefox backup JSON.
###############################################################################
def bookmark_node(row, tags):
    title = row["title"] or row["place_title"] or ""
    node = {
        "guid": row["guid"],
        "title": title,
        "index": row["position"],
        "dateAdded": row["dateAdded"],
        "lastModified": row["lastModified"],
        "id": row["id"],
        "typeCode": 1,
        "type": "text/x-moz-place",
        "uri": row["url"],
        "visitCount": row["visit_count"] or 0,
        "frecency": row["frecency"] or 0,
        "description": (row["description"] or "").strip(),
        "viewTime": row["view_time"] or 0,
    }
    if row["last_visit_date"]:
        node["lastVisitDate"] = row["last_visit_date"]
    names = tags.get(row["url"]) or []
    if names:
        # Firefox backups store tags as one comma separated string.
        node["tags"] = join_tags(names)
    return node


###############################################################################
### Build one folder node of a Firefox backup JSON.
###############################################################################
def folder_node(row):
    guid = row["guid"]
    title = row["title"] or ""
    if guid in GUID_TITLE and title in ("menu", "toolbar", "unfiled", "mobile", "tags", ""):
        title = GUID_TITLE[guid]
    node = {
        "guid": guid,
        "title": title,
        "index": row["position"] or 0,
        "dateAdded": row["dateAdded"],
        "lastModified": row["lastModified"],
        "id": row["id"],
        "typeCode": 2,
        "type": "text/x-moz-place-container",
        "children": [],
    }
    root = GUID_ROOT.get(guid)
    if root:
        node["root"] = root
    return node


###############################################################################
### Build the backup tree, without duplicating the tag folder.
###############################################################################
def build_firefox_tree(by_id, by_parent, tags):
    def attach(row):
        if row["guid"] == "tags________":
            return None
        if row["type"] == 2:
            node = folder_node(row)
            for child in by_parent.get(row["id"], []):
                built = attach(child)
                if built is not None:
                    node["children"].append(built)
            return node
        if row["type"] == 1 and row.get("url"):
            return bookmark_node(row, tags)
        if row["type"] == 3:
            return {
                "guid": row["guid"],
                "title": row["title"] or "",
                "index": row["position"],
                "id": row["id"],
                "typeCode": 3,
                "type": "text/x-moz-place-separator",
            }
        return None

    root = by_id.get(1) or next(
        row for row in by_id.values() if row["guid"] == "root________")
    return attach(root)


###############################################################################
### Tags of a bookmark node. Firefox writes a comma separated string; older
### versions of firefox_export.py wrote a list. Both are accepted.
###############################################################################
def tags_of(node):
    raw = node.get("tags") or []
    if isinstance(raw, str):
        raw = raw.split(",")
    return [tag.strip() for tag in raw if isinstance(tag, str) and tag.strip()]


###############################################################################
### Join tag names the way Firefox stores them. Commas are not allowed inside
### a tag and Firefox drops tags longer than 100 characters.
###############################################################################
def join_tags(names):
    clean = []
    for name in names:
        name = " ".join(str(name).replace(",", " ").split())[:100]
        if name and name not in clean:
            clean.append(name)
    return ",".join(clean)


###############################################################################
### Count the surface forms of the words of a text.
###############################################################################
def remember_surfaces(text, surfaces):
    for raw in re.findall(r"[^\W\d_]{2,}|c\+\+|c#", text or "", flags=re.UNICODE):
        surfaces[fold(raw)][raw] += 1


###############################################################################
### Most frequent spelling of a folded token.
###############################################################################
def preferred_surface(token, surfaces):
    options = surfaces.get(token)
    if not options:
        return token
    return options.most_common(1)[0][0]
