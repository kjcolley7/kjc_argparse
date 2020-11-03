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
		
		ARG_POSITIONAL("input1.json {inputN.json...}", arg) {
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
