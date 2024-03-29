#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include "kjc_argparse.h"

/*
Attempting to duplicate something like `docker login`:

$ docker login --help
Log in to a Docker registry or cloud backend.
If no registry server is specified, the default is defined by the daemon.

Usage:
  docker login [OPTIONS] [SERVER] [flags]
  docker login [command]

Available Commands:
  azure       Log in to azure

Flags:
  -h, --help              Help for login
  -p, --password string   password
      --password-stdin    Take the password from stdin
  -u, --username string   username

Use "docker login [command] --help" for more information about a command.
$ docker login azure --help
Log in to azure

Usage:
  docker login azure [flags]

Flags:
      --client-id string       Client ID for Service principal login
      --client-secret string   Client secret for Service principal login
      --cloud-name string      Name of a registered Azure cloud [AzureCloud | AzureChinaCloud | AzureGermanCloud | AzureUSGovernment] (AzureCloud by default)
  -h, --help                   Help for azure
      --tenant-id string       Specify tenant ID to use
*/


int docker_login_azure(struct kjc_argparse* context) {
	ARGPARSE_RESUME(context) {
		ARGPARSE_CONFIG_CUSTOM_USAGE(
			"Log in to azure\n"
			"\n"
			"Usage:\n"
			"  docker login azure [flags]"
		);
		
		ARG_STRING(0, "client-id", "Client ID for Service principal login", arg);
		ARG_STRING(0, "client-secret", "Client secret for Service principal login", arg);
		ARG_STRING(0, "cloud-name", "Name of a registered Azure cloud [AzureCloud | AzureChinaCloud | AzureGermanCloud | AzureUSGovernment] (AzureCloud by default)", arg);
		ARG('h', "help", "Help for azure") {
			ARGPARSE_HELP();
			break;
		}
		ARG_STRING(0, "tenant-id", "Specify tenant ID to use", arg);
		
		ARG_END {
			fprintf(stderr, "Imagine this opened your browser to the Azure auth page...\n");
		}
	}
	
	return 0;
}

int docker_container_list(struct kjc_argparse* context) {
	ARGPARSE_RESUME(context) {
		ARG_END {
			fprintf(stderr, "Imagine this listed running containers...\n");
		}
	}
	
	return 0;
}

int docker_container(struct kjc_argparse* context) {
	int ret = 0;
	
	ARGPARSE_RESUME(context) {
		ARG_COMMAND("ls", "List containers") {
			ret = docker_container_list(ARGPARSE_GET_CONTEXT());
			break;
		}
		ARG_COMMAND("list", NULL) {
			ret = docker_container_list(ARGPARSE_GET_CONTEXT());
			break;
		}
		ARG_COMMAND("ps", NULL) {
			ret = docker_container_list(ARGPARSE_GET_CONTEXT());
			break;
		}
	}
	
	return ret;
}

int main(int argc, char** argv) {
	int ret = 0;
	bool stop = false;
	
	// Pretend to be docker
	argv[0] = "docker";
	
	ARGPARSE(argc, argv) {
		ARGPARSE_CONFIG_CUSTOM_USAGE(
			"\n"
			"Usage:  docker [OPTIONS] COMMAND\n"
			"\n"
			"A self-sufficient runtime for containers"
		);
		
		ARGPARSE_CONFIG_HELP_SUFFIX(
			"Run 'docker COMMAND --help' for more information on a command.\n"
			"\n"
			"For more help on how to use Docker, head to https://docs.docker.com/go/guides/"
		);
		
		ARGPARSE_CONFIG_COMMAND_DESCRIPTION_COLUMN(14);
		
		ARG_COMMAND("login", "Log in to a registry") {
			ARGPARSE_NESTED {
				ARGPARSE_CONFIG_CUSTOM_USAGE(
					"Log in to a Docker registry or cloud backend.\n"
					"If no registry server is specified, the default is defined by the daemon.\n"
					"\n"
					"Usage:\n"
					"  docker login [OPTIONS] [SERVER] [flags]\n"
					"  docker login [command]"
				);
				
				ARGPARSE_CONFIG_HELP_SUFFIX(
					"Use \"docker login [command] --help\" for more information about a command."
				);
				
				ARGPARSE_CONFIG_COMMAND_DESCRIPTION_COLUMN(14);
				
				ARG_COMMAND("azure", "Log in to azure") {
					ret = docker_login_azure(ARGPARSE_GET_CONTEXT());
					stop = true;
					break;
				}
				
				ARG('h', "help", "Help for login") {
					ARGPARSE_HELP();
					stop = true;
					break;
				}
				
				ARG_STRING('p', "password", "password", arg);
				ARG(0, "password-stdin", "Take the password from stdin");
				ARG_STRING('u', "username", "username", arg);
				
				ARG_END {
					fprintf(stderr, "Imagine this asked for your username and password...\n");
				}
			}
			
			if(stop) {
				break;
			}
		}
		
		ARG_COMMAND("container", "Manage containers") {
			ret = docker_container(ARGPARSE_GET_CONTEXT());
			break;
		}
		
		ARG_COMMAND("ps", "List containers") {
			ret = docker_container_list(ARGPARSE_GET_CONTEXT());
			break;
		}
		
		ARG_END {
			ARGPARSE_HELP();
		}
	}
	
	return ret;
}
