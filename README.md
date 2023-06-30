kjc_argparse
=====

kjc_argparse lets you write powerful argument parsers in C without all the hassle of existing solutions.
Argument parser code with kjc_argparse is extremely readable and doesn't require any boilerplate or manual
initialization of structs. You don't even have to manually create any variables or call any functions!
Also, this library will automatically generate, format, and display a --help message when you call
`ARGPARSE_HELP()`!


### Motivation

I've written a number of command line programs in C, and I always ended up writing my own argument parser
as the existing libraries for argument parsing leave plenty to be desired. Finally, I decided I had enough
and I would go all out to create the one C argument parser to rule them all. The idea behind kjc_argparse
is to create a DSL (domain specific language) for argument parsing to simplify writing argument parsing code.
This is achieved through extensive use of macros.

-----

### Advantages

* Easy to read
* Easy to write
* Help text is automatically generated and printed for `--help`
* No external dependencies, only uses minimal parts of libc
* Lightweight, only uses a single heap allocation
* Fast, using lookup bitmaps, binary searching, and a jump table for fast argument matching
* Very portable, works with any C99+ compiler that supports `__COUNTER__` (GCC/Clang/MSVC all do)
* Support for subcommands (like `git clone` or `docker build`)
* Arguments can have values attached in multiple ways: `-p 2222`, `--port 2222`, `--port=2222`
* Supports multiple short options in a single argument like: `ls -laF`, `tar -xzf archive.tar.gz`
* Built-in support for using `--` to treat all further arguments as positional (not options)
* Optional configuration parameters for tuning parsing behavior and formatting of the help message


### Disadvantages

* [kjc_argparse.h](kjc_argparse.h) might not necessarily be the most readable code ever due to all of the macros.
  But hey, at least it's well commented!


### Examples

For a full example with most features in use and explained, see [full_example.c](examples/full_example.c). For an
example of defining and using subcommands (mimicing `docker login`), see [subcmd_example.c](examples/subcmd_example.c).
Below is just a small example to give you a taste of what argument parsing with kjc_argparse is like
(contents of [small_example.c](examples/small_example.c)):

```c
#include <stdio.h>
#include <stdbool.h>
#include <stdlib.h>
#include "kjc_argparse.h"

int main(int argc, char** argv) {
	bool parse_success = false;
	const char* base_url = NULL;
	bool verbose = false;
	int job_count = 1;
	const char* json_inputs[10] = {0};
	size_t json_count = 0;
	
	ARGPARSE(argc, argv) {
		ARG('v', "verbose", "Enable verbose logging") {
			verbose = true;
		}
		
		ARG_STRING('u', "base-url", "Base URL for resources", url) {
			base_url = url;
		}
		
		ARG_INT('j', "jobs", "Number of jobs to run in parallel", jobs) {
			if(jobs < 1) {
				printf("Need at least one job!\n\n");
				ARGPARSE_HELP();
				break;
			}
			
			job_count = jobs;
		}
		
		ARG_POSITIONAL("input1.json {inputN.json...}", arg) {
			if(json_count >= sizeof(json_inputs) / sizeof(json_inputs[0])) {
				printf("Too many JSON files!\n");
				ARGPARSE_HELP();
				break;
			}
			
			json_inputs[json_count++] = arg;
		}
		
		ARG_END {
			if(!base_url) {
				printf("Missing required argument --base-url!\n\n");
				ARGPARSE_HELP();
				break;
			}
			
			parse_success = true;
		}
	}
	
	if(!parse_success) {
		exit(EXIT_FAILURE);
	}
	
	//TODO: rest of program
	
	return 0;
}
```

Running `./small_example --help` produces the following output:

```
Usage: small_example [-juv] input1.json {inputN.json...}
Options:
  -v, --verbose           Enable verbose logging
  -u, --base-url string   Base URL for resources
  -j, --jobs int          Number of jobs to run in parallel
```


### Configuration Parameters

If you want to change how kjc_argparse works in some way, there are a bunch of configuration parameters that
can be changed, at build time or dynamically at runtime. Here's the complete list of all currently supported
configuration parameters and a description of what they do:

* `ARGPARSE_CONFIG_STREAM(FILE* output_fp);` - Set output stream used for argparse messages (like `ARGPARSE_HELP()`).
  - **Default**: `stderr`
  - The `STREAM` parameter changes which `FILE*` stream kjc_argparse will write messages to. This includes
    `ARGPARSE_HELP()` as well as any error messages that will be printed if argument syntax errors are encountered.
    Note that if you have an `ARG_OTHER` handler defined, most internal error messages will not be printed. This can
    be set to `NULL` to disable ALL output from kjc_argparse, including `ARGPARSE_HELP()`.

* `ARGPARSE_CONFIG_CUSTOM_USAGE(const char* usage);` - Set custom usage text for help output.
  - **Default**: `NULL`
  - The `CUSTOM_USAGE` parameter allows you to replace the first line of help output with a custom string.

* `ARGPARSE_CONFIG_HELP_SUFFIX(const char* suffix);` - Set custom text to appear at the end of help output.
  - **Default**: `NULL`
  - The `HELP_SUFFIX` parameter allows adding a custom string to the end of the help output message.

* `ARGPARSE_CONFIG_COMMAND_DESCRIPTION_COLUMN(int column);` Set column at which command descriptions will be printed.
  - **Default**: `-1` (dynamically calculated)
  - The `COMMAND_DESCRIPTION_COLUMN` parameter sets the column at which description messages for subcommands will be
    printed in help output. Normally, the column for command description printing is calculated based on the width
    needed to print the longest command name with `DESCRIPTION_PADDING` spaces after that.

