#!/bin/bash

script_dir="${BASH_SOURCE%/*}"
prog="$script_dir/full_example"

function run {
	echo "$@"
	$@
}

function run_tests {
	run $prog
	
	run $prog --help
	
	run $prog -h
	
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
	
	run $prog -s foo
	
	run $prog --my-string-argument foo
	
	run $prog --my-string-argument=foo
	
	run $prog -f -i 42
	
	run $prog -f --my-int-argument 42
	
	run $prog -f --my-int-argument=42
	
	run $prog -fi 42
	
	run $prog -if 42
	
	run $prog -f test
	
	run $prog -f -- here are some args
	
	run $prog -tHfots lol --my-int-argument=42 -- test lol omg --help -o -t
	
	run $prog -
	
	run $prog ---
	
	run $prog -f- test
	
	run $prog -=
	
	run $prog -=test
	
	run $prog --=
	
	run $prog --=test
}

run_tests >test_out.actual 2>test_err.actual
diff test_out.{expected,actual} && diff test_err.{expected,actual} && echo "All tests passed!" || echo "Tests failed."
