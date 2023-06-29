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
		
		ARGPARSE_CONFIG_CUSTOM_USAGE("Usage: example [OPTIONS]");
		ARGPARSE_CONFIG_HELP_SUFFIX("Example version 1.2.3");
		
		ARG('h', "help", "Show this help message") {
			ARGPARSE_HELP();
			return 0;
		}
		
		ARG_STRING('c', "command", "Command to execute", cmd) {
			system(cmd);
		}
	}
	
	return 0;
}
