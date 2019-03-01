//
//  kjc_argparse.h
//
//  Created by Kevin Colley on 2/26/19.
//  Copyright © 2019 Kevin Colley. All rights reserved.
//

#include "kjc_argparse.h"

#include <stddef.h>
#include <stdint.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>


int _argparse_init(struct _argparse* argparse_context, int argc, char** argv, int long_name_max_width) {
	argparse_context->orig_argc = argc;
	argparse_context->orig_argv = argv;
	argparse_context->long_name_max_width = long_name_max_width;
	
	/* Return initial state */
	return ARG_VALUE_COUNT;
}

void _argparse_add(
	struct _argparse* argparse_context,
	int arg_id,
	char short_name,
	const char* long_name,
	const char* description,
	_argtype type
) {
	/* Error if neither a short name nor a long name are provided */
	if(short_name == '\0' && !long_name) {
		abort();
	}
	
	/* Memset-init, then set fields */
	struct _arginfo arg = {};
	arg.arg_id = arg_id;
	arg.short_name = short_name;
	arg.long_name = long_name;
	arg.description = description;
	arg.type = type;
	
	/* Actually register argument in argparse context structure by appending it to the args array */
	if(argparse_context->args_count >= argparse_context->args_cap) {
		abort();
	}
	argparse_context->args[argparse_context->args_count++] = arg;
	
	/* Update max long_name_width while respecting the upper bound */
	if(long_name) {
		int arglen = (int)strlen(long_name);
		if(arglen > argparse_context->long_name_max_width) {
			arglen = argparse_context->long_name_max_width;
		}
		
		if(arglen > argparse_context->long_name_width) {
			argparse_context->long_name_width = arglen;
		}
	}
	
	/* Add short option to short_bitmap */
	if(short_name != '\0') {
		/* High 5 bits are used as byte index, low 3 bits are bit index */
		argparse_context->short_bitmap[(unsigned char)short_name >> 3] |= (1 << (short_name & 7));
		
		/* Add short option to short_value_bitmap if it takes a value */
		if(type != ARG_TYPE_VOID) {
			argparse_context->short_value_bitmap[(unsigned char)short_name >> 3] |= (1 << (short_name & 7));
		}
	}
}

static inline bool _argparse_has_short_option(struct _argparse* argparse_context, char short_name) {
	return short_name != '\0' && !!(argparse_context->short_bitmap[(unsigned char)short_name >> 3] & (1 << (short_name & 7)));
}

static inline bool _argparse_short_option_expects_value(struct _argparse* argparse_context, char short_name) {
	return short_name != '\0' && !!(argparse_context->short_value_bitmap[(unsigned char)short_name >> 3] & (1 << (short_name & 7)));
}

