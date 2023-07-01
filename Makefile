# Targets to build and their sources
EXAMPLE_SRCS := $(wildcard examples/*.c)
EXAMPLE_TARGETS := $(EXAMPLE_SRCS:.c=)

LIB_SRCS := kjc_argparse.c
LIB_HEADERS := kjc_argparse.h
LIB_STATIC := libkjc_argparse.a
LIB_SHARED := libkjc_argparse.so

# Computed variables
TARGETS := $(LIB_STATIC) $(LIB_SHARED) $(EXAMPLE_TARGETS)

# Default tool choices, if not specified as env vars or make args
CC ?= clang
LD ?= clang
AR ?= ar
INSTALL ?= install

# Compiler flags to use
override CFLAGS += \
	-std=c99 \
	-Wall \
	-Wextra \
	-Werror \
	-Wno-unused-function \
	-Wno-unused-variable \
	-Wno-unused-but-set-variable \
	-I. \

override OFLAGS += -O2
override LDFLAGS +=
override STRIP_FLAGS += -Wl,-S,-x

CC_LTO := -flto
LD_LTO := -flto

# Clang doesn't support -ffat-lto-objects yet:
# https://discourse.llvm.org/t/rfc-ffat-lto-objects-support/63977
CC_IS_CLANG := $(shell $(CC) --version | grep clang && echo 1)
ifndef CC_IS_CLANG
CC_LTO := $(CC_LTO) -ffat-lto-objects
endif

# Directory where build products are stored
BUILD := .build

# Object files that need to be produced from sources
LIB_OBJS := $(patsubst %,$(BUILD)/%.o,$(LIB_SRCS))
EXAMPLE_OBJS := $(patsubst %,$(BUILD)/%.o,$(EXAMPLE_SRCS))
ALL_OBJS := $(sort $(LIB_OBJS) $(EXAMPLE_OBJS))

# Dependency files that are produced during compilation
DEPS := $(ALL_OBJS:.o=.d)

# .dir files in every build directory
BUILD_DIR_FILES := $(addsuffix .dir,$(sort $(dir $(ALL_OBJS))))

define HASH
#
endef

# Print all commands executed when VERBOSE is defined
ifdef VERBOSE
_v :=
_V := @$(HASH)
else #VERBOSE
_v := @
_V := @
endif #VERBOSE


## Build rules

# Build the target by default
all: $(TARGETS)

# Build the library only
libs: staticlib dynamiclib

# Build only the static library archive
staticlib: $(LIB_STATIC)

# Build only the shared library
dynamiclib: $(LIB_SHARED)

# Build in debug mode (with asserts enabled)
debug: override CFLAGS += -ggdb -DDEBUG=1 -UNDEBUG
debug: override OFLAGS := -O0
debug: override STRIP_FLAGS :=
debug: override CC_LTO :=
debug: override LD_LTO :=
debug: $(TARGETS)

# Uses clang's Address Sanitizer to help detect memory errors
debug+: override CFLAGS += -fsanitize=address
debug+: override LDFLAGS += -fsanitize=address
debug+: debug

# Compiling rule for C sources
$(BUILD)/%.c.o: %.c | $(BUILD_DIR_FILES)
	$(_V)echo 'Compiling $<'
	$(_v)$(CC) $(CFLAGS) $(OFLAGS) $(CC_LTO) -I$(<D) -MD -MP -MF $(BUILD)/$*.c.d -c -o $@ $<

# Compiling dependency rules
-include $(DEPS)

# Linking rule for single-file programs
$(EXAMPLE_TARGETS): %: $(BUILD)/%.c.o $(LIB_STATIC)
	$(_V)echo 'Linking $@'
	$(_v)$(LD) $(LDFLAGS) $(OFLAGS) $(LD_LTO) $(STRIP_FLAGS) -o $@ $^

# Rule for static library archive
$(LIB_STATIC): $(LIB_OBJS)
	$(_V)echo 'Archiving $@'
	$(_v)$(AR) rcs $@ $^

$(LIB_SHARED): $(LIB_OBJS)
	$(_V)echo 'Linking $@'
	$(_v)$(LD) $(LDFLAGS) $(OFLAGS) $(LD_LTO) -shared -o $@ $^

# Preprocessing rule (for debugging purposes only)
%.pp: %
	$(_V)echo 'Preprocessing $<'
	$(_v)$(CC) -E $(CFLAGS) -I$(<D) -o $@ $<


# Common directories for installation paths
ifndef DESTDIR
DESTDIR :=
endif
prefix ?= /usr/local
exec_prefix ?= $(prefix)
includedir ?= $(prefix)/include
libdir ?= $(exec_prefix)/lib

install: libs
	$(_V)echo 'Installing under $(DESTDIR)$(prefix)...'
	$(_v)$(INSTALL) -d $(DESTDIR)$(includedir) $(DESTDIR)$(libdir) \
		&& $(INSTALL) -m 0644 $(LIB_HEADERS) $(DESTDIR)$(includedir)/ \
		&& $(INSTALL) -m 0644 $(LIB_STATIC) $(DESTDIR)$(libdir)/ \
		&& $(INSTALL) -m 0755 $(LIB_SHARED) $(DESTDIR)$(libdir)/

# Make sure that the .dir files aren't automatically deleted after building
.SECONDARY:

%/.dir:
	$(_v)mkdir -p $* && touch $@

clean:
	$(_V)echo 'Removing built products'
	$(_v)rm -rf $(BUILD) $(TARGETS)

# Used for debugging this Makefile
# `make CFLAGS?` will print the compiler flags used for compiling C code
%?:
	@echo '$* := $($*)'

check: test

test: $(EXAMPLE_TARGETS) test.sh examples/full_out.expected examples/full_err.expected
	$(_V)echo 'Running test suite'
	$(_v)./test.sh

# Rules whose names don't correspond to files that should be built
.PHONY: all debug debug+ libs staticlib dynamiclib install clean check test

# Disable stupid built-in rules
MAKEFLAGS += --no-builtin-rules
.SUFFIXES:
