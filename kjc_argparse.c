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
#include <errno.h>
#include <assert.h>


#ifdef NDEBUG
#define argparse_assert(x) do { \
	if(!(x)) { \
		abort(); \
	} \
} while(0)
#else /* NDEBUG */
#define argparse_assert assert
#endif /* NDEBUG */


void _argparse_init(struct kjc_argparse* argparse_context) {
	/* Subcommand argparse context? */
	if(argparse_context->parent) {
		argparse_context->argidx = argparse_context->parent->argidx;
		
		/* Need to grab argc and argv from parent context */
		argparse_context->orig_argc = argparse_context->parent->orig_argc;
		argparse_context->orig_argv = argparse_context->parent->orig_argv;
		
		/* This is needed later for printing program name in usage text */
		argparse_assert(argparse_context->parent->cur_arg->type == _kARG_TYPE_COMMAND);
	}
	else {
		argparse_context->argidx_top = 1;
		argparse_context->argidx = &argparse_context->argidx_top;
	}
	
	/* Configurable values */
	argparse_context->stream = ARGPARSE_DEFAULT_STREAM != (void*)1 ? ARGPARSE_DEFAULT_STREAM : stderr;
	argparse_context->custom_usage = ARGPARSE_DEFAULT_CUSTOM_USAGE;
	argparse_context->custom_suffix = ARGPARSE_DEFAULT_HELP_SUFFIX;
	argparse_context->long_arg_prefix = ARGPARSE_DEFAULT_LONG_PREFIX;
	argparse_context->long_prefix_len = strlen(ARGPARSE_DEFAULT_LONG_PREFIX);
	argparse_context->subcmd_description_column = ARGPARSE_DEFAULT_COMMAND_DESCRIPTION_COLUMN;
	argparse_context->description_column = ARGPARSE_DEFAULT_DESCRIPTION_COLUMN;
	argparse_context->indent = ARGPARSE_DEFAULT_INDENT;
	argparse_context->description_padding = ARGPARSE_DEFAULT_DESCRIPTION_PADDING;
	
	/* Configurable bit flags */
	argparse_context->flags = 0
		| (ARGPARSE_DEFAULT_USE_VARNAMES ? _kARGPARSE_USE_VARNAMES : 0)
		| (ARGPARSE_DEFAULT_TYPE_HINTS ? _kARGPARSE_TYPE_HINTS : 0)
		| (ARGPARSE_DEFAULT_SHORTGROUPS ? _kARGPARSE_WITH_SHORTGROUPS : 0)
		| (ARGPARSE_DEFAULT_AUTO_HELP ? _kARGPARSE_AUTO_HELP : 0)
		| (ARGPARSE_DEFAULT_DASHDASH ? _kARGPARSE_DASHDASH : 0)
		;
	
	/* Set initial state */
	argparse_context->state = _kARG_VALUE_COUNT;
}

static void _argparse_dealloc(struct kjc_argparse* argparse_context) {
	argparse_context->argstorage_count = 0;
	argparse_context->argstorage_cap = 0;
	argparse_context->subcmds_count = 0;
	argparse_context->subcmds_cap = 0;
	argparse_context->longargs_count = 0;
	argparse_context->longargs_cap = 0;
	argparse_context->shortargs_count = 0;
	argparse_context->shortargs_cap = 0;
	argparse_context->cur_arg = NULL;
	argparse_context->argvalue.val_string = NULL;
	memset(argparse_context->short_bitmap, 0, sizeof(argparse_context->short_bitmap));
	memset(argparse_context->short_value_bitmap, 0, sizeof(argparse_context->short_value_bitmap));
	free(argparse_context->argbuffer);
	argparse_context->argbuffer = NULL;
}

int _argparse_done(const struct kjc_argparse* argparse_context) {
	return argparse_context->state == _kARG_VALUE_ERROR
		|| argparse_context->state == _kARG_VALUE_BREAK
		|| (argparse_context->flags & _kARGPARSE_FLAG_DONE)
		;
}

static inline bool _argparse_has_short_option(const struct kjc_argparse* argparse_context, char short_name) {
	return short_name != '\0' &&
		!!(argparse_context->short_bitmap[(unsigned char)short_name >> 3] & (1 << (short_name & 7)));
}

static inline bool _argparse_short_option_expects_value(const struct kjc_argparse* argparse_context, char short_name) {
	return short_name != '\0' &&
		!!(argparse_context->short_value_bitmap[(unsigned char)short_name >> 3] & (1 << (short_name & 7)));
}

static inline struct _arginfo** _argparse_get_subcmds(const struct kjc_argparse* argparse_context) {
	return argparse_context->argbuffer;
}

static inline struct _arginfo** _argparse_get_longargs(const struct kjc_argparse* argparse_context) {
	struct _arginfo** subcmds = _argparse_get_subcmds(argparse_context);
	return &subcmds[argparse_context->subcmds_cap];
}

