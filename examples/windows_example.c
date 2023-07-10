#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include "kjc_argparse.h"

int main(int argc, char** argv) {
	// Pretending to be ipconfig
	argv[0] = "ipconfig";
	
	ARGPARSE(argc, argv) {
		// Windows CLI tools usually have arguments starting with a single "/"
		ARGPARSE_CONFIG_LONG_PREFIX("/");
		
		// The "--" argument doesn't make sense for Windows CLI tools
		ARGPARSE_CONFIG_DASHDASH(false);
		
		// Windows CLI tools don't have "/help", but "/?" instead
		ARGPARSE_CONFIG_AUTO_HELP(false);
		
		ARGPARSE_CONFIG_INDENT(7);
		
		// Handler for "/?"
		ARG(0, "?", "Display this help message") {
			ARGPARSE_HELP();
			return 0;
		}
		
		ARG(0,        "all",          "Display full configuration information.") {
			printf("Showing all info...\n");
		}
		ARG_STRING(0, "release",      "Release the IPv4 address for the specified adapter.", adapter) {
			printf("Releasing (IPv4) %s...\n", adapter);
		}
		ARG_STRING(0, "release6",     "Release the IPv6 address for the specified adapter.", adapter) {
			printf("Releasing (IPv6) %s...\n", adapter);
		}
		ARG_STRING(0, "renew",        "Renew the IPv4 address for the specified adapter.", adapter) {
			printf("Renewing (IPv4) %s...\n", adapter);
		}
		ARG_STRING(0, "renew6",       "Renew the IPv6 address for the specified adapter.", adapter) {
			printf("Renewing (IPv6) %s...\n", adapter);
		}
		ARG(0,        "flushdns",     "Purge the DNS Resolver cache.") {
			printf("Flushed DNS cache.\n");
		}
		ARG(0,        "registerdns",  "Refreshes all DHCP leases and re-registers DNS names") {
			printf("Refreshed DHCP leases and DNS names.\n");
		}
		ARG(0,        "displaydns",   "Display the contents of the DNS Resolver Cache.") {
			printf("DNS info:\n");
			FILE* fp = fopen("/etc/resolv.conf", "r");
			if(!fp) {
				break;
			}
			
			char buffer[100];
			while(fgets(buffer, sizeof(buffer), fp)) {
				printf("%s", buffer);
			}
			
			fclose(fp);
		}
		ARG_STRING(0, "showclassid",  "Displays all the dhcp class IDs allowed for adapter.", adapter) {
			printf("IPv4 ClassIds for %s: <none>\n", adapter);
		}
		ARG_STRING(0, "setclassid",   "Modifies the dhcp class id.", adapter_classid) {
			// Syntax is `ipconfig /setclassid <adapter> [<classid>]`
			char* classid = ARGPARSE_NEXT();
			if(classid) {
				// If next argument is an option starting with slash, put it back
				if(classid[0] == '/') {
					ARGPARSE_REWIND(1);
					classid = NULL;
				}
			}
			
			if(!classid) {
				printf("Removed IPv4 ClassId from %s.\n", adapter_classid);
			}
			else {
				printf("IPv4 ClassId for %s set to %s.\n", adapter_classid, classid);
			}
		}
		ARG_STRING(0, "showclassid6", "Displays all the IPv6 DHCP class IDs allowed for adapter.", adapter) {
			printf("IPv6 ClassIds for %s: <none>\n", adapter);
		}
		ARG_STRING(0, "setclassid6",  "Modifies the IPv6 DHCP class id.", adapter_classid) {
			// Syntax is `ipconfig /setclassid6 <adapter> [<classid>]`
			char* classid = ARGPARSE_NEXT();
			if(classid) {
				// If next argument is an option starting with slash, put it back
				if(classid[0] == '/') {
					ARGPARSE_REWIND(1);
					classid = NULL;
				}
			}
			
			if(!classid) {
				printf("Removed IPv6 ClassId from %s.\n", adapter_classid);
			}
			else {
				printf("IPv6 ClassId for %s set to %s.\n", adapter_classid, classid);
			}
		}
		
		ARGPARSE_CONFIG_HELP_SUFFIX(
			"\n"
			"The default is to display only the IP address, subnet mask and\n"
			"default gateway for each adapter bound to TCP/IP.\n"
			"\n"
			"For Release and Renew, if no adapter name is specified, then the IP address\n"
			"leases for all adapters bound to TCP/IP will be released or renewed.\n"
			"\n"
			"For Setclassid and Setclassid6, if no ClassId is specified, then the ClassId is removed.\n"
		);
	}
	
	return 0;
}
