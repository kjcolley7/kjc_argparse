#include <stdio.h>
#include <stdbool.h>
#include <stdlib.h>
#include "kjc_argparse.h"

int main(int argc, char** argv) {
	bool parse_success = false;
	const char* input_filename = NULL;
	bool verbose = false;
	int job_count = 1;
	
	ARGPARSE(argc, argv) {
		ARG('h', "help", "Display this help message") {
			ARGPARSE_HELP();
			break;
		}
		
		ARG_STRING('i', "input-file", "Input file", fname) {
			input_filename = fname;
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
		
		ARG_OTHER(arg) {
			printf("Invalid argument: %s\n\n", arg);
			ARGPARSE_HELP();
			break;
		}
		
		ARG_END() {
			if(!input_filename) {
				printf("Missing required argument --input-file!\n\n");
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