static inline struct _arginfo** _argparse_get_shortargs(const struct kjc_argparse* argparse_context) {
	struct _arginfo** long_args = _argparse_get_longargs(argparse_context);
	return &long_args[argparse_context->longargs_cap];
}

static inline size_t _argparse_get_args_cap(const struct kjc_argparse* argparse_context) {
	return argparse_context->subcmds_cap
		+ argparse_context->longargs_cap
		+ argparse_context->shortargs_cap;
}

static inline struct _arginfo* _argparse_get_argstorage(const struct kjc_argparse* argparse_context) {
	return (struct _arginfo*)(
		argparse_context->argbuffer + _argparse_get_args_cap(argparse_context) * sizeof(struct _arginfo*)
	);
}

static inline size_t _argparse_get_argbuffer_size(const struct kjc_argparse* argparse_context) {
	return _argparse_get_args_cap(argparse_context) * sizeof(struct _arginfo*) \
		+ argparse_context->argstorage_cap * sizeof(struct _arginfo);
}

void _argparse_add(
	struct kjc_argparse* argparse_context,
	int arg_id,
	char short_name,
	const char* long_name,
	const char* description,
	unsigned char type,
	const char* var_name
) {
	/* Error if neither a short name nor a long name are provided */
	argparse_assert(short_name != '\0' || long_name != NULL);
	
	/* Array of arginfo structs */
	struct _arginfo* argstorage = _argparse_get_argstorage(argparse_context);
	
	/* Memset-init, then set fields */
	struct _arginfo arg = {0};
	arg.arg_id = arg_id;
	arg.short_name = short_name;
	arg.long_name = long_name;
	arg.description = description;
	arg.type = type;
	arg.var_name = var_name;
	
	/* Store argument metadata in argparse context structure */
	argparse_assert(argparse_context->argstorage_count < argparse_context->argstorage_cap);
	struct _arginfo* parg = &argstorage[argparse_context->argstorage_count++];
	*parg = arg;
	
	/* Register long name (if set) */
	if(long_name != NULL) {
		if(type == _kARG_TYPE_COMMAND) {
			argparse_assert(argparse_context->subcmds_count < argparse_context->subcmds_cap);
			_argparse_get_subcmds(argparse_context)[argparse_context->subcmds_count++] = parg;
		}
		else {
			argparse_assert(argparse_context->longargs_count < argparse_context->longargs_cap);
			_argparse_get_longargs(argparse_context)[argparse_context->longargs_count++] = parg;
		}
	}
	
	/* Register short name (if set) */
	if(short_name != '\0') {
		/* Don't allow duplicate short options */
		argparse_assert(!_argparse_has_short_option(argparse_context, short_name));
		
		/* Add short option to short_bitmap */
		
		/* High 5 bits are used as byte index, low 3 bits are bit index */
		argparse_context->short_bitmap[(unsigned char)short_name >> 3] |= (1 << (short_name & 7));
		
		/* Add short option to short_value_bitmap if it takes a value */
		if(type != _kARG_TYPE_VOID) {
			argparse_context->short_value_bitmap[(unsigned char)short_name >> 3] |= (1 << (short_name & 7));
		}
		
		/* Actually register short argument in argparse context structure */
		argparse_assert(argparse_context->shortargs_count < argparse_context->shortargs_cap);
		_argparse_get_shortargs(argparse_context)[argparse_context->shortargs_count++] = parg;
	}
}


static int _arginfo_compare_long(const void* _a, const void* _b) {
	const struct _arginfo* const* a = _a;
	const struct _arginfo* const* b = _b;
	
	return strcmp((*a)->long_name, (*b)->long_name);
}

static int _arginfo_compare_short(const void* _a, const void* _b) {
	const struct _arginfo* const* a = _a;
	const struct _arginfo* const* b = _b;
	
	return (*a)->short_name - (*b)->short_name;
}

static const char* _argtype_name(unsigned char type) {
	switch(type) {
		case _kARG_TYPE_STRING: return "string";
		case _kARG_TYPE_LONG: return "int";
	}
	
	return NULL;
}

static const char* _arginfo_value_hint(const struct kjc_argparse* argparse_context, const struct _arginfo* arginfo) {
	if((argparse_context->flags & _kARGPARSE_USE_VARNAMES) && arginfo->var_name != NULL) {
		return arginfo->var_name;
	}
	
	return _argtype_name(arginfo->type);
}

static void _argparse_post_init(struct kjc_argparse* argparse_context) {
	/* Initialization phase just ended, check consistency */
	argparse_assert(argparse_context->subcmds_count == argparse_context->subcmds_cap);
	argparse_assert(argparse_context->longargs_count == argparse_context->longargs_cap);
	argparse_assert(argparse_context->shortargs_count == argparse_context->shortargs_cap);
	argparse_assert(argparse_context->argstorage_count == argparse_context->argstorage_cap);
	
	struct _arginfo** subcmds = _argparse_get_subcmds(argparse_context);
	struct _arginfo** longargs = _argparse_get_longargs(argparse_context);
	struct _arginfo** shortargs = _argparse_get_shortargs(argparse_context);
	
	/* Sort each sub-array of arginfo pointers */
	qsort(subcmds, argparse_context->subcmds_count, sizeof(*subcmds), _arginfo_compare_long);
	qsort(longargs, argparse_context->longargs_count, sizeof(*longargs), _arginfo_compare_long);
	qsort(shortargs, argparse_context->shortargs_count, sizeof(*shortargs), _arginfo_compare_short);
	
	/* Check for duplicate subcommands */
	const char* prev = NULL;
	for(unsigned i = 0; i < argparse_context->subcmds_count; i++) {
		const char* cur = subcmds[i]->long_name;
		if(prev && cur) {
			argparse_assert(strcmp(prev, cur) != 0 && "Duplicate subcommand");
		}
		
		prev = cur;
	}
	
	/* Check for duplicate long names */
	prev = NULL;
	for(unsigned i = 0; i < argparse_context->longargs_count; i++) {
		const char* cur = longargs[i]->long_name;
		if(prev && cur) {
			argparse_assert(strcmp(prev, cur) != 0 && "Duplicate long arg name");
		}
		
		prev = cur;
	}
	
	/* Compute longest subcmd width */
	for(unsigned i = 0; i < argparse_context->subcmds_count; i++) {
		unsigned cmdlen = (unsigned)strlen(subcmds[i]->long_name);
		if(cmdlen > argparse_context->subcmd_width) {
			argparse_context->subcmd_width = cmdlen;
		}
	}
	
	/* Compute longest long arg width */
	for(unsigned i = 0; i < argparse_context->longargs_count; i++) {
		/* Update max long_name_width */
		unsigned arglen = (unsigned)strlen(longargs[i]->long_name);
		const char* valhint = _arginfo_value_hint(argparse_context, longargs[i]);
		if(valhint) {
			/* For something like "--count <num>", this counts the length of the " <num>" part */
			arglen += 3 + (unsigned)strlen(valhint);
		}
		
		if(arglen > argparse_context->long_name_width) {
			argparse_context->long_name_width = arglen;
		}
	}
	
	/* In case the long argument prefix was changed */
	argparse_context->long_prefix_len = strlen(argparse_context->long_arg_prefix);
}

static int _arginfo_find_long(const void* key, const void* item) {
	const char* name = key;
	const struct _arginfo* const* parg = item;
	argparse_assert((*parg)->long_name != NULL);
	
	/* Check up to the length of arginfo's long arg */
	size_t long_name_len = strlen((*parg)->long_name);
	int diff = strncmp(name, (*parg)->long_name, long_name_len);
	if(diff) {
		return diff;
	}
	
	/* Fully matched long arg? */
	if(name[long_name_len] == '\0') {
		return 0;
	}
	
	/* Allow if next char is '=', for cases like --foo-arg=42 */
	if(name[long_name_len] == '=') {
		return 0;
	}
	
	/*
	 * Example case: name="--foo-arg", arginfo->long_name="--foo"
	 * The long arg is just a prefix of the one we're searching for.
	 * If a registered arg with this name exists, it must be later in the array.
	 */
	return 1;
}

static int _arginfo_find_short(const void* key, const void* item) {
	const char* name = key;
	const struct _arginfo* const* parg = item;
	argparse_assert((*parg)->short_name != '\0');
	
	return *name - (*parg)->short_name;
}

static struct _arginfo** _args_search_long(struct _arginfo** args, const char* name, unsigned count) {
	return bsearch(name, args, count, sizeof(*args), _arginfo_find_long);
}

static struct _arginfo** _args_search_short(struct _arginfo** args, char name, unsigned count) {
	return bsearch(&name, args, count, sizeof(*args), _arginfo_find_short);
}

static struct _arginfo* _argparse_find_subcmd(struct kjc_argparse* argparse_context, const char* subcmd) {
	struct _arginfo** parg = _args_search_long(
		_argparse_get_subcmds(argparse_context), subcmd, argparse_context->subcmds_count
	);
	return parg ? *parg : NULL;
}

static struct _arginfo* _argparse_find_longarg(struct kjc_argparse* argparse_context, const char* longarg) {
	struct _arginfo** parg = _args_search_long(
		_argparse_get_longargs(argparse_context), longarg, argparse_context->longargs_count
	);
	return parg ? *parg : NULL;
}

static struct _arginfo* _argparse_find_shortarg(struct kjc_argparse* argparse_context, char shortarg) {
	struct _arginfo** parg = _args_search_short(
		_argparse_get_shortargs(argparse_context), shortarg, argparse_context->shortargs_count
	);
	return parg ? *parg : NULL;
}

