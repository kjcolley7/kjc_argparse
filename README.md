kjc_argparse
=====

kjc_argparse lets you write powerful argument parsers in C without all the hassle of existing solutions. Argument parser code with kjc_argparse is extremely readable and doesn't require any boilerplate or manual initialization of structs. You don't even have to manually create any variables or call any functions! Also, this library will automatically generate, format, and display a --help message when you call `ARGPARSE_HELP()`!

### Motivation

I've written a number of command line programs in C, and I always ended up writing my own argument parser as the existing libraries for argument parsing leave plenty to be desired. Finally, I decided I had enough and I would go all out to create the C argument parser to rule them all. The idea behind kjc_argparse is to create a DSL (domain specific language) for argument parsing to simplify writing argument parsing code. This is achieved through extensive use of macros.

-----

### Advantages

* Easy to read
* Easy to write
* Help text is automatically generated
* No external dependencies, only uses minimal parts of libc
* Very portable, works with any C99+ compiler that supports `__COUNTER__` (MSVC/GCC/Clang all do)
* Lightweight, only uses a single heap allocation
* Fast
* Arguments can have values attached in multiple ways: `-p 2222`, `--port 2222`, `--port=2222`
* Supports multiple short options in a single argument like: `ls -laF`, `tar -tzf archive.tar.gz`

### Disadvantages

* No support (yet!) for subcommands (like `git clone` or `docker build`)
* kjc_argparse.h might not necessarily be the most readable code ever due to all of the macros. But hey, at least it's well commented!

### Example

For a full example with every feature in use and explained, see [full_example.c](full_example.c). Below is just a small example to give you a taste of what argument parsing with kjc_argparse is like (contents of [small_example.c](small_example.c)):

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
		ARG('h', "help", "Display this help message") {
			ARGPARSE_HELP();
			break;
		}
		
		ARG_STRING('u', "base-url", "Base URL for resouroces", url) {
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
		
		ARG('v', "verbose", "Enable verbose logging") {
			verbose = true;
		}
		
		ARG_POSITIONAL(arg, "input1.json {inputN.json...}") {
			if(json_count >= sizeof(json_inputs) / sizeof(json_inputs[0])) {
				printf("Too many JSON files!\n");
				ARGPARSE_HELP();
				break;
			}
			
			json_inputs[json_count++] = arg;
		}
		
		ARG_END() {
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
Usage: ./small_example [-hjuv] input1.json {inputN.json...}
Options:
    -h, --help      Display this help message
    -u, --base-url  [string] Base URL for resouroces
    -j, --jobs      [int] Number of jobs to run in parallel
    -v, --verbose   Enable verbose logging
```