* `ARGPARSE_CONFIG_DESCRIPTION_COLUMN(int column);` - Set column at which arg descriptions will be printed.
  - **Default**: `-1` (dynamically calculated)
  - The `DESCRIPTION_COLUMN` parameter sets the column at which description messages for options will be printed in
    help output. Normally, the column for option description printing is calculated based on the width needed to print
    the longest argument (including any type or variable name hints) and then `DESCRIPTION_PADDING` spaces after that.

* `ARGPARSE_CONFIG_INDENT(int column);` - Set column at which commands and options begin printing.
  - **Default**: `2`
  - The `INDENT` parameter sets the column at which subcommands and options start printing.

* `ARGPARSE_CONFIG_USE_VARNAMES(bool val);` - Set whether variable names (or type names) should be used in help text.
  - **Default**: `true`
  - The `USE_VARNAMES` parameter controls whether the variable names you use for arguments which have values should be
    used in the formatted help message. If this is disabled, the type name (`string` or `int`) will be used instead.
  - **Example**: With something like `ARG_INT('s', "sport", "Source port number", port)`, the help text would be:
    - `true  |  -s, --sport=port   Source port number`
    - `false |  -s, --sport=int   Source port number`

* `ARGPARSE_CONFIG_TYPE_HINTS(bool val);` - Set whether type hints are printed in option descriptions.
  - **Default**: `false`
  - The `TYPE_HINTS` parameter controls whether option type hints will be printed at the beginning of the description.
  - **Example**: With something like `ARG_INT('s', "sport", "Source port number", port)`, the help text would be:
    - `true  |  -s, --sport=port   [int] Source port number`
    - `false |  -s, --sport=port   Source port number`

* `ARGPARSE_CONFIG_DESCRIPTION_PADDING(int padding);` - Set minimum number of spaces before options' descriptions.
  - **Default**: `3`
  - The `DESCRIPTION_PADDING` parameter controls how many spaces (at minimum) are printed before the description text
    of a subcommand or option.

* `ARGPARSE_CONFIG_SHORTGROUPS(bool enable);` - True to enable support for multiple short options in a single argument.
  - **Default**: `true`
  - The `SHORTGROUPS` parameter controls whether support for groups of multiple short options in a single argument is
    enabled. This is for arguments like `-xzf` within `tar -xzf archive.tar.gz`, `-planetu` in `netstat -planetu`, or
    `-laF` in `ls -laF`. If this parameter is disabled, arguments like these will go to the `ARG_OTHER` handler.

* `ARGPARSE_CONFIG_AUTO_HELP(bool enable);` - True to automatically support `--help`.
  - **Default**: `true`
  - The `AUTO_HELP` parameter can be set to `false` to disable automatic support for the `--help` option. The help
    message will be automatically printed when an _unhandled_ `--help` argument is encountered. It's the equivalent of
    defining this:

```c
ARG(0, "--help", NULL) {
	ARGPARSE_HELP();
	break;
}
```

* `ARGPARSE_CONFIG_DASHDASH(bool enable);` - True to treat everything after `--` as `ARG_POSITIONAL`.
  - **Default**: `true`
  - The `DASHDASH` parameter controls whether a `--` argument stops option parsing and instead treats every argument
    after that as an `ARG_POSITIONAL` argument (even if it starts with a `-`). This is useful to refer to files that
    start with a `-`, for example. It's also useful for accepting a command that should be run, as without `--`, there
    would be confusion about whether the arguments at the end are for this program or the one it's supposed to execute.

* `ARGPARSE_CONFIG_DEBUG(bool debug);` - Print internal argparse debug information.
  - **Default**: `false`
  - The `DEBUG` parameter enables debug printing of kjc_argparse's internal data structures and state machine
    transitions during argument parsing. It is a useful debugging aid (for me while working on kjc_argparse), and it
    can help the programmer with understanding how kjc_argparse works. To compile out support for this parameter,
    resulting in a smaller compiled size, define the `NDEBUG` macro when building `kjc_argparse.c`.

These configuration parameters should be placed directly inside the `ARGPARSE` block, like this:

```c
#include <stdio.h>
#include <stdbool.h>
#include <stdlib.h>
#include "kjc_argparse.h"

int main(int argc, char** argv) {
	ARGPARSE(argc, argv) {
		ARGPARSE_CONFIG_STREAM(stdout);
		ARGPARSE_CONFIG_INDENT(4);
		ARGPARSE_CONFIG_DESCRIPTION_PADDING(5);
		ARGPARSE_CONFIG_USE_VARNAMES(false);
		ARGPARSE_CONFIG_AUTO_HELP(false);
		
		ARGPARSE_CONFIG_CUSTOM_USAGE("Usage: example [OPTIONS]");
		ARGPARSE_CONFIG_HELP_SUFFIX("Example version 1.2.3");
		
		ARG(0, "usage", "Print usage information") {
			ARGPARSE_HELP();
			return 0;
		}
		
		ARG_STRING('c', "command", "Command to execute", cmd) {
			system(cmd);
		}
	}
	
	return 0;
}
```

`./config_example --usage` produces this output (written to `stdout`):

```
Usage: example [OPTIONS]

Options:
        --usage           Print usage information
    -c, --command cmd     Command to execute

Example version 1.2.3
```