#ifndef NDEBUG
static inline const char* _argparse_repr_type(unsigned char type) {
	switch(type) {
		case _kARG_TYPE_VOID: return "_kARG_TYPE_VOID";
		case _kARG_TYPE_STRING: return "_kARG_TYPE_STRING";
		case _kARG_TYPE_LONG: return "_kARG_TYPE_LONG";
		case _kARG_TYPE_SHORTGROUP: return "_kARG_TYPE_SHORTGROUP";
		case _kARG_TYPE_COMMAND: return "_kARG_TYPE_COMMAND";
		default: return "<invalid>";
	}
}

static inline void _dump_char_or_nul(char c, FILE* f) {
	if(c == '\0') {
		fprintf(f, "'\\0'");
	}
	else {
		fprintf(f, "'%c'", c);
	}
}

static inline void _dump_string_or_null(const char* s, FILE* f) {
	if(s == NULL) {
		fprintf(f, "NULL");
	}
	else {
		fprintf(f, "\"%s\"", s);
	}
}

static inline void _arginfo_dump(const struct _arginfo* arginfo, FILE* f) {
	fprintf(f, "%p {\n", arginfo);
	fprintf(f, "    .arg_id = %d,\n", arginfo->arg_id);
	
	fprintf(f, "    .short_name = ");
	_dump_char_or_nul(arginfo->short_name, f);
	fprintf(f, ",\n");
	
	fprintf(f, "    .long_name = ");
	_dump_string_or_null(arginfo->long_name, f);
	fprintf(f, ",\n");
	
	fprintf(f, "    .description = ");
	_dump_string_or_null(arginfo->description, f);
	fprintf(f, ",\n");
	
	fprintf(f, "    .type = %s,\n", _argparse_repr_type(arginfo->type));
	
	fprintf(f, "    .var_name = ");
	_dump_string_or_null(arginfo->var_name, f);
	fprintf(f, ",\n");
	
	fprintf(f, "}\n");
}

static inline void _arginfo_dump_multiple(struct _arginfo** args, unsigned count, FILE* f) {
	for(unsigned i = 0; i < count; i++) {
		fprintf(f, "[%u] ", i);
		_arginfo_dump(args[i], f);
	}
}

static inline const char* _argparse_state_name(const struct kjc_argparse* argparse_context, int state) {
	static char buf[50];
	
	switch(state) {
		case _kARG_VALUE_INIT: return "INIT";
		case _kARG_VALUE_COUNT: return "COUNT";
		case _kARG_VALUE_POSITIONAL: return "POSITIONAL";
		case _kARG_VALUE_OTHER: return "OTHER";
		case _kARG_VALUE_END: return "END";
		case _kARG_VALUE_BREAK: return "BREAK";
		case _kARG_VALUE_ERROR: return "ERROR";
	}
	
	const struct _arginfo* argstorage = _argparse_get_argstorage(argparse_context);
	
	for(unsigned i = 0; i < argparse_context->argstorage_count; i++) {
		const struct _arginfo* pcur = &argstorage[i];
		if(pcur->arg_id == state) {
			if(pcur->long_name != NULL) {
				if(pcur->type == _kARG_TYPE_COMMAND) {
					snprintf(buf, sizeof(buf), "COMMAND(\"%s\")", pcur->long_name);
				}
				snprintf(buf, sizeof(buf), "ARG(\"%s%s\")", argparse_context->long_arg_prefix, pcur->long_name);
			}
			else {
				snprintf(buf, sizeof(buf), "ARG(\"-%c\")", pcur->short_name);
			}
		}
	}
	
	return buf;
}
#endif /* NDEBUG */

void _argparse_parse(struct kjc_argparse* argparse_context) {
	int ret = _kARG_VALUE_OTHER;
	struct _arginfo* arginfo = NULL;
	const char* argval_str = NULL;
	const char* arg = NULL;
	size_t arglen = 0;
	FILE* f = argparse_context->stream;
	int state = argparse_context->state;
	
	if(state == _kARG_VALUE_COUNT) {
		size_t bufsize = _argparse_get_argbuffer_size(argparse_context);
		if (bufsize > 0) {
			/* We now know how many arguments we will need to register, so allocate memory */
			argparse_context->argbuffer = calloc(1, bufsize);
			argparse_assert(argparse_context->argbuffer != NULL && "Allocation failure");
		}
		
		/* Transition into initialization phase */
		ret = _kARG_VALUE_INIT;
		goto out;
	}
	
	/* Illegal to enter the state machine in the error state */
	argparse_assert(state != _kARG_VALUE_ERROR);
	
	/* Time for cleanup? */
	if(state == _kARG_VALUE_END || state == _kARG_VALUE_BREAK) {
		_argparse_dealloc(argparse_context);
		
		/* Set the done flag but still do one more parsing run (to support ARG_END) */
		if(state == _kARG_VALUE_END) {
			argparse_context->flags |= _kARGPARSE_FLAG_DONE;
		}
		
		ret = state;
		goto out;
	}
	
	/* Just finished with init phase? */
	if(state == _kARG_VALUE_INIT) {
		_argparse_post_init(argparse_context);
		
#ifndef NDEBUG
		if (argparse_context->flags & _kARGPARSE_DEBUG && f != NULL) {
			fprintf(f, "subcmds:\n");
			_arginfo_dump_multiple(_argparse_get_subcmds(argparse_context), argparse_context->subcmds_count, f);
			
			fprintf(f, "\nlongargs:\n");
			_arginfo_dump_multiple(_argparse_get_longargs(argparse_context), argparse_context->longargs_count, f);
			
			fprintf(f, "\nshortargs:\n");
			_arginfo_dump_multiple(_argparse_get_shortargs(argparse_context), argparse_context->shortargs_count, f);
		}
#endif /* NDEBUG */
		
		/* Fallthrough to start parsing arguments */
	}
	
	/* Multiple short options in a single argument like ls -laF */
	if(argparse_context->argtype == _kARG_TYPE_SHORTGROUP) {
		/* Read next character of current argument */
		char c = *argparse_context->argvalue.val_string++;
		if(c != '\0') {
			/* Next character exists, so look it up */
			arginfo = _argparse_find_shortarg(argparse_context, c);
			
			/* We should have already ensured that all characters in this argument are valid short options */
			argparse_assert(arginfo != NULL);
			
			/* Found an argument handler registered to handle this short option */
			goto parse_done;
		}
	}
	else if(argparse_context->argtype == _kARG_TYPE_COMMAND) {
		/* Don't do any more parsing in this argparse context after encountering a subcommand */
		ret = _kARG_VALUE_END;
		goto out;
	}
	else if(argparse_context->argtype == _kARG_TYPE_DASHDASH) {
		/* This label is jumped to directly after finding the "--" argument */
	dash_dash:
		/* Treat all remaining arguments as ARG_POSITIONAL */
		if((*argparse_context->argidx)++ < argparse_context->orig_argc) {
			ret = _kARG_VALUE_POSITIONAL;
			goto parse_done;
		}
		
		/* Clear out "dashdash" status and mark arg parsing as done */
		argparse_context->argtype = _kARG_TYPE_VOID;
		ret = _kARG_VALUE_END;
		goto out;
	}
	
	/* Clear argument type and value */
	argparse_context->argtype = _kARG_TYPE_VOID;
	memset(&argparse_context->argvalue, 0, sizeof(argparse_context->argvalue));
	
	/* Grab next argument (if not at the end) */
	arg = _argparse_next(argparse_context);
	if(!arg) {
		/*
		 * Don't cleanup resources just yet, we'll do that after one more iteration
		 * to allow ARG_END to use ARGPARSE_HELP()
		 */
		ret = _kARG_VALUE_END;
		goto out;
	}
	
	/* Check if this arg is a subcmd */
	arginfo = _argparse_find_subcmd(argparse_context, arg);
	if(arginfo) {
		argparse_context->argtype = _kARG_TYPE_COMMAND;
		goto parse_done;
	}
	
	/* Get argument length just once */
	arglen = strlen(arg);
	
	/* In case the long argument prefix was overridden to something like "-", check that first */
	if(
		arglen > argparse_context->long_prefix_len
		&& strncmp(arg, argparse_context->long_arg_prefix, argparse_context->long_prefix_len) == 0
	) {
		/* Long option */
		const char* longarg = &arg[argparse_context->long_prefix_len];
		arginfo = _argparse_find_longarg(argparse_context, longarg);
		if(!arginfo) {
			/* Support for the auto help handler (--help, /help, depending on prefix) */
			if((argparse_context->flags & _kARGPARSE_AUTO_HELP) && strcmp(longarg, "help") == 0) {
				ret = _kARG_VALUE_HELP;
			}
			goto parse_done;
		}
		
		/* Check if this argument is in the form --long-with-value=foo */
		argval_str = strchr(arg, '=');
		if(argval_str) {
			argval_str++;
		}
		
		goto parse_done;
	}
	
	if(arg[0] != '-' || arglen < 2) {
		/*
		 * Positional arguments include:
		 *
		 * - Any string that doesn't start with a '-' (like "foo")
		 * - The string "-" (commonly used as a filename to mean stdin or stdout)
		 * - The empty string (might be used as a blank value on purpose)
		 */
		ret = _kARG_VALUE_POSITIONAL;
		goto parse_done;
	}
	else if(arglen == 2) {
		/* Check for support of "--" (if configured) */
		if(arg[1] == '-' && (argparse_context->flags & _kARGPARSE_DASHDASH)) {
			/* Don't override a user-defined "--" handler if present */
			if(!_argparse_has_short_option(argparse_context, '-')) {
				argparse_context->argtype = _kARG_TYPE_DASHDASH;
				goto dash_dash;
			}
		}
		
		/* Single short argument */
		arginfo = _argparse_find_shortarg(argparse_context, arg[1]);
		
		/* If not found, will be passed to ARG_OTHER */
		goto parse_done;
	}
	else if(arg[1] != '-') {
		/* Multiple short options in a single argument, like "-xzf" in "tar -xzf archive.tar.gz" */
		if(!(argparse_context->flags & _kARGPARSE_WITH_SHORTGROUPS)) {
			/* If support for short groups is disabled, pass to the ARG_OTHER handler */
			goto parse_done;
		}
		
		/* Ensure that every character in this argument is a registered short option */
		for(unsigned i = 1; i < arglen; i++) {
			if(!_argparse_has_short_option(argparse_context, arg[i]) || arg[i] == '-') {
				if(!(argparse_context->flags & _kARGPARSE_HAS_CATCHALL)) {
					if(f != NULL) {
						fprintf(f,
							"Error: In argument \"%s\", there is no supported option '-%c'\n",
							arg, arg[i]
						);
					}
					ret = _kARG_VALUE_ERROR;
				}
				goto parse_done;
			}
			
			/* Only the last short option can take a value */
			if(_argparse_short_option_expects_value(argparse_context, arg[i]) && i != arglen - 1) {
				if(!(argparse_context->flags & _kARGPARSE_HAS_CATCHALL)) {
					if(f != NULL) {
						fprintf(f,
							"Error: In argument \"%s\", option '-%c' expects a value and therefore"
							" must be the last character.\n",
							arg, arg[i]
						);
					}
					ret = _kARG_VALUE_ERROR;
				}
				goto parse_done;
			}
		}
		
		/* Store current argument character position in argvalue.val_string */
		argparse_context->argtype = _kARG_TYPE_SHORTGROUP;
		argparse_context->argvalue.val_string = &arg[2];
		
		/* Look up short option character */
		arginfo = _argparse_find_shortarg(argparse_context, arg[1]);
		
		/* Shouldn't be possible as we just ensured that all characters are registered short options */
		assert(arginfo != NULL);
		goto parse_done;
	}
	
parse_done:
	if(arginfo != NULL) {
		ret = arginfo->arg_id;
	}
	
	/* Pass positional arguments to the ARG_OTHER handler if there's no ARG_POSITIONAL handler */
	if(ret == _kARG_VALUE_POSITIONAL) {
		if(!argparse_context->positional_usage) {
			ret = _kARG_VALUE_OTHER;
		}
	}
	
	/* Did we fail to parse this argument? */
	if(ret == _kARG_VALUE_OTHER) {
		/* Error out unless there's an ARG_OTHER catchall present */
		if(argparse_context->flags & _kARGPARSE_HAS_CATCHALL) {
			goto out;
		}
		
		if(f != NULL) {
			fprintf(f, "Error: Unexpected argument: \"%s\"\n", arg);
		}
		ret = _kARG_VALUE_ERROR;
		goto out;
	}
	else if(arginfo != NULL) {
		/* We found a matching argument, now check if it has a non-void type */
		if(arginfo->type == _kARG_TYPE_VOID) {
			/* Argument shouldn't have a value, so ensure that it doesn't */
			if(argval_str) {
				if(argparse_context->flags & _kARGPARSE_HAS_CATCHALL) {
					ret = _kARG_VALUE_OTHER;
					goto out;
				}
				
				if(f != NULL) {
					fprintf(f, "Error: Argument \"%s\" has an embedded value but doesn't expect any value.\n", arg);
				}
				ret = _kARG_VALUE_ERROR;
				goto out;
			}
		}
		else if(arginfo->type != _kARG_TYPE_COMMAND) {
			/* Get argument value string */
			if(!argval_str) {
				/* This argument expects a value as the next argument like --test foo */
				argval_str = _argparse_next(argparse_context);
				if(!argval_str) {
					/* No more arguments, so this is an error */
					if(f != NULL) {
						fprintf(f, "Error: Argument \"%s\" needs a value but there are no more arguments.\n", arg);
					}
					ret = _kARG_VALUE_ERROR;
					goto out;
				}
			}
			
			switch(arginfo->type) {
				case _kARG_TYPE_LONG: {
					/* Parse the string as an integer with strtol() to support negative ints and other bases */
					char* str_end = NULL;
					long val = strtol(argval_str, &str_end, 0);
					if(*str_end != '\0') {
						/* Failed to fully parse argument value string */
						if(f != NULL) {
							fprintf(f, "Error: ");
							if(arginfo->long_name) {
								fprintf(f, "The %s%s", argparse_context->long_arg_prefix, arginfo->long_name);
							}
							else {
								fprintf(f, "The -%c", arginfo->short_name);
							}
							fprintf(f, " option expects an integral value, not \"%s\".\n", argval_str);
						}
						ret = _kARG_VALUE_ERROR;
						goto out;
					}
					
					/* Store argument type and value in argparse context */
					argparse_context->argtype = _kARG_TYPE_LONG;
					argparse_context->argvalue.val_long = val;
					break;
				}
				
				case _kARG_TYPE_STRING:
					/* Store argument type and value in argparse context */
					argparse_context->argtype = _kARG_TYPE_STRING;
					argparse_context->argvalue.val_string = argval_str;
					break;
				
				default:
					argparse_assert(false);
					break;
			}
		}
	}
	
out:
#ifndef NDEBUG
	if((argparse_context->flags & _kARGPARSE_DEBUG) && f != NULL) {
		int aidx = *argparse_context->argidx - (argparse_context->argtype != _kARG_TYPE_SHORTGROUP);
		
		if(arg) {
			fprintf(f, "\"%s\": ", arg);
		}
		
		fprintf(f, "%s", _argparse_state_name(argparse_context, state));
		fprintf(f, " -> ");
		fprintf(f, "%s", _argparse_state_name(argparse_context, ret));
		fprintf(f, ", argidx=%d", aidx);
		fprintf(f, "\n");
	}
#endif /* NDEBUG */
	
	argparse_context->cur_arg = arginfo;
	argparse_context->state = ret;
}