int _argparse_parse(struct _argparse* argparse_context, int* argidx, int state) {
	int ret = ARG_VALUE_OTHER;
	struct _arginfo* arginfo = NULL;
	const char* argval_str = NULL;
	const char* arg = NULL;
	size_t arglen = 0;
	char c;
	unsigned i;
	
	if(state == ARG_VALUE_COUNT) {
		if(argparse_context->args_cap == 0) {
			if(!argparse_context->has_catchall) {
				// An argument parser with no registered arg handlers an no catchall is pointless
				abort();
			}
		}
		else {
			/* We now know how many arguments we will need to register, so allocate memory */
			argparse_context->args = calloc(argparse_context->args_cap, sizeof(*argparse_context->args));
			if(!argparse_context->args) {
				perror("calloc");
				abort();
			}
		}
		
		/* Transition into initialization phase */
		return ARG_VALUE_INIT;
	}
	
	/* Time for cleanup? */
	if(state == ARG_VALUE_END) {
		argparse_context->args_count = 0;
		argparse_context->args_cap = 0;
		free(argparse_context->args);
		argparse_context->args = NULL;
		return ARG_VALUE_END;
	}
	
	if(state == ARG_VALUE_INIT) {
		/* Initialization phase just ended, check consistency */
		if(argparse_context->args_count != argparse_context->args_cap) {
			abort();
		}
		
		/* Fallthrough to start parsing arguments */
	}
	
	/* Multiple short options in a single argument like ls -laF */
	if(argparse_context->argtype == ARG_TYPE_SHORTGROUP) {
		/* Read next character of current argument */
		c = *argparse_context->argvalue.val_string++;
		if(c != '\0') {
			/* Next character exists, so look it up */
			for(i = 0; i < argparse_context->args_count; i++) {
				struct _arginfo* pcur = &argparse_context->args[i];
				if(c == pcur->short_name) {
					/* Found an argument handler registered to handle this short option */
					arginfo = pcur;
					ret = pcur->arg_id;
					goto parse_done;
				}
			}
			
			/* We should have already ensured that all characters in this argument are valid short options */
			abort();
		}
	}
	
	/* Clear argument type and value */
	argparse_context->argtype = ARG_TYPE_VOID;
	memset(&argparse_context->argvalue, 0, sizeof(argparse_context->argvalue));
	
	/* Did we parse all of the arguments? */
	if(*argidx == argparse_context->orig_argc) {
		/*
		 * Don't cleanup resources just yet, we'll do that after one more iteration
		 * to allow ARG_END to use ARGPARSE_HELP()
		 */
		return ARG_VALUE_END;
	}
	
	arg = argparse_context->orig_argv[(*argidx)++];
	if(arg[0] != '-') {
		/* Argument doesn't start with a dash, so it's an "other" argument */
		goto parse_done;
	}
	
	/* Get argument length just once */
	arglen = strlen(arg);
	if(arglen == 2) {
		/* Quick check to ensure the character is a registered short option */
		if(_argparse_has_short_option(argparse_context, arg[1])) {
			/* This is a short argument, so check all the short args */
			for(i = 0; i < argparse_context->args_count; i++) {
				struct _arginfo* pcur = &argparse_context->args[i];
				if(arg[1] == pcur->short_name) {
					arginfo = pcur;
					ret = pcur->arg_id;
					goto parse_done;
				}
			}
			
			/* Shouldn't be possible */
			abort();
		}
	}
	else if(arglen < 2) {
		/* Empty string or single dash, pass to ARG_OTHER */
		goto parse_done;
	}
	else if(arg[1] != '-') {
		/* Multiple short options in a single argument */
		
		/* Ensure that every character in this argument is a registered short option */
		for(i = 1; i < arglen; i++) {
			if(!_argparse_has_short_option(argparse_context, arg[i])) {
				printf("In argument \"%s\", there is no supported option \"-%c\"\n", arg, arg[i]);
				ret = ARG_VALUE_ERROR;
				goto parse_done;
			}
			
			/* Only the last short option can take a value */
			if(_argparse_short_option_expects_value(argparse_context, arg[i]) && i != arglen - 1) {
				printf("In argument \"%s\", option -%c expects a value and therefore must be the last character.\n", arg, arg[i]);
				ret = ARG_VALUE_ERROR;
				goto parse_done;
			}
		}
		
		/* Store current argument character position in argvalue.val_string */
		argparse_context->argtype = ARG_TYPE_SHORTGROUP;
		argparse_context->argvalue.val_string = &arg[2];
		
		/* Look up short option character */
		c = arg[1];
		for(i = 0; i < argparse_context->args_count; i++) {
			struct _arginfo* pcur = &argparse_context->args[i];
			if(c == pcur->short_name) {
				/* Found an argument handler registered to handle this short option */
				arginfo = pcur;
				ret = pcur->arg_id;
				goto parse_done;
			}
		}
		
		/* Shouldn't be possible as we just ensured that all characters are registered short options */
		abort();
	}
	else {
		size_t long_name_length = arglen - 2;
		
		/* Check if this argument is in the form --long-with-value=foo */
		argval_str = strchr(arg, '=');
		if(argval_str) {
			long_name_length = argval_str - &arg[2];
			argval_str++;
		}
		
		/* This is a properly formed long argument, so check all the long args */
		for(i = 0; i < argparse_context->args_count; i++) {
			struct _arginfo* pcur = &argparse_context->args[i];
			
			/*
			 * Check that the first long_name_length characters of the registered argument's long name
			 * match this argument's long name and that the registered argument's long name ends there.
			 * This is to ensure that --test=4 won't match an argument registered with long name "testing".
			 */
			if(
				pcur->long_name &&
				strncmp(&arg[2], pcur->long_name, long_name_length) == 0 &&
				pcur->long_name[long_name_length] == '\0'
			) {
				arginfo = pcur;
				ret = pcur->arg_id;
				goto parse_done;
			}
		}
	}
	
parse_done:
	/* Did we fail to parse this argument? */
	if(ret == ARG_VALUE_OTHER) {
		/* Error out unless there's an ARG_OTHER catchall present */
		if(argparse_context->has_catchall) {
			return ret;
		}
		else {
			printf("Unexpected argument: \"%s\"\n", arg);
			return ARG_VALUE_ERROR;
		}
	}
	else if(arginfo != NULL) {
		/* We found a matching argument, now check if it has a non-void type */
		if(arginfo->type != ARG_TYPE_VOID) {
			/* Get argument value string */
			if(!argval_str) {
				/* This argument expects a value as the next argument like --test foo */
				if(*argidx == argparse_context->orig_argc) {
					/* No more arguments, so this is an error */
					return ARG_VALUE_ERROR;
				}
				else {
					/* Get next argument */
					argval_str = argparse_context->orig_argv[(*argidx)++];
				}
			}
			
			switch(arginfo->type) {
				case ARG_TYPE_INT: {
					/* Parse the string as an integer with strtol() to support negative ints and other bases */
					char* str_end = NULL;
					int val = (int)strtol(argval_str, &str_end, 0);
					if(*str_end != '\0') {
						/* Failed to fully parse argument value string */
						if(arginfo->long_name) {
							printf("The --%s", arginfo->long_name);
						}
						else {
							printf("The -%c", arginfo->short_name);
						}
						printf(" option expects a positive number value, not \"%s\".\n", argval_str);
						
						// Display help message
						_argparse_help(argparse_context);
						return EXIT_FAILURE;
					}
					
					/* Store argument type and value in argparse context */
					argparse_context->argtype = ARG_TYPE_INT;
					argparse_context->argvalue.val_int = val;
					break;
				}
				
				case ARG_TYPE_STRING:
					/* Store argument type and value in argparse context */
					argparse_context->argtype = ARG_TYPE_STRING;
					argparse_context->argvalue.val_string = argval_str;
					break;
				
				default:
					abort();
			}
		}
	}
	
	return ret;
}

