#
# Copyright (c) 2022 Craig Watt
#
# Contact: craig@webrefine.co.uk
#
# Permission is hereby granted, free of charge, to any person obtaining a copy
# of this software and associated documentation files (the "Software"), to deal
# in the Software without restriction, including without limitation the rights
# to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
# copies of the Software, and to permit persons to whom the Software is
# furnished to do so, subject to the following conditions:
#
# The above copyright notice and this permission notice shall be included in
# all copies or substantial portions of the Software.
#
# THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
# IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
# FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL
# THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
# LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
# OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
# THE SOFTWARE.
#

# Includes the project configurations
include config.mk

#
# Validating project variables defined in project.conf
#
ifndef PROJECT_NAME
$(error Missing PROJECT_NAME. Put variables at config.mk file)
endif
ifndef BINARY
$(error Missing BINARY. Put variables at config.mk file)
endif
ifndef PROJECT_PATH
$(error Missing PROJECT_PATH. Put variables at config.mk file)
endif

# Gets the Operating system name
OS := $(shell uname -s)

# Default shell
SHELL := zsh

# Color prefix for Linux distributions
COLOR_PREFIX := e

ifeq ($(OS),Darwin)
	COLOR_PREFIX := 033
endif

# Color definition for print purpose
BROWN=\$(COLOR_PREFIX)[0;33m
BLUE=\$(COLOR_PREFIX)[1;34m
END_COLOR=\$(COLOR_PREFIX)[0m

# Source code directory structure
BINDIR := bin
SRCDIR := src
LOGDIR := log
LIBDIR := lib
TESTDIR := test
TESTLIBDIR := test/lib

# Source code file extension
SRCEXT := c

# Defines the C Compiler
CC := gcc

# Defines the language standards for GCC
STD := -std=gnu99 # See man gcc for more options

# Protection for stack-smashing attack
STACK := -fstack-protector-all -Wstack-protector

# Specifies to GCC the required warnings
WARNS := -Wall -Wextra -pedantic # -pedantic warns on language standards

# Flags for compiling
CFLAGS := -O3 $(STD) $(STACK) $(WARNS)

# Debug options
DEBUG := -g3 -DDEBUG=1

# Dependency libraries
LIBS := -I /opt/homebrew/include

# Test libraries
TEST_LIBS := -l cmocka -L /opt/homebrew/lib

# Tests binary file
TEST_BINARY := $(BINARY)_test_runner

TESTING_MACRO := -D TESTING

# %.o file names
NAMES := $(notdir $(basename $(wildcard $(SRCDIR)/*/*.$(SRCEXT)))) $(notdir $(basename $(wildcard $(SRCDIR)/*/*/*.$(SRCEXT))))
OBJECTS := $(patsubst %,$(LIBDIR)/%.o,$(NAMES))

TEST_NAMES := $(notdir $(basename $(wildcard $(TESTDIR)/*.$(SRCEXT)))) $(notdir $(basename $(wildcard $(TESTDIR)/*/*.$(SRCEXT)))) $(notdir $(basename $(wildcard $(SRCDIR)/*/*.$(SRCEXT)))) $(notdir $(basename $(wildcard $(SRCDIR)/*/*/*.$(SRCEXT))))
TEST_OBJECTS := $(patsubst %, $(TESTLIBDIR)/%.o,$(TEST_NAMES))

#
# COMPILATION RULES
#

default: all

# Rule for link and generate the binary file
all: $(OBJECTS)
	@echo -en "$(BROWN)LD $(END_COLOR)";
	$(CC) -o $(BINDIR)/$(BINARY) $+ $(DEBUG) $(CFLAGS) $(LIBS)
	@echo -en "\n--\nBinary file placed at" \
			  "$(BROWN)$(BINDIR)/$(BINARY)$(END_COLOR)\n";

# Rule for object binaries compilation
$(LIBDIR)/%.o: $(SRCDIR)/*/%.$(SRCEXT) 
	@echo -en "$(BROWN)CC $(END_COLOR)";
	$(CC) -c $^ -o $@ $(DEBUG) $(CFLAGS) $(LIBS)

$(LIBDIR)/%.o: $(SRCDIR)/*/*/%.$(SRCEXT) 
	@echo -en "$(BROWN)CC $(END_COLOR)";
	$(CC) -c $^ -o $@ $(DEBUG) $(CFLAGS) $(LIBS)

#just show name variable
show_names: 
	@echo $(NAMES);
	@echo $(TEST_NAMES);

show_objects:
	@echo $(OBJECTS);
	@echo $(TEST_OBJECTS);

# Rule for run valgrind tool
valgrind:
	valgrind \
		--track-origins=yes \
		--leak-check=full \
		--leak-resolution=high \
		--log-file=$(LOGDIR)/$@.log \
		$(BINDIR)/$(BINARY)
	@echo -en "\nCheck the log file: $(LOGDIR)/$@.log\n"

# Compile tests and run the test binary
# Rule for link and generate the binary file
tests: $(TEST_OBJECTS)
	@echo -en "$(BROWN)CC $(END_COLOR)";
	$(CC) -o $(BINDIR)/$(TEST_BINARY) $+ $(DEBUG) $(CFLAGS) $(LIBS) $(TEST_LIBS) $(TESTING_MACRO)
	@which ldconfig && ldconfig -C /tmp/ld.so.cache || true # caching the library linking
	@echo -en "$(BROWN) Running tests: $(END_COLOR)";
	./$(BINDIR)/$(TEST_BINARY)

$(TESTLIBDIR)/%.o: $(SRCDIR)/*/%.$(SRCEXT) 
	@echo -en "$(BROWN)CC $(END_COLOR)";
	$(CC) -c $^ -o $@ $(DEBUG) $(CFLAGS) $(LIBS) $(TESTING_MACRO)

$(TESTLIBDIR)/%.o: $(SRCDIR)/*/*/%.$(SRCEXT) 
	@echo -en "$(BROWN)CC $(END_COLOR)";
	$(CC) -c $^ -o $@ $(DEBUG) $(CFLAGS) $(LIBS) $(TESTING_MACRO)

# Rule for object binaries compilation
$(TESTLIBDIR)/%.o: $(TESTDIR)/%.$(SRCEXT) 
	@echo -en "$(BROWN)CC $(END_COLOR)";
	$(CC) -c $^ -o $@ $(DEBUG) $(CFLAGS) $(LIBS) $(TESTING_MACRO)

$(TESTLIBDIR)/%.o: $(TESTDIR)/*/%.$(SRCEXT) 
	@echo -en "$(BROWN)CC $(END_COLOR)";
	$(CC) -c $^ -o $@ $(DEBUG) $(CFLAGS) $(LIBS) $(TESTING_MACRO)

# Rule for cleaning the project
clean:
	@rm -rvf $(BINDIR)/* $(LIBDIR)/*;

clean_tests:
	@rm -rvf $(BINDIR)/* $(TESTLIBDIR)/*;

install: $(OBJECTS)
	sudo mkdir -p $(DESTDIR)$(PREFIX)/bin
	sudo mkdir -p $(DESTDIR)$(PREFIX)/bin/vfo_conf_folder
	sudo cp -f bin/vfo $(DESTDIR)$(PREFIX)/bin
	sudo cp -f src/vfo_config.conf $(DESTDIR)$(PREFIX)/bin/vfo_conf_folder
	

uninstall:
	sudo rm -f $(DESTDIR)$(PREFIX)/bin/vfo
	sudo rm -f $(DESTDIR)$(PREFIX)/bin/vfo_conf_folder/vfo_config.conf
	sudo rmdir $(DESTDIR)$(PREFIX)/bin/vfo_conf_folder
