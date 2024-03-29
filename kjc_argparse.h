//
//  kjc_argparse.h
//
//  Created by Kevin Colley on 4/30/17.
//  Copyright © 2017 Kevin Colley. All rights reserved.
//

#ifndef KJC_ARGPARSE_H
#define KJC_ARGPARSE_H

/*
 * Public API of kjc_argparse:
 *
 * Top-level argparse blocks:
 * - ARGPARSE(int argc, char** argv) { argparse body } - Parse all arguments
 * - ARGPARSE_RESUME(struct kjc_argparse* parent_context) { argparse body } - Parse all arguments in a new sub-context
 * - ARGPARSE_NESTED {argparse body} - Parse all arguments, under a parent ARGPARSE/ARGPARSE_RESUME block
 *
 * Arg handlers (in argparse block body):
 * - ARG(char shortarg, const char* longarg, const char* help) { arg handler } - Arg with no associated value
 * - ARG_INT(char shortarg, const char* longarg, const char* help, name) { arg handler } - Arg with an int value
 * - ARG_LONG(char shortarg, const char* longarg, const char* help, name) { arg handler } - Arg with a long value
 * - ARG_STRING(char shortarg, const char* longarg, const char* help, name) { arg handler } - Arg with a string value
 * - ARG_COMMAND(const char* cmd, const char* help) { arg handler } - Named subcommand with its own argument parsing
 * - ARG_POSITIONAL(const char* help, name) { arg handler } - Handles any unhandled arguments
 * - ARG_OTHER(name) { arg handler } - Handles any unhandled arguments
 * - ARG_END { arg handler } - Runs after argparse ends
 *
 * Argparse configuration (in argparse block body):
 * - ARGPARSE_CONFIG_STREAM(FILE* output_fp); - Set output stream used for argparse messages (like ARGPARSE_HELP())
 * - ARGPARSE_CONFIG_CUSTOM_USAGE(const char* usage); - Set custom usage text for help output
 * - ARGPARSE_CONFIG_HELP_SUFFIX(const char* suffix); - Set custom text to appear at the end of help output
 * - ARGPARSE_CONFIG_COMMAND_DESCRIPTION_COLUMN(int column); Set column at which command descriptions will be printed
 * - ARGPARSE_CONFIG_DESCRIPTION_COLUMN(int column); - Set column at which arg descriptions will be printed
 * - ARGPARSE_CONFIG_INDENT(int column); - Set column at which commands and options begin printing
 * - ARGPARSE_CONFIG_USE_VARNAMES(bool val); - Set whether variable names (or type names) should be used in help text
 * - ARGPARSE_CONFIG_TYPE_HINTS(bool val); - Set whether type hints are printed in option descriptions
 * - ARGPARSE_CONFIG_DESCRIPTION_PADDING(int padding); - Set minimum number of spaces before options' descriptions
 * - ARGPARSE_CONFIG_SHORTGROUPS(bool enable); - True to enable support for multiple short options in a single argument
 * - ARGPARSE_CONFIG_AUTO_HELP(bool enable); - True to automatically support "--help"
 * - ARGPARSE_CONFIG_DASHDASH(bool enable); - True to treat everything after "--" as ARG_POSITIONAL
 * - ARGPARSE_CONFIG_LONG_PREFIX(const char* prefix); - String used as the prefix for long options, "--" by default
 * - ARGPARSE_CONFIG_DEBUG(bool debug); - Print internal argparse debug information
 *
 * Argparse functions (only valid within an arg handler)
 * - void ARGPARSE_HELP() - Print help usage message to configured output stream
 * - int ARGPARSE_INDEX() - Get index of current argument
 * - char* ARGPARSE_NEXT() - Take the next argument, or NULL if there are no more
 * - void ARGPARSE_REWIND(int count) - Rewinds the argparse index by the given amount
 *
 * For usage instructions, refer to full_example.c and other example programs
 */

#ifdef __cplusplus
extern "C" {
#endif

#ifdef __GNUC__
/* Dangling else syntax is required to achieve this argparse syntax */
#pragma GCC diagnostic ignored "-Wdangling-else"
#endif

#ifndef UNIQUIFY
#ifdef __COUNTER__
#define UNIQUIFY(macro, ...) UNIQUIFY_(macro, __COUNTER__, ##__VA_ARGS__)
#else
#define UNIQUIFY(macro, ...) UNIQUIFY_(macro, __LINE__, ##__VA_ARGS__)
#endif
#define UNIQUIFY_(macro, id, ...) macro(id, ##__VA_ARGS__)
#endif

#ifndef STRINGIFY
#define STRINGIFY(x) STRINGIFY_(x)
#define STRINGIFY_(x) #x
#endif


struct kjc_argparse;

/* ARGPARSE(int argc, char** argv) { argparse body } - Parse all arguments */
#define ARGPARSE(argc, argv)                                                                                          \
	_argparse_setup()                                                                                                 \
	/* Setup root argparse context with argc and argv */                                                              \
	_argparse_stmt(struct kjc_argparse _argparse_context = {0})                                                       \
	_argparse_stmt(_argparse_context.orig_argc = (argc), _argparse_context.orig_argv = (argv))                        \
	_argparse_top()

/* ARGPARSE_RESUME(struct kjc_argparse* parent_context) { argparse body } - Parse all arguments in a new sub-context */
#define ARGPARSE_RESUME(parent_context)                                                                               \
	_argparse_setup()                                                                                                 \
	/* Setup child argparse context with parent pointer */                                                            \
	_argparse_stmt(struct kjc_argparse _argparse_context = {0})                                                       \
	_argparse_stmt(_argparse_context.parent = (parent_context))                                                       \
	_argparse_top()

#define _argparse_setup() _argparse_setup_(_top)
#define _argparse_setup_(id) _argparse_stmt_(id, int _argparse_once##id = 1)
#define _argparse_stmt(...) _argparse_stmt_(_top, ##__VA_ARGS__)
#define _argparse_stmt_(id, ...) for(__VA_ARGS__; _argparse_once##id; _argparse_once##id = 0)
#define _argparse_enter_exit(enter_expr, exit_expr) _argparse_enter_exit_(_top, enter_expr, exit_expr)
#define _argparse_enter_exit_(id, enter_expr, exit_expr)                                                              \
	for(enter_expr; _argparse_once##id; _argparse_once##id = 0, exit_expr)

#define _argparse_block()                                                                                             \
	/* Jump table based on the generated argument ID to select an argument handler */                                 \
	/* For the count and initialization phases, we instead jump to the beginning of the code block */                 \
	/* Trailing statement after this macro invocation will attach to this switch statement! */                        \
	switch(_argparse_pcontext->state)                                                                                 \
		if(0) {                                                                                                       \
		case _kARG_VALUE_HELP:                                                                                        \
			ARGPARSE_HELP();                                                                                          \
			_argparse_pcontext->state = _kARG_VALUE_BREAK;                                                            \
			break;                                                                                                    \
		} else                                                                                                        \
		case _kARG_VALUE_COUNT:                                                                                       \
		case _kARG_VALUE_INIT:

#define _argparse_top()                                                                                               \
	/* Keep a pointer to the "current" argparse context, which may be changed for subcommands */                      \
	_argparse_stmt(struct kjc_argparse* _argparse_pcontext = &_argparse_context)                                      \
	_argparse_loop()

/* ARGPARSE_NESTED {argparse body} - Parse all arguments, under a parent ARGPARSE/ARGPARSE_RESUME block */
#define ARGPARSE_NESTED                                                                                               \
	UNIQUIFY(_argparse_nested)
#define _argparse_nested(id)                                                                                          \
	/* Setup _argparse_once##id to allow non-looping for loops (for declaring scope-local variables) */               \
	_argparse_setup_(id)                                                                                              \
	/* Setup nested argparse info structure */                                                                        \
	_argparse_stmt_(id, struct kjc_argparse _argparse_context##id = {0})                                              \
	/* Keep a pointer to the "parent" argparse context to restore _argparse_pcontext later */                         \
	_argparse_stmt_(id, _argparse_context##id.parent = _argparse_pcontext)                                            \
	/* Update _argparse_pcontext pointer to this subcommand's context, then restore it afterwards */                  \
	_argparse_enter_exit_(id,                                                                                         \
		_argparse_pcontext = &_argparse_context##id,                                                                  \
		_argparse_pcontext = _argparse_pcontext->parent                                                               \
	)                                                                                                                 \
	_argparse_loop()

/*
 * Actual argument parsing loop, first iteration is counting phase, then initialization phase, then
 * after that, each iteration is for parsing one option.
 */
#define _argparse_loop()                                                                                              \
	for(_argparse_init(_argparse_pcontext); !_argparse_done(_argparse_pcontext); _argparse_parse(_argparse_pcontext)) \
		_argparse_block()

#define _arg_handler(id, ...)                                                                                         \
	/* Set up _arg_loop to determine when this outer loop has run at least once. */                                   \
	/* Also set up _arg_break, which is only set to zero when the inner loop's update */                              \
	/* expression runs. This means that if the inner loop's update expression is skipped by */                        \
	/* use of the break keyword within that loop, then _arg_break will still be 1. */                                 \
	for(int _arg_loop##id = 0, _arg_break##id = 1; ; ++_arg_loop##id)                                                 \
		if(_arg_loop##id == 1) {                                                                                      \
			/* We already ran the argument handler body */                                                            \
			if(_arg_break##id && _argparse_pcontext->state != _kARG_VALUE_END) {                                      \
				/* The argument handler was escaped via the break keyword, so break out of the argparse loop */       \
				_argparse_pcontext->state = _kARG_VALUE_BREAK;                                                        \
				break;                                                                                                \
			}                                                                                                         \
			else {                                                                                                    \
				/* Argument handler block executed normally */                                                        \
				/* Only break out of the above loop that defines _arg_loop */                                         \
				break;                                                                                                \
			}                                                                                                         \
		}                                                                                                             \
		else                                                                                                          \
			/* First loop, so run the argument handler and check if it ends normally or breaks out early */           \
			/* Trailing statement after this macro invocation will attach to this for statement! */                   \
			for(__VA_ARGS__; _arg_break##id; _arg_break##id = 0)

#define _arg_custom_helper(short_name, long_name, description, type, varname, handler, ...)                           \
	UNIQUIFY(_arg_custom_helper_, short_name, long_name, description, type, varname, handler, ##__VA_ARGS__)

/* Ensure that the argument ID won't collide with any "special" _kARG_VALUE_* values (even if value is 0) */
#define _arg_make_id(value) (((value) << 1) | 1)

#define _arg_custom_helper_(id, short_name, long_name, description, type, varname, handler, ...)                      \
	if(_argparse_pcontext->state == _kARG_VALUE_COUNT) {                                                              \
		/* Count phase: increment the count of arguments to be registered during the initialization phase */          \
		++_argparse_pcontext->argstorage_cap;                                                                         \
		/* For options with both a short name and long name, we double count them */                                  \
		if(short_name != '\0') {                                                                                      \
			++_argparse_pcontext->shortargs_cap;                                                                      \
		}                                                                                                             \
		if(long_name) {                                                                                               \
			if(type == _kARG_TYPE_COMMAND) {                                                                          \
				++_argparse_pcontext->subcmds_cap;                                                                    \
			}                                                                                                         \
			else {                                                                                                    \
				++_argparse_pcontext->longargs_cap;                                                                   \
			}                                                                                                         \
		}                                                                                                             \
	}                                                                                                                 \
	else if(_argparse_pcontext->state == _kARG_VALUE_INIT) {                                                          \
		/* Initialization phase: register this argument's info in the _argparse_context struct */                     \
		_argparse_add(_argparse_pcontext, _arg_make_id(id), short_name, long_name, description, type, varname);       \
	}                                                                                                                 \
	/* Code inside is only accessible via jumptable from switch statement in _argparse_block(), NOT initialization */ \
	else if(0)                                                                                                        \
		case _arg_make_id(id):                                                                                        \
			/* Trailing statement after this macro invocation will be the argument handler body. */                   \
			/* Keywords like break and continue will work as expected, but return will leak memory */                 \
			handler(id, ##__VA_ARGS__)

#define _arg_helper(short_name, long_name, description, type, varname, ...)                                           \
	_arg_custom_helper(short_name, long_name, description, type, varname, _arg_handler, ##__VA_ARGS__)

/* ARG(char shortarg, const char* longarg, const char* help) { arg handler } - Arg with no associated value */
#define ARG(short_name, long_name, description)                                                                       \
	_arg_helper(short_name, long_name, description, _kARG_TYPE_VOID, (const char*)0)

#define _arg_long_helper(short_name, long_name, description, var_type, var)                                           \
	_arg_helper(short_name, long_name, description, _kARG_TYPE_LONG, STRINGIFY(var),                                  \
		var_type var = (var_type)_argparse_value_long(_argparse_pcontext)                                             \
	)

/* ARG_INT(char shortarg, const char* longarg, const char* help, name) { arg handler } - Arg with an int value */
#define ARG_INT(short_name, long_name, description, var)                                                              \
	_arg_long_helper(short_name, long_name, description, int, var)

/* ARG_LONG(char shortarg, const char* longarg, const char* help, name) { arg handler } - Arg with a long value */
#define ARG_LONG(short_name, long_name, description, var)                                                             \
	_arg_long_helper(short_name, long_name, description, long, var)

/* ARG_STRING(char shortarg, const char* longarg, const char* help, name) { arg handler } - Arg with a string value */
#define ARG_STRING(short_name, long_name, description, var)                                                           \
	_arg_helper(short_name, long_name, description, _kARG_TYPE_STRING, STRINGIFY(var),                                \
		const char* var = _argparse_value_string(_argparse_pcontext)                                                  \
	)

/* ARG_COMMAND(const char* cmd, const char* help) { arg handler } - Named subcommand with its own argument parsing */
#define ARG_COMMAND(name, description)                                                                                \
	_arg_helper(0, name, description, _kARG_TYPE_COMMAND, (const char*)0)


/* ARG_POSITIONAL(const char* help, name) { arg handler } - Handles any positional arguments */
#define ARG_POSITIONAL(usage, var)                                                                                    \
	_arg_other_helper(var, 0, usage, _kARG_VALUE_POSITIONAL)

/* ARG_OTHER(var) { arg handler } - Handles any unhandled arguments */
#define ARG_OTHER(var)                                                                                                \
	_arg_other_helper(var, 1, (const char*)0, _kARG_VALUE_OTHER)

#define _arg_other_helper(var, is_other, usage, argval)                                                               \
	UNIQUIFY(_arg_other_helper_, var, is_other, usage, argval)

#define _arg_other_helper_(id, var, is_other, usage, argval)                                                          \
	if(_argparse_pcontext->state == _kARG_VALUE_INIT) {                                                               \
		/* Initialization phase: mark the existence of an ARG_POSITIONAL block in the _argparse_context struct */     \
		if(is_other) {                                                                                                \
			_argparse_pcontext->flags |= _kARGPARSE_HAS_CATCHALL;                                                     \
		}                                                                                                             \
		if(usage) {                                                                                                   \
			_argparse_pcontext->positional_usage = (usage);                                                           \
		}                                                                                                             \
	}                                                                                                                 \
	/* Code inside is only accessible via jumptable from switch statement in _argparse_block(), NOT initialization */ \
	else if(0)                                                                                                        \
		case argval:                                                                                                  \
			/* Trailing statement after this macro invocation will be the argument handler body. */                   \
			/* Keywords like break and continue will work as expected, but return will leak memory */                 \
			_arg_handler(id, const char* var = _argparse_pcontext->orig_argv[*_argparse_pcontext->argidx - 1])

/* ARG_END { arg handler } - Runs after argparse ends */
#define ARG_END                                                                                                       \
	UNIQUIFY(_arg_end_helper)

#define _arg_end_helper(id)                                                                                           \
	/* Code inside is only accessible via jumptable from switch statement in _argparse_block(), NOT initialization */ \
	if(0)                                                                                                             \
		case _kARG_VALUE_END:                                                                                         \
			/* Trailing statement after this macro invocation will be the argument handler body. */                   \
			/* Keywords like break and continue will work as expected, but return will leak memory */                 \
			_arg_handler(id)


#define _argparse_config_helper(field, value) do {                                                                    \
	if(_argparse_pcontext->state == _kARG_VALUE_INIT) {                                                               \
		_argparse_pcontext->field = (value);                                                                          \
	}                                                                                                                 \
} while(0)

#define _argparse_config_flag(flag, value)                                                                            \
	_argparse_config_helper(flags, (_argparse_pcontext->flags & ~(flag)) | (-!!(value) & (flag)))

/* ARGPARSE_CONFIG_STREAM(FILE* output_fp); - Set output stream used for argparse messages (like ARGPARSE_HELP()) */
#define ARGPARSE_CONFIG_STREAM(fp) _argparse_config_helper(stream, fp)
#ifndef ARGPARSE_DEFAULT_STREAM
#define ARGPARSE_DEFAULT_STREAM ((FILE*)1)
#endif

/* ARGPARSE_CONFIG_CUSTOM_USAGE(const char* usage); - Set custom usage text for help output */
#define ARGPARSE_CONFIG_CUSTOM_USAGE(usage) _argparse_config_helper(custom_usage, usage)
#ifndef ARGPARSE_DEFAULT_CUSTOM_USAGE
#define ARGPARSE_DEFAULT_CUSTOM_USAGE ((const char*)0)
#endif

/* ARGPARSE_CONFIG_HELP_SUFFIX(const char* suffix); - Set custom text to appear at the end of help output */
#define ARGPARSE_CONFIG_HELP_SUFFIX(suffix) _argparse_config_helper(custom_suffix, suffix)
#ifndef ARGPARSE_DEFAULT_HELP_SUFFIX
#define ARGPARSE_DEFAULT_HELP_SUFFIX ((const char*)0)
#endif

/* ARGPARSE_CONFIG_COMMAND_DESCRIPTION_COLUMN(int column); Set column at which command descriptions will be printed */
#define ARGPARSE_CONFIG_COMMAND_DESCRIPTION_COLUMN(column) _argparse_config_helper(subcmd_description_column, column)
#ifndef ARGPARSE_DEFAULT_COMMAND_DESCRIPTION_COLUMN
#define ARGPARSE_DEFAULT_COMMAND_DESCRIPTION_COLUMN -1
#endif

/* ARGPARSE_CONFIG_DESCRIPTION_COLUMN(int column); - Set column at which arg descriptions will be printed */
#define ARGPARSE_CONFIG_DESCRIPTION_COLUMN(column) _argparse_config_helper(description_column, column)
#ifndef ARGPARSE_DEFAULT_DESCRIPTION_COLUMN
#define ARGPARSE_DEFAULT_DESCRIPTION_COLUMN -1
#endif

/* ARGPARSE_CONFIG_INDENT(int column); - Set column at which commands and options begin printing */
#define ARGPARSE_CONFIG_INDENT(column) _argparse_config_helper(indent, column)
#ifndef ARGPARSE_DEFAULT_INDENT
#define ARGPARSE_DEFAULT_INDENT 2
#endif

/* ARGPARSE_CONFIG_USE_VARNAMES(bool val); - Set whether variable names (or type names) should be used in help text */
#define ARGPARSE_CONFIG_USE_VARNAMES(val) _argparse_config_flag(_kARGPARSE_USE_VARNAMES, val)
#ifndef ARGPARSE_DEFAULT_USE_VARNAMES
#define ARGPARSE_DEFAULT_USE_VARNAMES 1
#endif

/* ARGPARSE_CONFIG_TYPE_HINTS(bool val); - Set whether type hints are printed in option descriptions */
#define ARGPARSE_CONFIG_TYPE_HINTS(val) _argparse_config_flag(_kARGPARSE_TYPE_HINTS, val)
#ifndef ARGPARSE_DEFAULT_TYPE_HINTS
#define ARGPARSE_DEFAULT_TYPE_HINTS 0
#endif

/* ARGPARSE_CONFIG_DESCRIPTION_PADDING(int padding); - Set minimum number of spaces before options' descriptions */
#define ARGPARSE_CONFIG_DESCRIPTION_PADDING(padding) _argparse_config_helper(description_padding, padding)
#ifndef ARGPARSE_DEFAULT_DESCRIPTION_PADDING
#define ARGPARSE_DEFAULT_DESCRIPTION_PADDING 3
#endif

/* ARGPARSE_CONFIG_SHORTGROUPS(bool enable); - True to enable support for multiple short options in a single argument */
#define ARGPARSE_CONFIG_SHORTGROUPS(enable) _argparse_config_flag(_kARGPARSE_WITH_SHORTGROUPS, enable)
#ifndef ARGPARSE_DEFAULT_SHORTGROUPS
#define ARGPARSE_DEFAULT_SHORTGROUPS 1
#endif

/* ARGPARSE_CONFIG_AUTO_HELP(bool enable); - True to automatically support "--help" */
#define ARGPARSE_CONFIG_AUTO_HELP(enable) _argparse_config_flag(_kARGPARSE_AUTO_HELP, enable)
#ifndef ARGPARSE_DEFAULT_AUTO_HELP
#define ARGPARSE_DEFAULT_AUTO_HELP 1
#endif

/* ARGPARSE_CONFIG_DASHDASH(bool enable); - True to treat everything after "--" as ARG_POSITIONAL */
#define ARGPARSE_CONFIG_DASHDASH(enable) _argparse_config_flag(_kARGPARSE_DASHDASH, enable)
#ifndef ARGPARSE_DEFAULT_DASHDASH
#define ARGPARSE_DEFAULT_DASHDASH 1
#endif

/* ARGPARSE_CONFIG_LONG_PREFIX(const char* prefix); - String used as the prefix for long options, "--" by default */
#define ARGPARSE_CONFIG_LONG_PREFIX(prefix) _argparse_config_helper(long_arg_prefix, prefix)
#ifndef ARGPARSE_DEFAULT_LONG_PREFIX
#define ARGPARSE_DEFAULT_LONG_PREFIX "--"
#endif

#ifndef NDEBUG
/* ARGPARSE_CONFIG_DEBUG(bool debug); - Print internal argparse debug information */
#define ARGPARSE_CONFIG_DEBUG(debug) _argparse_config_flag(_kARGPARSE_DEBUG, debug)
#else /* NDEBUG */
#define ARGPARSE_CONFIG_DEBUG(debug) do {} while(0)
#endif /* NDEBUG */


/* void ARGPARSE_HELP() - Print help usage message to configured output stream */
#define ARGPARSE_HELP() _argparse_help(_argparse_pcontext)

/* int ARGPARSE_INDEX() - Get index of current argument */
#define ARGPARSE_INDEX() (*_argparse_pcontext->argidx - (_argparse_pcontext->argtype != _kARG_TYPE_SHORTGROUP))

/* char* ARGPARSE_NEXT() - Take the next argument, or NULL if there are no more */
#define ARGPARSE_NEXT() _argparse_next(_argparse_pcontext)

/* void ARGPARSE_REWIND(int count) - Rewinds the argparse index by the given amount */
#define ARGPARSE_REWIND(count) do { *_argparse_pcontext->argidx -= (count); } while(0)

/* void* ARGPARSE_GET_CONTEXT() - Get a pointer to the argparse context (to pass to a function) */
#define ARGPARSE_GET_CONTEXT() _argparse_pcontext


/*
 * Everything below this line is considered PRIVATE API - DO NOT USE.
 */


/* "Special" values for the argument ID, stored in argparse_context.state */
#define _kARG_VALUE_COUNT      (0 << 1)  /* Set only during the first iteration, aka the counting phase */
#define _kARG_VALUE_INIT       (1 << 1)  /* Set only during the second iteration, aka the initialization phase */
#define _kARG_VALUE_POSITIONAL (2 << 1)  /* Set when the argument doesn't start with '-' */
#define _kARG_VALUE_OTHER      (3 << 1)  /* Set when the argument being parsed doesn't match a declared argument */
#define _kARG_VALUE_END        (4 << 1)  /* Set when all arguments have been parsed */
#define _kARG_VALUE_BREAK      (5 << 1)  /* Set when the break keyword is used from an argument handler */
#define _kARG_VALUE_ERROR      (6 << 1)  /* Set when argparse internally encounters an error for some reason */
#define _kARG_VALUE_HELP       (7 << 1)  /* Set when the automatic "--help" handler should run */

/* Intentionally not using an enum so the underlying type doesn't have to be int */
#define _kARG_TYPE_VOID        0
#define _kARG_TYPE_STRING      1
#define _kARG_TYPE_LONG        2
#define _kARG_TYPE_SHORTGROUP  3
#define _kARG_TYPE_COMMAND     4
#define _kARG_TYPE_DASHDASH    5

/* Configurable flags for argparse */
#define _kARGPARSE_HAS_CATCHALL      (1 << 0)
#define _kARGPARSE_USE_VARNAMES      (1 << 1)
#define _kARGPARSE_TYPE_HINTS        (1 << 2)
#define _kARGPARSE_WITH_SHORTGROUPS  (1 << 3)
#define _kARGPARSE_DEBUG             (1 << 4)
#define _kARGPARSE_AUTO_HELP         (1 << 5)
#define _kARGPARSE_DASHDASH          (1 << 6)

/* Also stored in flags but not configurable */
#define _kARGPARSE_FLAG_DONE         (1 << 7)


/* Fields have been hand-packed, hence the weird ordering */
struct _arginfo {
	const char* long_name;
	const char* description;
	const char* var_name;
	int arg_id;
	unsigned char type;
	char short_name;
};

/* Fields have been hand-packed, hence the weird ordering */
struct kjc_argparse {
	struct kjc_argparse* parent;
	int* argidx;
	void* stream;
	const char* custom_usage;
	const char* custom_suffix;
	const char* long_arg_prefix;
	const char* positional_usage;
	struct _arginfo* cur_arg;
	union {
		const char* val_string;
		long val_long;
	} argvalue;
	void* argbuffer;
	char** orig_argv;
	int orig_argc;
	int argidx_top;
	int state;
	unsigned argstorage_cap;
	unsigned argstorage_count;
	unsigned subcmds_cap;
	unsigned subcmds_count;
	unsigned longargs_cap;
	unsigned longargs_count;
	unsigned shortargs_cap;
	unsigned shortargs_count;
	unsigned indent;
	unsigned long_prefix_len;
	int subcmd_description_column;
	int description_column;
	unsigned description_padding;
	unsigned subcmd_width;
	unsigned long_name_width;
	unsigned char short_bitmap[32];
	unsigned char short_value_bitmap[32];
	unsigned char argtype;
	unsigned char flags;
};


/* Initializes the argparse context structure and returns the initial argparse state (_kARG_VALUE_COUNT) */
void _argparse_init(struct kjc_argparse* argparse_context);

/* Register an argument with the argparse context struct */
void _argparse_add(
	struct kjc_argparse* argparse_context,
	int arg_id,
	char short_name,
	const char* long_name,
	const char* description,
	unsigned char type,
	const char* var_name
);

/* Returns nonzero if argument parsing should stop */
int _argparse_done(const struct kjc_argparse* argparse_context);

/* Advance the argparse state machine, usually by parsing an argument */
void _argparse_parse(struct kjc_argparse* argparse_context);

/* Automatically build, format, and display usage and help text based on the info of registered arguments */
void _argparse_help(const struct kjc_argparse* argparse_context);

/* Return the next argument (unparsed), advancing the argparse index */
char* _argparse_next(struct kjc_argparse* argparse_context);

/* Get current argument's attached integer value */
long _argparse_value_long(const struct kjc_argparse* argparse_context);

/* Get current argument's attached string value */
const char* _argparse_value_string(const struct kjc_argparse* argparse_context);


#ifdef __cplusplus
}
#endif

#endif /* KJC_ARGPARSE_H */