/* For sorting short options, sort case insensitive but caps comes first to break tie */
static int charcmp(const void* a, const void* b) {
	int x = *(const char*)a;
	int y = *(const char*)b;
	int diff = toupper(x) - toupper(y);
	
	return diff ?: x - y;
}

void _argparse_help(struct _argparse* argparse_context) {
	char shortOptions[256] = {};
	size_t shortOptionCount = 0;
	uint16_t i;
	
	/* Get all short option names */
	for(i = 0; i < argparse_context->args_count; i++) {
		struct _arginfo* pcur = &argparse_context->args[i];
		if(pcur->short_name != '\0') {
			if(shortOptionCount >= sizeof(shortOptions)) {
				abort();
			}
			shortOptions[shortOptionCount++] = pcur->short_name;
		}
	}
	
	/* Print usage header with program name */
	printf("Usage: %s", argparse_context->orig_argv[0]);
	
	if(shortOptionCount > 0) {
		/* Sort short option names by their ASCII values */
		qsort(shortOptions, shortOptionCount, sizeof(*shortOptions), charcmp);
		
		/* Print all available short options */
		printf(" [-%s]", shortOptions);
	}
	
	printf(" [...]\n");
	printf("Options:\n");
	
	/* Print description of each argument */
	for(i = 0; i < argparse_context->args_count; i++) {
		struct _arginfo* pcur = &argparse_context->args[i];
		printf("    ");
		
		/* Print short option (if set) */
		if(pcur->short_name != '\0') {
			printf("-%c", pcur->short_name);
		}
		else {
			printf("  ");
		}
		
		/* Print long option (if set) */
		if(pcur->long_name != NULL) {
			/* If short option was set, separate it with a comma */
			if(pcur->short_name != '\0') {
				printf(", ");
			}
			else {
				printf("  ");
			}
			
			/* Print long option */
			printf("--%-*s", argparse_context->long_name_width, pcur->long_name);
		}
		else {
			/* Print spaces to pad the space where a long option would normally be */
			printf("%*s", 2 + 2 + argparse_context->long_name_width, "");
		}
		
		/* Print the argument's type if not void */
		int spacesBeforeDescription = 1;
		switch(pcur->type) {
			case ARG_TYPE_INT:
				printf("  [int]");
				break;
			
			case ARG_TYPE_STRING:
				printf("  [string]");
				break;
			
			default:
				spacesBeforeDescription = 2;
				break;
		}
		
		/* Print the description (if set) */
		if(pcur->description != NULL) {
			printf("%*s%s", spacesBeforeDescription, "", pcur->description);
		}
		
		/* End the line */
		printf("\n");
	}
}

int _argparse_value_int(struct _argparse* argparse_context) {
	if(argparse_context->argtype != ARG_TYPE_INT) {
		abort();
	}
	return argparse_context->argvalue.val_int;
}

const char* _argparse_value_string(struct _argparse* argparse_context) {
	if(argparse_context->argtype != ARG_TYPE_STRING) {
		abort();
	}
	return argparse_context->argvalue.val_string;
}