/* Recursively print cmd of each argparse context, top-down */
static void _argparse_help_cmd(const struct kjc_argparse* ctx, FILE* f) {
	if(!ctx->parent) {
		/* Root context: get cmd from basename of argv[0] */
		const char* cmd = ctx->orig_argv[0];
		const char* last_slash = strrchr(cmd, '/');
		if(last_slash) {
			cmd = last_slash + 1;
		}
		
		fprintf(f, " %s", cmd);
		return;
	}
	
	/* Print parent commands first, so order comes naturally */
	_argparse_help_cmd(ctx->parent, f);
	
	/* Get cmd from subcommand name used to call into this argparse context */
	fprintf(f, " %s", ctx->parent->cur_arg->long_name);
}

static void _argparse_help_usage(const struct kjc_argparse* argparse_context, FILE* f) {
	if(argparse_context->custom_usage) {
		fprintf(f, "%s\n", argparse_context->custom_usage);
		return;
	}
	
	/* Print usage header with command used to reach this argparse context */
	fprintf(f, "Usage:");
	_argparse_help_cmd(argparse_context, f);
	
	if(argparse_context->shortargs_count > 0) {
		/* Print all available short options (already sorted) */
		fprintf(f, " [-");
		
		struct _arginfo** shortargs = _argparse_get_shortargs(argparse_context);
		for(unsigned i = 0; i < argparse_context->shortargs_count; i++) {
			fprintf(f, "%c", shortargs[i]->short_name);
		}
		
		fprintf(f, "]");
	}
	
	if(argparse_context->longargs_count > 0) {
		fprintf(f, " [OPTIONS]");
	}
	
	/* Print usage description of positional arguments */
	if(argparse_context->positional_usage != NULL) {
		fprintf(f, " %s", argparse_context->positional_usage);
	}
	
	/* Print usage description for subcommand */
	if(argparse_context->subcmds_count) {
		fprintf(f, " COMMAND ...");
	}
	
	fprintf(f, "\n");
}

