#include <stdio.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdlib.h>

#include "kjc_argparse.h"

/*
$ ./example --help
Usage: ./example [-fHhiost] [...]
Options:
    -h, --help                Display this help message
        --hello               Say hello!
    -H                        Hello but in caps
    -t, --test                This is a test lol
    -f, --flag                Turns this flag on
    -o, --once                This flag may only be set once
    -i, --my-int-argument     [int] This argument expects an integer value
    -s, --my-string-argument  [string] This argument expects a string value
*/

int main(int argc, char** argv) {
	bool flag = false, once = false;
	int ret = EXIT_FAILURE;
	
	// Start parsing arguments in the ARGPARSE() loop, passing the argc and argv from main()
	ARGPARSE(argc, argv) {
		// In the body of the ARGPARSE() loop, there should be one or more argument handlers.
		// It is not safe to put code in the ARGPARSE() body outside of an argument handler.
		
		// This is run for both -h and --help
		ARG('h', "help", "Display this help message") {
			// A help message is automatically generated, formatted nicely, and printed when you use ARGPARSE_HELP()
			ARGPARSE_HELP();
			break; //Stop parsing arguments, this will break directly out of the ARGPARSE() loop, skipping ARG_END
		}
		
		// Both the short option and long option are optional, though at least one must be given
		
		// No short argument, only --hello
		ARG(0, "hello", "Say hello!") {
			printf("Hello!\n");
		}
		
		// No long argument, only -H
		ARG('H', NULL, "Hello but in caps") {
			printf("HELLO\n");
		}
		
		// You can run whatever code you want in an argument handler
		ARG('t', "test", "This is a test lol") {
			printf("Test:");
			for(int i = 1; i <= 10; i++) {
				printf(" %d", i);
			}
			printf("\n");
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
		
		// Use this with -i 42 or --my-int-argument 1337. This supports negative integers and also other bases
		// such as hex (when prefixed with 0x) and octal (when prefixed with a 0), as this internally uses strtol().
		ARG_INT('i', "my-int-argument", "This argument expects an integer value", myInt) {
			// The final parameter to ARG_INT() is the name of the variable of type int that holds
			// the arguments' matching value, parsed as an integer
			printf("--my-int-argument %d\n", myInt);
		}
		
		// Use this with one of the following:
		//
		// -s example_lol
		// --my-string-argument "this is a single string argument"
		// --my-string-argument=inline_value
		ARG_STRING('s', "my-string-argument", "This argument expects a string value", myString) {
			// The final parameter to ARG_STRING() is the name of the variable of type const char*
			// that holds the argument's matching string value
			printf("--my-string-argument %s\n", myString);
		}
		
		// You can optionally include an ARG_OTHER() handler, which is called whenever an argument doesn't match
		// any of the registered handlers. If you do not include an ARG_OTHER() handler, the following message will
		// be printed to stdout followed by argument parsing stopping:
		//
		// Unexpected argument: "%s"
		ARG_OTHER(arg) {
			// The argument to ARG_OTHER is the name of a variable that will be created as type const char*
			// which holds the current argument
			printf("ARG_OTHER: %s\n", arg);
			
			// You can also get the index of the current argument in the argv array with ARGPARSE_INDEX() */
			printf("Index: %d\n", ARGPARSE_INDEX());
			
			ret = -1;
			break;
		}
		
		// You can optionally include an ARG_END handler which runs after all arguments from argv have been parsed
		// without errors or breaking early.
		ARG_END() {
			printf("All done with argument parsing!\n");
			
			// This is a good place to perform final argument parsing validation such as ensuring that required
			// arguments were given. It is still possible to use ARGPARSE_HELP() here, while it is not possible
			// to use that outside of the ARGPARSE() loop.
			if(!flag) {
				printf("ERROR --flag is required!\n");
				ARGPARSE_HELP();
				exit(EXIT_FAILURE);
			}
			
			// Argument parsing succeeded, so set successful exit code
			ret = 0;
		}
	}
	
	// No longer possible to use ARGPARSE_HELP() here, as we're outside of the scope of the ARGPARSE() loop
	return ret;
}
