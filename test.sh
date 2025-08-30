#!/bin/bash

script_dir="${BASH_SOURCE%/*}"
prog_dir="$script_dir/examples"
prog="$prog_dir/full_example"

function run {
	echo "$@"
	$@
}

function run_tests {
	run $prog
	
	run $prog --help
	
	run $prog --usage
	
	run $prog --test
	
	run $prog -f
	
	run $prog --flag -f
	
	run $prog -ffffffff
	
	run $prog -o -f
	
	run $prog -of
	
	run $prog -ofo
	
	run $prog -f -oo
	
	run $prog -f -o --once
	
	run $prog --flag=test
	
	run $prog -n foo
	
	run $prog --set-name foo
	
	run $prog --set-name=foo
	
	run $prog -f -i 42
	
	run $prog -f --int-argument 42
	
	run $prog -f --int-argument=42
	
	run $prog -fi 42
	
	run $prog -if 42
	
	run $prog -f test
	
	run $prog -f -- here are some args
	
	run $prog -tHfotn lol --int-argument=42 -- test lol omg --help -o -t
	
	run $prog -
	
	run $prog ---
	
	run $prog -f- test
	
	run $prog -=
	
	run $prog -=test
	
	run $prog --=
	
	run $prog --=test
}

run_tests >$prog_dir/full_out.actual 2>$prog_dir/full_err.actual
diff $prog_dir/full_out.{expected,actual} && \
	diff $prog_dir/full_err.{expected,actual} && \
	echo "All tests passed!" || \
	echo "Tests failed."
