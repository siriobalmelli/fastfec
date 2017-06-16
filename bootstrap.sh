#!/bin/bash

#	bootstrap.sh
# This file (re-)sets up the Meson build system and makes the relevant build directories.
# Run this only once. Afterwards, just go into the relevant build directory and issue
#+	`ninja all`.
#
# NOTE: This file is very specifically for the author's convenience;
# This library will probably build just fine on your system,
#+	even if not supported in this script;
#+	it will just require that you replicate these steps manually.
#
# (c) 2017 Sirio Balmelli; https://b-ad.ch



#	run_die()
# Simple helper: exec something with the shell; die if it barfs
#		$*	:	command sequence
run_die()
{
	/bin/bash -c "$*"
	poop=$?
	if (( $poop )); then
		echo "failed: $*" >&2
		exit $poop
	fi
}



#	main()
# Pseudo-main: better cater to the rat-brain internal parser of C programmers ;)
main()
{
	# Make sure ninja and meson exist
	if ! which ninja; then
		run_die sudo apt-get install ninja-build
	fi
	if ! which meson; then
		run_die sudo -H pip3 install meson
	fi

	# make build dirs
	rm -rfv build*
	run_die meson --buildtype debugoptimized build-debug
	run_die meson --buildtype release build-release
	run_die meson --buildtype plain build-plain
}

main
