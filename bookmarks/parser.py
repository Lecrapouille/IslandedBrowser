#!/usr/bin/env python3
###############################################################################
### Parse a Firefox bookmark JSON file and generate a C++ source file adapted
### for this particular project.
###
### See the Firefox document to know how to save your bookmarks as JSON:
### https://support.mozilla.org/en-US/kb/restore-bookmarks-from-backup-or-move-them#w_backup-and-restore
###
### This script is based on this blog (initially I wanted to use jq but this
### was too complex for parsing but it can be used to pretty print your JSON file):
### https://johackim.com/exporter-ses-bookmarks-firefox-en-markdown
###
### See this document explaining fields of the Firefox bookmark:
### https://searchfox.org/mozilla-release/source/toolkit/components/places/Bookmarks.jsm
###############################################################################

import json, sys, os

###############################################################################
### Structure holding information after having parsed a JSON folder.
###############################################################################
class Folder(object):
    def __init__(self, title, uid, parent):
        # String
        self.title = title
        # Integer
        self.uid = uid
        # Integer
        self.parent = parent

    def stringify(self):
        return 'Folder "' + self.title + '" (parent: ' + str(self.parent) + ': "' + folders[self.parent].title + '")\n'

    def __str__(self):
        return self.stringify()

    def __repr__(self):
        return self.stringify()

###############################################################################
### Structure holding information after having parsed a JSON bookmark.
###############################################################################
class Bookmark(object):
    def __init__(self, title, uri, uid, parent):
        # String
        self.title = title
        # String
        self.uri = uri
        # Integer
        self.uid = uid
        # Integer
        self.parent = parent

    def stringify(self):
        return 'Bookmark "' + self.title + '": "' + self.uri + '" (parent: ' + str(self.parent) + ': "' + folders[self.parent].title + '")\n'

    def __str__(self):
        return self.stringify()

    def __repr__(self):
        return self.stringify()

###############################################################################
### Context of the parser translating a JSON file depicting a Firefox bookmarks
### into a C++ file state machine holding bookmarks as graph structure.
###############################################################################
class Parser(object):

    ###########################################################################
    ### Default dummy constructor.
    ###########################################################################
    def __init__(self):
        # JSON file as dictionary
        self.jdict = None
        # Bookmarks that are not folders (tree leaves)
        self.bookmarks = dict()
        # Bookmarks that are folders (tree nodes)
        self.folders = dict()

    ###########################################################################
    ### Entry point for parsing JSON file and generate C++ file
    ###########################################################################
    def parse_firefox_bookmark(self, json_file, cpp_file):
        self.jdict = json.load(open(json_file, 'r'))
        self.extract(self.jdict)
        #print('folders:', self.folders)
        #print('bookmarks:', self.bookmarks)
        self.generate(cpp_file)

    ###########################################################################
    ### Extract needed information from the JSON dictionary.
    ### param json: the JSON dictionary.
    ### param parent: root node uid (we start from 0 while Firefox starts at 1)
    ###########################################################################
    def extract(self, json, parent=0):
        uid = json['id'] - 1
        # Tree leaves
        if 'children' not in json.keys():
            try:
                self.bookmarks[uid] = Bookmark(json['title'].replace('"', '\''), json['uri'], uid, parent)
            except:
                pass
            return

        # Tree nodes
        self.folders[uid] = Folder(json['title'].replace('"', '\''), uid, parent)

        # Recursivity under child nodes (folders)
        for children in json['children']:
            self.extract(children, uid)

    ###############################################################################
    ### Generate the C++ file
    ###############################################################################
    def generate(self, path):
        fd = open(path, 'w')

        fd.write('// This file has been automatically created\n')
        fd.write('#include "IslandedBrowser.hpp"\n\n')

        fd.write('void IslandedBrowser::init(Bookmarks& bookmarks, Folders& folders)\n{\n')
        fd.write('    folders =\n')
        fd.write('    {\n')
        for key, folder in self.folders.items():
            fd.write('        { ' + str(key) + ', { .title = "' + folder.title + '", .id = ' + str(folder.uid) + ', .parent = ' + str(folder.parent) + ' } },\n')
        fd.write('    };\n\n')

        fd.write('    bookmarks =\n')
        fd.write('    {\n')
        for key, bookmark in self.bookmarks.items():
            fd.write('        { ' + str(key) + ', { .title = "' + bookmark.title + '", .uri = "' + bookmark.uri + '", .id = ' + str(bookmark.uid) + ', .parent = ' + str(bookmark.parent) + ' } },\n')
        fd.write('    };\n')
        fd.write('}')
        fd.close()

###############################################################################
### Display command line usage
###############################################################################
def usage():
    print('Command line: ' + sys.argv[0] + ' <JSON file> <C++ file>')
    print('Where:')
    print('   <JSON file>: the path of the Firefox JSON file of exported bookmarks')
    print('   [state machine name]: is an optional name to postfix the name of the state machine class')
    print('Example:')
    print('   ' + sys.argv[1] + 'bookmarks.json bookmarks.cpp')
    print('Will create a C++ file bookmarks.cpp file holding bookmarks in graph structure')
    sys.exit(-1)

###############################################################################
### Entry point.
### argv[1] Mandatory: path of the JSON file
### argv[2] Mandatory: path of the C++ file to create.
###############################################################################
def main():
    argc = len(sys.argv)
    if argc < 2:
        usage()

    parser = Parser()
    parser.parse_firefox_bookmark(sys.argv[1], sys.argv[2])


if __name__ == '__main__':
    main()
