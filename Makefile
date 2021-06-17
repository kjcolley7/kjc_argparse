# Targets to build and their sources
SMALL_TARGET := small_example
SMALL_SRCS := small_example.c

FULL_TARGET := full_example
FULL_SRCS := full_example.c

LIB_TARGET := kjc_argparse.o
LIB_SRCS := kjc_argparse.c


# Computed variables
TARGETS := $(SMALL_TARGET) $(FULL_TARGET) $(LIB_TARGET)

# Compiler choices
CC := clang
LD := clang

# Compiler flags to use
override CFLAGS += \
	-std=gnu99 \
	-D_GNU_SOURCE=1 \
	-Wall \
	-Wextra \
	-Werror \
	-Wno-unused-function \
	-I. \


override OFLAGS += -O2 -flto
override LDFLAGS +=
override STRIP_FLAGS += -Wl,-S -Wl,-x


# Directory where build products are stored
BUILD := .build

# Object files that need to be produced from sources
SMALL_OBJS := $(patsubst %,$(BUILD)/%.o,$(SMALL_SRCS))
FULL_OBJS := $(patsubst %,$(BUILD)/%.o,$(FULL_SRCS))
LIB_OBJS := $(patsubst %,$(BUILD)/%.o,$(LIB_SRCS))
ALL_OBJS := $(sort $(SMALL_OBJS) $(FULL_OBJS) $(LIB_OBJS))

# Dependency files that are produced during compilation
DEPS := $(ALL_OBJS:.o=.d)

# .dir files in every build directory
BUILD_DIR_FILES := $(BUILD)/.dir


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


# Linking rule for small_example
$(SMALL_TARGET): $(SMALL_OBJS) $(LIB_TARGET)
	@echo 'Linking $@'
	$(_v)$(LD) $(LDFLAGS) $(OFLAGS) $(STRIP_FLAGS) -o $@ $^

# Linking rule for full_example
$(FULL_TARGET): $(FULL_OBJS) $(LIB_TARGET)
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

test: $(FULL_TARGET) test.sh test_out.expected test_err.expected
	@echo 'Running test suite'
	$(_v)./test.sh

# Rules whose names don't correspond to files that should be built
.PHONY: all debug debug+ clean check test

# Disable stupid built-in rules
MAKEFLAGS += --no-builtin-rules
.SUFFIXES:
