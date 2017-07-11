#!/bin/bash

#	bootstrap.sh
# This file (re-)sets up the Meson build system and makes all the build directories.
# Run this only once.
# Afterwards, just go into the relevant build directory and issue
#+	`ninja all`.
#
# NOTE: This file is very specifically for the author's convenience;
# This library will probably build just fine on your system,
#+	even if not supported in this script;
#+	it will just require that you replicate these steps manually.
#
# NOTE: Designed with Travis CI in mind - just set 'script: ./bootstrap.sh'
#+	in '.travis.yml' and you should be good.
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


#	comp_ver()
# Compare 2 arbitrary version strings using sort
#		$1	:	check this
#		$2	:	... against this
#		$3	:	name - for error print only
# Returns 0 if $1 >= $2
comp_ver()
{
	if [[ "$1" = "$2" || "$1" > "$2" ]]; then
		return 0
	fi
	# bad
	echo "'$3' version '$1' does not meet required '$2'" >&2
	return 1
}


#	run_pkg_()
# Run available package managers to try and install something.
#		$1	:	array of "sudo" or "" strings ... because brew is the
#+					ONE package manager that gets pissy being run sudo SMH
#		$2	:	array of package managers
#		$3	:	array of command line options (one per package manager)
#		$4	:	array of package names
# Return 0 on success
run_pkg_()
{
	SUDO="$1"
	MGR="$2"
	OPT="$3"
	PKG="$4"
	# This is bread-and-butter brute-force; try all the ones we know
	#+	until one of them shows positive.
	for i in $(seq 0 $(( ${#MGR[@]} -1 ))); do
		if which ${MGR[$i]} >/dev/null 2>&1; then
			run_die ${SUDO[$i]} ${MGR[$i]} ${OPT[$i]} ${PKG[$i]}
			return 0
		fi
	done

	# no joy
	echo "failed to find a package manager and install '${3[0]}'" >&2
	return 1
}


#	check_ninja()
# Check the existence and version of ninja; otherwise try to install a binary
check_ninja()
{
	# Try and get ninja from the package manager
	if ! which ninja; then
		SUDO=("sudo"   "sudo"       "sudo"    "sudo"   "sudo"    ""        "sudo" )
		MGR=( "pacman" "apt-get"    "dnf"     "emerge" "port"    "brew"    "pkg" )
		OPT=( "-S"     "-y install" "install" ""       "install" "install" "install" )
		PKG=( "ninja" \
			"ninja-build" \
			"ninja-build" \
			"dev-util/ninja" \
			"ninja" \
			"ninja"	\
			"ninja" )

		# ... not a critical failure if this doesn't fly - we will try
		#+	installing a binary below
		run_pkg_ "$SUDO" "$MGR" "$OPT" "$PKG"
	fi

	# ... and it must be the correct version
	VERSION="1.7.2"
	if ! which ninja || ! comp_ver "$(ninja --version)" "$VERSION" "ninja"; then
		# download a binary
		echo "will try to download ninja binary..."
		URL="https://github.com/ninja-build/ninja/releases"
		PLATFORM=$(uname)
		if [[ "$PLATFORM" = "Linux" ]]; then
			URL="${URL}/download/v$VERSION/ninja-linux.zip"
		elif [[ "$PLATFORM" = "Darwin" ]]; then
			URL="${URL}/download/v$VERSION/ninja-mac.zip"
		else
			echo "don't know how to handle '$PLATFORM'" >&2
			exit 1
		fi
		run_die mkdir -p ./toolchain
		run_die wget -O ./toolchain/ninja.zip "$URL"

		# install
		INSTALL="/usr/local/bin"
		run_die sudo unzip -o -d $INSTALL/ ./toolchain/ninja.zip
		run_die sudo chmod go+rx $INSTALL/ninja
		# check version again
		if ! comp_ver "$(ninja --version)" "$VERSION" "ninja"; then
			echo "installed a binary ninja v$VERSION to $INSTALL; 'ninja --version' still fails." >&2
			echo "Please check \$PATH" >&2
			exit 1
		fi
	fi
}


#	check_meson()
check_meson()
{
	# check mason
	if ! which meson; then
		# ... which requires pip3
		if ! which pip3; then
			# TODO: expand selection; fix implicit version in MacPorts
			SUDO=("sudo"        "sudo"       "" )
			MGR=( "apt-get"     "port"       "brew" )
			OPT=( "-y install"     "install" "install" )
			PKG=( "python3-pip" "py35-pip"   "python3" )

			if ! run_pkg_ "$SUDO" "$MGR" "$OPT" "$PKG"; then
				exit 1
			fi
		fi
		# full path to pip3 ... because Travis? wtf
		run_die sudo -H $(which pip3) install meson
	fi
}


#	main()
# Pseudo-main: better cater to the rat-brain internal parser of C programmers ;)
main()
{
	# optional: update cscope
	if which cscope; then
		run_die cscope -b -q -U -I./include -s./src -s./test
	fi

	# requires ninja build system
	check_ninja
	# meson
	check_meson

	# specify desired builds; test all of them
	run_die rm -rfv build*
	BUILD_NAMES=( "debug" "debug-opt"      "release" "plain" )
	BUILD_TYPES=( "debug" "debugoptimized" "release" "plain" )
	BUILD_GRIND=( ""      "yes"            ""        "" )

	for i in $(seq 0 $(( ${#BUILD_NAMES[@]} -1 ))); do
		run_die meson --buildtype ${BUILD_TYPES[$i]} build-${BUILD_NAMES[$i]}
		pushd build-${BUILD_NAMES[$i]}
		run_die ninja test
		# NOTE: we set 'VALGRIND' so it can be tested with getenv()
		#+	inside the tests themselves.
		if [[ ${BUILD_GRIND[$i]} ]] && which valgrind; then
			run_die VALGRIND=1 mesontest --wrap=\'valgrind --leak-check=full\'
		fi
		popd
	done
}

main
