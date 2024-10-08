OBJS_DIR = .objs

# the really challenging part in this makefile is tying all the components
# together

# assume all mappers have the same deps, this makes it way easier to build
# mappername.o is added to each of the deps when the target is invoked
MAPPERS_SRCS=$(wildcard mappers/*.c)
MAPPERS=$(MAPPERS_SRCS:mappers/%.c=mapper_%)
MAPPERS_DEPS=core/mapper.o

# same deal for reducers
REDUCERS_SRCS=$(wildcard reducers/*.c)
REDUCERS=$(REDUCERS_SRCS:reducers/%.c=reducer_%)
REDUCERS_DEPS=core/reducer.o core/libds.o core/utils.o

# pi is a little different
# I've just hardcoded those targets in this file

# all of the connector tools
# again, I'm asserting that all of these have the same deps
TOOLS=mapreduce splitter
TOOLS_DEPS=core/utils.o

# set up compiler
CC = clang
WARNINGS = -Wall -Wextra -Werror -Wno-error=unused-parameter
INC=-Icore/
CFLAGS_COMMON = $(WARNINGS) $(INC) -std=c99 -c -MMD -MP -D_GNU_SOURCE
CFLAGS_RELEASE = $(CFLAGS_COMMON) -O2
CFLAGS_DEBUG = $(CFLAGS_COMMON) -O0 -g -DDEBUG

# pi stuff needs a c++ compiler
# we never build pi in debug mode
CXX = clang++
CXXFLAGS=-O3 -Wall -Icore/

# set up linker
LD = clang
LDFLAGS = -lm

# the string in grep must appear in the hostname, otherwise the Makefile will
# not allow the assignment to compile
IS_VM=$(shell hostname | grep "cs341")
VM_OVERRIDE=$(shell echo $$HOSTNAME)
ifeq ($(IS_VM),)
ifneq ($(VM_OVERRIDE),cs241grader)
$(error This assignment must be compiled on the CS341 VMs)
endif
endif

.PHONY: all
all: release

# build types
# run clean before building debug so that all of the release executables
# disappear
.PHONY: debug
.PHONY: release

release: mappers-release reducers-release pi tools-release
debug:   clean mappers-debug reducers-debug pi tools-debug

.PHONY: mappers-relase
mappers-release: $(MAPPERS)

.PHONY: mappers-debug
mappers-debug : $(MAPPERS:%=%-debug)

.PHONY: reducers-relase
reducers-release: $(REDUCERS)

.PHONY: reducers-debug
reducers-debug : $(REDUCERS:%=%-debug)

.PHONY: tools-release
tools-release: $(TOOLS)

.PHONY: tools-debug
tools-debug: $(TOOLS:%=%-debug)

.PHONY: pi
pi: mapper_pi reducer_pi

.PHONY: data
data: data/alice.txt data/dracula.txt

data/alice.txt:
		curl -L "https://www.gutenberg.org/files/11/11-0.txt" > data/alice.txt

data/dracula.txt:
		curl -L "http://www.gutenberg.org/cache/epub/345/pg345.txt" > data/dracula.txt

# include dependencies
ROOT_DIR:=$(shell dirname $(realpath $(lastword $(MAKEFILE_LIST))))
-include $(shell find $(ROOT_DIR)/$(OBJS_DIR) -name '*.d')

$(OBJS_DIR):
	@mkdir -p $(OBJS_DIR)

# patterns to create objects
# keep the debug and release postfix for object files so that we can always
# separate them correctly
$(OBJS_DIR)/%-debug.o: %.c | $(OBJS_DIR)
	@mkdir -p $(basename $@)
	$(CC) $(CFLAGS_DEBUG) $< -o $@

$(OBJS_DIR)/%-release.o: %.c | $(OBJS_DIR)
	@mkdir -p $(basename $@)
	$(CC) $(CFLAGS_RELEASE) $< -o $@

# executables
$(MAPPERS): mapper_% : $(MAPPERS_DEPS:%.o=$(OBJS_DIR)/%-release.o) $(OBJS_DIR)/mappers/%-release.o
	$(LD) $^ $(LDFLAGS) -o $@

$(MAPPERS:%=%-debug): mapper_%-debug : $(MAPPERS_DEPS:%.o=$(OBJS_DIR)/%-debug.o) $(OBJS_DIR)/mappers/%-debug.o
	$(LD) $^ $(LDFLAGS) -o $@

$(REDUCERS): reducer_% : $(REDUCERS_DEPS:%.o=$(OBJS_DIR)/%-release.o) $(OBJS_DIR)/reducers/%-release.o
	$(LD) $^ $(LDFLAGS) -o $@

$(REDUCERS:%=%-debug): reducer_%-debug : $(REDUCERS_DEPS:%.o=$(OBJS_DIR)/%-debug.o) $(OBJS_DIR)/reducers/%-debug.o
	$(LD) $^ $(LDFLAGS) -o $@

$(TOOLS): % : $(TOOLS_DEPS:%.o=$(OBJS_DIR)/%-release.o) $(OBJS_DIR)/%-release.o
	$(LD) $^ $(LDFLAGS) -o $@

$(TOOLS:%=%-debug): %-debug : $(TOOLS_DEPS:%.o=$(OBJS_DIR)/%-debug.o) $(OBJS_DIR)/%-debug.o
	$(LD) $^ $(LDFLAGS) -o $@

# pi stuff
mapper_pi: $(OBJS_DIR)/core/mapper-release.o pi/mapper_pi.cpp
	$(CXX) $(CXXFLAGS) $^ -o $@

reducer_pi: $(OBJS_DIR)/core/reducer-release.o $(OBJS_DIR)/core/libds-release.o pi/reducer_pi.cpp $(OBJS_DIR)/core/utils-release.o
	$(CXX) $(CXXFLAGS) $^ -o $@

.PHONY: clean
clean:
	-rm -rf $(MAPPERS) $(MAPPERS:%=%-debug)
	-rm -rf $(REDUCERS) $(REDUCERS:%=%-debug)
	-rm -rf mapper_pi reducer_pi
	-rm -rf $(TOOLS) $(TOOLS:%=%-debug)
	-rm -rf .objs $(EXES_STUDENT)
