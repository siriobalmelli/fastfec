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
	echo -e "\n\nEXEC: $*"
	/bin/bash -c "$*"
	poop=$?
	if (( $poop )); then
		echo "failed: $*" >&2
		exit $poop
	fi
}

#	install_ninja()
# Try and get a package manager for the system;
#+	if found use it to install ninja.
install_ninja()
{
	MGR=( "pacman"	"apt-get"	"dnf"		"emerge"	"port"		"brew"		"pkg" )
	OPT=( "-S"	"install"	"install"	""		"install"	"install"	"install" )
	PKG=( "ninja"	"ninja-build"	"ninja-build" 	"dev-util/ninja" "ninja"	"ninja"		"ninja" )

	# This is bread-and-butter brute-force; try all the ones we know
	#+	until one of them shows positive.
	for i in $(seq 0 $(( ${#MGR[@]} -1 ))); do
		if which ${MGR[$i]} >/dev/null 2>&1; then
			run_die sudo ${MGR[$i]} ${OPT[$i]} ${PKG[$i]}
			return 0
		fi
	done

	# no joy
	echo "missing package manager for this platform" >&2
	exit 1
}


#	main()
# Pseudo-main: better cater to the rat-brain internal parser of C programmers ;)
main()
{
	# optional: update cscope
	if which cscope; then
		run_die cscope -b -q -U -I./include -s./src -s./test
	fi

	# Make sure ninja and meson exist
	if ! which ninja; then
		install_ninja
	fi
	if ! which meson; then
		run_die sudo -H pip3 install meson
	fi

	# make build dirs
	rm -rfv build*
	run_die meson --buildtype debugoptimized build-debug
	run_die meson --buildtype release build-release
	run_die meson --buildtype plain build-plain

	# run the build
	pushd build-debug
	run_die ninja test
	# optional: valgrind all the tests
	if which valgrind; then
		run_die mesontest --wrap=\'valgrind --leak-check=full\'
	fi
	popd
}

main
