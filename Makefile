# Targets to build and their sources
EXAMPLE_SRCS := $(wildcard examples/*.c)
EXAMPLE_TARGETS := $(EXAMPLE_SRCS:.c=)

LIB_TARGET := kjc_argparse.o
LIB_SRCS := kjc_argparse.c


# Computed variables
TARGETS := $(LIB_TARGET) $(EXAMPLE_TARGETS)

# Compiler choices
CC := clang
LD := clang

# Compiler flags to use
override CFLAGS += \
	-std=c99 \
	-D_GNU_SOURCE=1 \
	-Wall \
	-Wextra \
	-Werror \
	-Wno-unused-function \
	-Wno-unused-variable \
	-Wno-unused-but-set-variable \
	-I. \


override OFLAGS += -O2 -flto
override LDFLAGS +=
override STRIP_FLAGS += -Wl,-S -Wl,-x


# Directory where build products are stored
BUILD := .build

# Object files that need to be produced from sources
EXAMPLE_OBJS := $(patsubst %,$(BUILD)/%.o,$(EXAMPLE_SRCS))
LIB_OBJS := $(patsubst %,$(BUILD)/%.o,$(LIB_SRCS))
ALL_OBJS := $(sort $(EXAMPLE_OBJS) $(LIB_OBJS))

# Dependency files that are produced during compilation
DEPS := $(ALL_OBJS:.o=.d)

# .dir files in every build directory
BUILD_DIR_FILES := $(addsuffix .dir,$(sort $(dir $(ALL_OBJS))))


# Print all commands executed when VERBOSE is defined
ifdef VERBOSE
_v :=
else #VERBOSE
_v := @
endif #VERBOSE


## Build rules

# Build the target by default
all: $(TARGETS)

# Build in debug mode (with asserts enabled)
debug: override CFLAGS += -ggdb -DDEBUG=1 -UNDEBUG
debug: override OFLAGS := -O0
debug: override STRIP_FLAGS :=
debug: $(TARGETS)

# Uses clang's Address Sanitizer to help detect memory errors
debug+: override CFLAGS += -fsanitize=address
debug+: override LDFLAGS += -fsanitize=address
debug+: debug


# Linking rule for single-file programs
$(EXAMPLE_TARGETS): %: $(BUILD)/%.c.o $(LIB_TARGET)
	@echo 'Linking $@'
	$(_v)$(LD) $(LDFLAGS) $(OFLAGS) $(STRIP_FLAGS) -o $@ $^

# Rule for lib target
$(LIB_TARGET): $(LIB_OBJS)
	@echo 'Producing $@'
	$(_v)cp $< $@

# Compiling rule for C sources
$(BUILD)/%.c.o: %.c | $(BUILD_DIR_FILES)
	@echo 'Compiling $<'
	$(_v)$(CC) $(CFLAGS) $(OFLAGS) -I$(<D) -MD -MP -MF $(BUILD)/$*.c.d -c -o $@ $<

# Preprocessing rule
%.pp: %
	@echo 'Preprocessing $<'
	$(_v)$(CC) -E $(CFLAGS) -I$(<D) -o $@ $<


# Build dependency rules
-include $(DEPS)

# Make sure that the .dir files aren't automatically deleted after building
.SECONDARY:

%/.dir:
	$(_v)mkdir -p $* && touch $@

clean:
	@echo 'Removing built products'
	$(_v)rm -rf $(BUILD) $(TARGETS)

# Used for debugging this Makefile
# `make CFLAGS?` will print the compiler flags used for compiling C code
%?:
	@echo '$* := $($*)'

check: test

test: $(EXAMPLE_TARGETS) test.sh examples/full_out.expected examples/full_err.expected
	@echo 'Running test suite'
	$(_v)./test.sh

# Rules whose names don't correspond to files that should be built
.PHONY: all debug debug+ clean check test

# Disable stupid built-in rules
MAKEFLAGS += --no-builtin-rules
.SUFFIXES:
