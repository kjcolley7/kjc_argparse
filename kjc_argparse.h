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
 * ARGPARSE()
 * ARG()
 * ARG_INT()
 * ARG_STRING()
 * ARG_OTHER()
 * ARG_END()
 * ARGPARSE_HELP()
 * ARGPARSE_INDEX()
 *
 * For usage instructions, refer to example.c: https://github.com/kjcolley7/kjc_argparse/blob/master/example.c
 */


#ifndef UNIQUIFY
#define UNIQUIFY(macro, ...) UNIQUIFY_(macro, __COUNTER__, ##__VA_ARGS__)
#define UNIQUIFY_(macro, id, ...) UNIQUIFY__(macro, id, ##__VA_ARGS__)
#define UNIQUIFY__(macro, id, ...) macro(id, ##__VA_ARGS__)
#endif

#define ARGPARSE(argc, argv) \
/* Setup _argparse_once to allow non-looping for loops (for declaring scope-local variables) */ \
for(int _argparse_once = 1; _argparse_once; _argparse_once = 0) \
	/* Setup argparse info structure with argc and argv */ \
	for(struct _argparse _argparse_context = {}; _argparse_once; _argparse_once = 0) \
		/* Actual argument parsing loop, first iteration is counting phase, then initialization phase, then */ \
		/* after that each iteration is for parsing argv[_argidx]. */ \
		for(int _argidx = 1, _arg = _argparse_init(&_argparse_context, (argc), (argv), LONG_ARG_MAX_WIDTH), _argdone = 0; \
			_arg != ARG_VALUE_ERROR && !_argdone; \
			/* _argdone is updated before _arg, that way we perform one iteration where _arg is ARG_VALUE_END (for ARG_END) */ \
			_argdone = _arg == ARG_VALUE_END, \
				/* Actually parse the argument argv[_argidx] */ \
				/* When _arg is ARG_VALUE_END, instead free resources in _argparse_context */ \
				_arg = _argparse_parse(&_argparse_context, &_argidx, _arg)) \
			/* Code inside is only reachable via goto _argparse_break */ \
			if(0) { \
				/* This is here to support the break keyword in an argument handler working as expected */ \
				_argparse_break: \
					/* Setting _arg to this value will cause the update part of the argparse loop to free the context */ \
					/* Also, this will cause the loop condition to evaluate false, exiting the loop after cleanup */ \
					_arg = ARG_VALUE_END; \
					continue; \
			} \
			else \
				/* Jump table based on the generated argument ID to select an argument handler */ \
				/* For the count and initialization phases, we instead jump to the beginning of the code block */ \
				/* Trailing statement after this macro invocation will attach to this switch statement! */ \
				switch(_arg) \
					case ARG_VALUE_COUNT: \
					case ARG_VALUE_INIT:

/* Ensure that the argument ID won't collide with any "special" ARG_VALUE_* values (even if value is 0) */
#define _arg_make_id(value) ((((value) << 1) | 1) << ARG_NORMAL_SHIFT)

#define _arg_handler(...) \
/* Set up _arg_loop to determine when this outer loop has run at least once. */ \
/* Also set up _arg_break, which is only set to zero when the inner loop's update */ \
/* expression runs. This means that if the inner loop's update expression is skipped by */ \
/* use of the break keyword within that loop, then _arg_break will still be 1. */ \
for(int _arg_loop = 0, _arg_break = 1; ; ++_arg_loop) \
	if(_arg_loop == 1) { \
		/* We already ran the argument handler body */ \
		if(_arg_break) { \
			/* The argument handler was escaped via the break keyword, so break out of the argparse loop */ \
			goto _argparse_break; \
		} \
		else { \
			/* Argument handler block executed normally */ \
			/* Only break out of the above loop that defines _arg_loop */ \
			break; \
		} \
	} \
	else \
		/* First loop, so run the argument handler and check if it ends normally or breaks out early */ \
		/* Trailing statement after this macro invocation will attach to this for statement! */ \
		for(__VA_ARGS__; _arg_break; _arg_break = 0)

#define ARG(short_name, long_name, description) \
UNIQUIFY(_arg_helper, short_name, long_name, description)
#define _arg_helper(id, short_name, long_name, description) \
if(_arg == ARG_VALUE_COUNT) { \
	/* Count phase: increment the count of arguments to be registered during the initialization phase */ \
	++_argparse_context.args_cap; \
} \
else if(_arg == ARG_VALUE_INIT) { \
	/* Initialization phase: register this argument's info in the _argparse_context struct */ \
	_argparse_add(&_argparse_context, _arg_make_id(id), short_name, long_name, description, ARG_TYPE_VOID); \
} \
/* Code inside is only accessible via jumptable from switch statement in ARGPARSE, NOT the initialization phase */ \
else if(0) \
	case _arg_make_id(id): \
		/* Trailing statement after this macro invocation will be the argument handler body. */ \
		/* Keywords like break and continue will work as expected, but return will leak memory */ \
		_arg_handler()

#define ARG_INT(short_name, long_name, description, value) \
UNIQUIFY(_arg_int_helper, short_name, long_name, description, value)
#define _arg_int_helper(id, short_name, long_name, description, value) \
if(_arg == ARG_VALUE_COUNT) { \
	/* Count phase: increment the count of arguments to be registered during the initialization phase */ \
	++_argparse_context.args_cap; \
} \
else if(_arg == ARG_VALUE_INIT) { \
	/* Initialization phase: register this argument's info in the _argparse_context struct */ \
	_argparse_add(&_argparse_context, _arg_make_id(id), short_name, long_name, description, ARG_TYPE_INT); \
} \
/* Code inside is only accessible via jumptable from switch statement in ARGPARSE, NOT the initialization phase */ \
else if(0) \
	case _arg_make_id(id): \
		/* Trailing statement after this macro invocation will be the argument handler body. */ \
		/* Keywords like break and continue will work as expected, but return will leak memory */ \
		_arg_handler(int value = _argparse_value_int(&_argparse_context))

#define ARG_STRING(short_name, long_name, description, value) \
UNIQUIFY(_arg_string_helper, short_name, long_name, description, value)
#define _arg_string_helper(id, short_name, long_name, description, value) \
if(_arg == ARG_VALUE_COUNT) { \
	/* Count phase: increment the count of arguments to be registered during the initialization phase */ \
	++_argparse_context.args_cap; \
} \
else if(_arg == ARG_VALUE_INIT) { \
	/* Initialization phase: register this argument's info in the _argparse_context struct */ \
	_argparse_add(&_argparse_context, _arg_make_id(id), short_name, long_name, description, ARG_TYPE_STRING); \
} \
/* Code inside is only accessible via jumptable from switch statement in ARGPARSE, NOT the initialization phase */ \
else if(0) \
	case _arg_make_id(id): \
		/* Trailing statement after this macro invocation will be the argument handler body. */ \
		/* Keywords like break and continue will work as expected, but return will leak memory */ \
		_arg_handler(const char* value = _argparse_value_string(&_argparse_context))

#define ARG_POSITIONAL(usage, argname) UNIQUIFY(_arg_positional_helper, usage, argname)
#define _arg_positional_helper(id, usage, argname) \
if(_arg == ARG_VALUE_INIT) { \
	/* Initialization phase: mark the existence of an ARG_OTHER block in the _argparse_context struct */ \
	_argparse_context.has_catchall = 1; \
	_argparse_context.positional_usage = (usage); \
} \
/* Code inside is only accessible via jumptable from switch statement in ARGPARSE, NOT the initialization phase */ \
else if(0) \
	case ARG_VALUE_OTHER: \
	default: \
		/* Trailing statement after this macro invocation will be the argument handler body. */ \
		/* Keywords like break and continue will work as expected, but return will leak memory */ \
		_arg_handler(const char* argname = _argparse_context.orig_argv[_argidx-1])

#define ARG_OTHER(argname) ARG_POSITIONAL(argname, NULL)

#define ARG_END() UNIQUIFY(_arg_end_helper)
#define _arg_end_helper(id) \
/* Code inside is only accessible via jumptable from switch statement in ARGPARSE, NOT the initialization phase */ \
if(0) \
	case ARG_VALUE_END: \
		/* Trailing statement after this macro invocation will be the argument handler body. */ \
		/* Keywords like break and continue will work as expected, but return will leak memory */ \
		_arg_handler()

#define ARGPARSE_HELP() _argparse_help(&_argparse_context)

#define ARGPARSE_INDEX() (_argidx-1)

#define ARGPARSE_SET_OUTPUT(fp) do { \
	if(_arg == ARG_VALUE_INIT) { \
		_argparse_context.stream = (fp); \
	} \
} while(0)


/*
 * For defining an upper limit on the column for aligning help text.
 * This can be overridden by defining it to another value before including kjc_argparse.h
 */
#ifndef LONG_ARG_MAX_WIDTH
#define LONG_ARG_MAX_WIDTH 30
#endif

/* "Special" values for the argument ID, _arg */
#define ARG_VALUE_COUNT 0  /* Set only during the first iteration, aka the counting phase */
#define ARG_VALUE_INIT  1  /* Set only during the second iteration, aka the initialization phase */
#define ARG_VALUE_ERROR 2  /* Set when argparse internally encounters an error for some reason */
#define ARG_VALUE_END   3  /* Set when all arguments have been parsed */
#define ARG_VALUE_OTHER 4  /* Set when the argument being parsed doesn't match a declared argument */

/* (1 << ARG_NORMAL_SHIFT) should be greater than the max special ARG_VALUE_* value (4 aka 0b100) */
#define ARG_NORMAL_SHIFT 3


/* Intentionally not using an enum so the underlying type doesn't have to be int */
typedef unsigned char _argtype;
#define ARG_TYPE_VOID 0
#define ARG_TYPE_INT 1
#define ARG_TYPE_STRING 2
#define ARG_TYPE_SHORTGROUP 3

/* Fields have been hand-packed, hence the weird ordering */
struct _arginfo {
	const char* long_name;
	const char* description;
	int arg_id;
	_argtype type;
	char short_name;
};

/* Fields have been hand-packed, hence the weird ordering */
struct _argparse {
	char** orig_argv;
	union {
		const char* val_string;
		int val_int;
	} argvalue;
	struct _arginfo* args;
	unsigned args_cap;
	unsigned args_count;
	int orig_argc;
	int long_name_width;
	int long_name_max_width;
	int has_catchall;
	const char* positional_usage;
	void* stream;
	unsigned char short_bitmap[32];
	unsigned char short_value_bitmap[32];
	_argtype argtype;
};


/* Initializes the argparse context structure and returns the initial argparse state (ARG_VALUE_COUNT) */
int _argparse_init(struct _argparse* argparse_context, int argc, char** argv, int long_name_max_width);

/* Register an argument with the argparse context struct */
void _argparse_add(
	struct _argparse* argparse_context,
	int arg_id,
	char short_name,
	const char* long_name,
	const char* description,
	_argtype type
);

/* Advance the argparse state machine, usually by parsing an argument */
int _argparse_parse(struct _argparse* argparse_context, int* argidx, int state);

/* Automatically build, format, and display usage and help text based on the info of registered arguments */
void _argparse_help(struct _argparse* argparse_context);

/* Get current argument's attached integer value */
int _argparse_value_int(struct _argparse* argparse_context);

/* Get current argument's attached string value */
const char* _argparse_value_string(struct _argparse* argparse_context);


#endif /* KJC_ARGPARSE_H */
