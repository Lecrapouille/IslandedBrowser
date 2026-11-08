###################################################
# IslandedBrowser, built by MyMakefile 3.
# https://github.com/Lecrapouille/MyMakefile
###################################################

PROJECT_NAME := IslandedBrowser
PROJECT_VERSION := 0.1.0
PROJECT_DESCRIPTION := Firefox bookmarks drawn as an island
LICENSE := MIT
CXX_STANDARD := c++14
PREFIX ?= /usr

TARGETS := IslandedBrowser
IslandedBrowser.DESCRIPTION := Firefox bookmarks drawn as an island
IslandedBrowser.SOURCES := src/*.cpp
IslandedBrowser.INCLUDES := src
IslandedBrowser.GENERATED_SOURCES := Bookmarks.cpp
IslandedBrowser.CXXFLAGS := -fopenmp -Wno-switch-enum -Wno-undef \
	-Wno-unused-parameter -Wno-old-style-cast -Wno-sign-conversion \
	-Wno-c++20-extensions
IslandedBrowser.LDFLAGS := -fopenmp
IslandedBrowser.PKG_CONFIG := sfml-graphics
IslandedBrowser.DATA := data

M ?= .makefile
include $(M)/MyMakefile.mk

# Produced from the Firefox JSON export. Save bookmarks/bookmarks.json first.
$(GEN_DIR)/Bookmarks.cpp: bookmarks/parser.py bookmarks/bookmarks.json
	$(call print-to,Generating,bookmarks,$@)
	$(Q)mkdir -p $(dir $@)
	$(Q)./bookmarks/parser.py bookmarks/bookmarks.json $@
