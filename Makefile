## MIT License
##
## Copyright (c) 2022 Quentin Quadrat
##
## Permission is hereby granted, free of charge, to any person obtaining a copy
## of this software and associated documentation files (the "Software"), to deal
## in the Software without restriction, including without limitation the rights
## to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
## copies of the Software, and to permit persons to whom the Software is
## furnished to do so, subject to the following conditions:
##
## The above copyright notice and this permission notice shall be included in all
## copies or substantial portions of the Software.
##
## THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
## IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
## FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
## AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
## LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
## OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
## SOFTWARE.

TARGET_BIN = IslandedBrowser

PARSER=./bookmarks/parser.py

# Needed for the command: make install
DESTDIR ?=
PREFIX ?= /usr
BINDIR := $(DESTDIR)$(PREFIX)/bin
LIBDIR := $(DESTDIR)$(PREFIX)/lib
DATADIR := $(DESTDIR)$(PREFIX)/share/$(TARGET_BIN)/data

# Compilation searching files
BUILD = build
VPATH = $(BUILD) src src/Renderer
INCLUDES = -Isrc -Isrc/Renderer

# C++14 (only because of std::make_unique not present in C++11)
STANDARD=--std=c++14

# Compilation flags
COMPIL_FLAGS = -Wall -Wextra -Wuninitialized -Wundef -Wunused   \
  -Wunused-result -Wunused-parameter -Wtype-limits -Wshadow     \
  -Wcast-align -Wcast-qual -Wconversion -Wfloat-equal           \
  -Wpointer-arith -Wswitch-enum -Wpacked -Wold-style-cast       \
  -Wdeprecated -Wvariadic-macros -Wvla -Wsign-conversion        \
  -D_GLIBCXX_ASSERTIONS

COMPIL_FLAGS += -Wno-switch-enum -Wno-undef -Wno-unused-parameter \
  -Wno-old-style-cast -Wno-sign-conversion

# Project flags
CXXFLAGS += $(STANDARD) $(COMPIL_FLAGS) -fopenmp
LDFLAGS += -lpthread -fopenmp
DEFINES += -DDATADIR=\"$(DATADIR)\"

# Lib SFML https://www.sfml-dev.org/index-fr.php
CXXFLAGS += `pkg-config --cflags sfml-graphics`
LDFLAGS += `pkg-config --libs sfml-graphics`

## Pretty print the stack trace https://github.com/bombela/backward-cpp
## You can comment these lines if backward-cpp is not desired
CXXFLAGS += -g -O0
LDFLAGS += -ldw
DEFINES += -DBACKWARD_HAS_DW=1
OBJS += backward.o

# Header file dependencies
DEPFLAGS = -MT $@ -MMD -MP -MF $(BUILD)/$*.Td
POSTCOMPILE = mv -f $(BUILD)/$*.Td $(BUILD)/$*.d

# Desired compiled files for the shared library
OBJS += Bookmarks.o ForceDirectedGraph.o IslandedBrowser.o Application.o IslandedBrowserGUI.o main.o

# Verbosity control
ifeq ($(VERBOSE),1)
Q :=
else
Q := @
endif

# Compile the target
all: $(TARGET_BIN)

# Link the target
$(TARGET_BIN): $(LIB_OBJS) $(OBJS)
	@echo "Linking $@"
	$(Q)cd $(BUILD) && $(CXX) $(INCLUDES) -o $(TARGET_BIN) $(LIB_OBJS) $(OBJS) $(LDFLAGS)

# Compile C++ source files
%.o : %.cpp $(BUILD)/%.d Makefile
	@echo "Compiling $<"
	$(Q)$(CXX) $(DEPFLAGS) -fPIC $(CXXFLAGS) $(INCLUDES) $(DEFINES) -c $(abspath $<) -o $(abspath $(BUILD)/$@)
	@$(POSTCOMPILE)

# Compile C source files
%.o : %.c $(BUILD)/%.d Makefile
	@echo "Compiling $<"
	$(Q)$(CXX) $(DEPFLAGS) -fPIC $(CXXFLAGS) $(INCLUDES) $(DEFINES) -c $(abspath $<) -o $(abspath $(BUILD)/$@)
	@$(POSTCOMPILE)

src/Bookmarks.cpp: $(PARSER) bookmarks/bookmarks.json
	@echo "Generating bookmarks"
	$(Q)$(PARSER) bookmarks/bookmarks.json $@

# Install the project
.PHONY: install
install: $(TARGET_BIN) $(TARGET_LIB)
	@echo "Installing $(TARGET_BIN)"
	$(Q)mkdir -p $(BINDIR)
	$(Q)mkdir -p $(LIBDIR)
	$(Q)mkdir -p $(DATADIR)
	cp -r data $(DATADIR)/..
	cp $(BUILD)/$(TARGET_BIN) $(BINDIR)
	cp $(BUILD)/$(TARGET_LIB) $(LIBDIR)

# Do unit tests and code coverage
.PHONY: check
check:
	@echo "Compiling unit tests"
	$(Q)$(MAKE) -C tests check

# Create the documentation
.PHONY: doc
doc:
	$(Q)doxygen Doxyfile

# Create the tarball
.PHONY: tarball
tarball:
	$(Q)./.targz.sh $(PWD) $(TARGET_BIN)

# Delete compiled files
.PHONY: clean
clean:
	$(Q)-rm -fr $(BUILD)

# Delete compiled files and backup files
.PHONY: veryclean
veryclean: clean
	$(Q)-rm -fr *~ .*~
	$(Q)find src -name "*~" -print -delete
	$(Q)-rm -fr doc/html

# Create the directory before compiling sources
$(LIB_OBJS) $(OBJS): | $(BUILD)
$(BUILD):
	@mkdir -p $(BUILD)

# Create the dependency files
$(BUILD)/%.d: ;
.PRECIOUS: $(BUILD)/%.d

# Header file dependencies
-include $(patsubst %,$(BUILD)/%.d,$(basename $(LIB_OBJS) $(OBJS)))
