#include <stdio.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdlib.h>
#include <string.h>

#include "kjc_argparse.h"

/*
$ ./full_example --help
Usage: full_example [-Hfilnot] [OPTIONS] [extra args...]

Options:
    -t, --test                     This is a test lol
        --hello                    Say hello!
    -H                             Hello but in caps
    -f, --flag                     Turns this flag on
    -o, --once                     This flag may only be set once
    -i, --int-argument <NUMBER>    [int] This argument expects an integer value
    -n, --set-name <NAME>          [string] This argument expects a string value
    -l, --long-like-really-extremely-long-argument
                                   This argument is really long
*/

int main(int argc, char** argv) {
	bool flag = false, once = false;
	int ret = EXIT_FAILURE;
	
	// Start parsing arguments in the ARGPARSE() block, passing the argc and argv from main()
	ARGPARSE(argc, argv) {
		// In the ARGPARSE() block, there should be one or more argument handlers.
		// It is not safe to put normal code in the ARGPARSE() block outside of an argument handler.
		
		// There are a few configuration options that can be put directly in the ARGPARSE() block:
		
		// To use the variable names from your argument handlers rather than type names in help text.
		// Example:
		//
		// ARG_INT(0, "sport", "Source port number", PORT) {...}
		//
		// Usage information when use_varnames=false:
		//   --sport=int
		//
		// Usage information when use_varnames=true:
		//   --sport=PORT
		ARGPARSE_CONFIG_USE_VARNAMES(true);
		
		// Enable type hints like "[string]" in option descriptions
		ARGPARSE_CONFIG_TYPE_HINTS(true);
		
		// If you have very long option names and want to pick a reasonable column for option
		// descriptions to begin being displayed from
		ARGPARSE_CONFIG_DESCRIPTION_COLUMN(35);
		
		// For more spacious help layout
		ARGPARSE_CONFIG_INDENT(4);
		
		// To change where argparse messages (like ARGPARSE_HELP()) are written (default is stderr)
		//ARGPARSE_CONFIG_STREAM(stdout);
		
		// To help understand how argparse works internally
		ARGPARSE_CONFIG_DEBUG(getenv("ARGPARSE_DEBUG") != NULL);
		
		
		// This is run for --usage. Setting the description string to NULL hides this option
		// from the generated help message. Note that --help is automatically handled unless
		// you set ARGPARSE_CONFIG_AUTO_HELP(false).
		ARG(0, "usage", NULL) {
			// A help message is automatically generated, formatted nicely, and printed when you use ARGPARSE_HELP()
			ARGPARSE_HELP();
			break; //Stop parsing arguments. This will break directly out of the ARGPARSE() block, skipping ARG_END
		}
		
		// Both the short option and long option are optional, though at least one must be given
		
		// You can run whatever code you want in an argument handler
		ARG('t', "test", "This is a test lol") {
			printf("Test:");
			for(int i = 1; i <= 10; i++) {
				printf(" %d", i);
			}
			printf("\n");
		}
		
		// No short argument, only --hello
		ARG(0, "hello", "Say hello!") {
			printf("Hello!\n");
		}
		
		// No long argument, only -H
		ARG('H', NULL, "Hello but in caps") {
			printf("HELLO\n");
		}
		
		// A common use case is to simply turn on a boolean flag when you see an argument
		ARG('f', "flag", "Turns this flag on") {
			flag = true;
		}
		
		// You can also have validation logic such as ensuring you can't pass the same flag more than once
		ARG('o', "once", "This flag may only be set once") {
			if(once) {
				printf("Flag --once given multiple times!\n");
				
				// Not safe to return from a ARG handler (will leak memory)
				ret = -1;
				break;
			}
			once = true;
		}
		
		// Arguments can also require values after them as the next argument.
		// Supported types: int (ARG_INT), string (ARG_STRING)
		
		// Use this with -i 42 or --int-argument 1337. This supports negative integers and also other bases
		// such as hex (when prefixed with 0x) and octal (when prefixed with a 0), as this internally uses strtol().
		ARG_INT('i', "int-argument", "This argument expects an integer value", NUMBER) {
			// The final parameter to ARG_INT() is the name of the variable of type int that holds
			// the arguments' matching value, parsed as an integer
			
			printf("--int-argument %d\n", NUMBER);
		}
		
		// Use this with one of the following:
		//
		// -n example_lol
		// --set-name "this is a single string argument"
		// --set-name=inline_value
		ARG_STRING('n', "set-name", "This argument expects a string value", NAME) {
			// The final parameter to ARG_STRING() is the name of the variable of type const char*
			// that holds the argument's matching string value
			printf("--set-name %s\n", NAME);
			
			// The break and continue keywords both work as you'd expect.
			if(NAME[0] != '@') {
				continue;
			}
			
			if(strcmp(NAME, "@ADMIN") == 0) {
				printf("Error: Illegal to set name to @ADMIN!\n");
				break;
			}
		}
		
		// Long argument name to show how the description will be printed on the next line
		ARG('l', "long-like-really-extremely-long-argument", "This argument is really long") {}
		
		// You can optionally define an ARG_POSITIONAL() handler, which is called whenever a non-option
		// argument (one that doesn't start with '-') is encountered.
		ARG_POSITIONAL("[extra args...]", arg) {
			// The argument to ARG_POSITIONAL() is the name of a variable that will be created
			// as type const char* which holds the current argument
			printf("ARG_POSITIONAL: %s\n", arg);
		}
		
		// You can optionally define an ARG_OTHER() handler, which is called whenever an arg doesn't match
		// any of the registered handlers. If you do not include an ARG_OTHER() handler, the following message
		// will be printed to the output stream, followed by argument parsing terminating:
		//
		// Unexpected argument: "%s"
		ARG_OTHER(arg) {
			printf("ARG_OTHER: %s\n", arg);
			
			// You can also get the index of the current argument in the argv array with ARGPARSE_INDEX() */
			printf("Index: %d\n", ARGPARSE_INDEX());
			ret = -1;
			break;
		}
		
		// You can optionally include an ARG_END handler which runs after all arguments from argv have been parsed
		// without errors or breaking early.
		ARG_END {
			printf("All done with argument parsing!\n");
			
			// This is a good place to perform final argument parsing validation such as ensuring that required
			// arguments were given. It is still possible to use ARGPARSE_HELP() here, while it is not possible
			// to use that outside of the ARGPARSE() block.
			if(!flag) {
				printf("ERROR: --flag is required!\n");
				ARGPARSE_HELP();
				exit(EXIT_FAILURE);
			}
			
			// Argument parsing succeeded, so set successful exit code
			ret = 0;
		}
	}
	
	// No longer possible to use ARGPARSE_HELP() here, as we're outside of the scope of the ARGPARSE() block
	return ret;
}