static unsigned _argparse_get_subcmd_description_column(const struct kjc_argparse* argparse_context) {
	if(argparse_context->subcmd_description_column >= 0) {
		return argparse_context->subcmd_description_column;
	}
	
	return 0
		+ argparse_context->indent
		+ argparse_context->subcmd_width
		+ argparse_context->description_padding
		;
}

static void _argparse_help_subcmds(const struct kjc_argparse* argparse_context, FILE* f) {
	struct _arginfo** subcmds = _argparse_get_subcmds(argparse_context);
	bool work_to_do = false;
	
	for(unsigned i = 0; i < argparse_context->subcmds_count; i++) {
		if(subcmds[i]->description != NULL) {
			work_to_do = true;
		}
	}
	
	if(!work_to_do) {
		return;
	}
	
	unsigned descStart = _argparse_get_subcmd_description_column(argparse_context);
	
	fprintf(f, "\n");
	fprintf(f, "Commands:\n");
	
	/* Print description of each command */
	for(unsigned i = 0; i < argparse_context->subcmds_count; i++) {
		/* Don't print help for subcommands without a description */
		if(!subcmds[i]->description) {
			continue;
		}
		
		unsigned col = 0;
		col += fprintf(f, "%*s", argparse_context->indent, "");
		
		/* Print command name */
		col += fprintf(f, "%s", subcmds[i]->long_name);
		
		/* Seek to description column */
		if(col + 2 > descStart) {
			/* Description will go on next line */
			fprintf(f, "\n");
			col = 0;
		}
		
		/* Pad with spaces until reaching description column */
		fprintf(f, "%*s", descStart - col, "");
		
		/* Print command description */
		fprintf(f, "%s\n", subcmds[i]->description);
	}
}

static unsigned _argparse_get_description_column(const struct kjc_argparse* argparse_context) {
	if(argparse_context->description_column >= 0) {
		return argparse_context->description_column;
	}
	
	return 0
		+ argparse_context->indent
		+ 2 /* Short option */
		+ 2 /* Comma and space between short and long options */
		+ argparse_context->long_prefix_len /* "--" before long option */
		+ argparse_context->long_name_width
		+ argparse_context->description_padding
		;
}

static void _argparse_help_options(const struct kjc_argparse* argparse_context, FILE* f) {
	struct _arginfo* argstorage = _argparse_get_argstorage(argparse_context);
	bool work_to_do = false;
	
	for(unsigned i = 0; i < argparse_context->argstorage_count; i++) {
		struct _arginfo* pcur = &argstorage[i];
		if(pcur->type != _kARG_TYPE_COMMAND && pcur->description != NULL) {
			work_to_do = true;
			break;
		}
	}
	
	if(!work_to_do) {
		return;
	}
	
	unsigned descStart = _argparse_get_description_column(argparse_context);
	
	fprintf(f, "\n");
	fprintf(f, "Options:\n");
	
	/* Print description of each argument */
	for(unsigned i = 0; i < argparse_context->argstorage_count; i++) {
		struct _arginfo* pcur = &argstorage[i];
		const char* value_hint = _arginfo_value_hint(argparse_context, pcur);
		unsigned col = 0;
		
		/* Don't print help info for options without a description or subcommands */
		if(!pcur->description || pcur->type == _kARG_TYPE_COMMAND) {
			continue;
		}
		
		col += fprintf(f, "%*s", argparse_context->indent, "");
		
		/* Print short option (if set) */
		if(pcur->short_name != '\0') {
			col += fprintf(f, "-%c", pcur->short_name);
			if(!pcur->long_name && value_hint != NULL) {
				col += fprintf(f, " <%s>", value_hint);
			}
		}
		else {
			col += fprintf(f, "  ");
		}
		
		/* Print long option (if set) */
		if(pcur->long_name != NULL) {
			/* If short option was set, separate it with a comma */
			if(pcur->short_name != '\0') {
				col += fprintf(f, ", ");
			}
			else {
				col += fprintf(f, "  ");
			}
			
			/* Print long option */
			col += fprintf(f, "%s%s", argparse_context->long_arg_prefix, pcur->long_name);
			if(value_hint != NULL) {
				col += fprintf(f, " <%s>", value_hint);
			}
		}
		
		/* Seek to description column */
		if(col + 2 > descStart) {
			/* Description will go on next line */
			fprintf(f, "\n");
			col = 0;
		}
		
		/* Pad with spaces until reaching description column */
		fprintf(f, "%*s", descStart - col, "");
		
		/* Print the argument's type if not void */
		if(argparse_context->flags & _kARGPARSE_TYPE_HINTS) {
			switch(pcur->type) {
				case _kARG_TYPE_LONG:
					fprintf(f, "[int] ");
					break;
				
				case _kARG_TYPE_STRING:
					fprintf(f, "[string] ");
					break;
			}
		}
		
		/* Print the description and end the line */
		fprintf(f, "%s\n", pcur->description);
	}
}

static void _argparse_help_suffix(const struct kjc_argparse* argparse_context, FILE* f) {
	if(!argparse_context->custom_suffix) {
		return;
	}
	
	fprintf(f, "\n");
	fprintf(f, "%s\n", argparse_context->custom_suffix);
}

void _argparse_help(const struct kjc_argparse* argparse_context) {
	FILE* f = argparse_context->stream;
	if(!f) {
		return;
	}
	
	_argparse_help_usage(argparse_context, f);
	_argparse_help_subcmds(argparse_context, f);
	_argparse_help_options(argparse_context, f);
	_argparse_help_suffix(argparse_context, f);
}

char* _argparse_next(struct kjc_argparse* argparse_context) {
	if(*argparse_context->argidx >= argparse_context->orig_argc) {
		return NULL;
	}
	
	return argparse_context->orig_argv[(*argparse_context->argidx)++];
}

long _argparse_value_long(const struct kjc_argparse* argparse_context) {
	argparse_assert(argparse_context->argtype == _kARG_TYPE_LONG);
	return argparse_context->argvalue.val_long;
}

const char* _argparse_value_string(const struct kjc_argparse* argparse_context) {
	argparse_assert(argparse_context->argtype == _kARG_TYPE_STRING);
	return argparse_context->argvalue.val_string;
}
