---
title: NonlibC
order: 100
---

# nonlibc [![Build Status](https://travis-ci.org/siriobalmelli/nonlibc.svg?branch=master)](https://travis-ci.org/siriobalmelli/nonlibc)

Collection of standard-not-standard utilities for the discerning C programmer.

See these docs:

-	[on github](https://github.com/siriobalmelli/nonlibc)
-	[as a web page](https://siriobalmelli.github.io/nonlibc/)

## Yeah, but what does it do

The functions in this library solve or alleviate a bunch of commonplace
	problems/annoyances related to writing programs in C.

This is stuff not addressed in `libc` or `GLib`
	(or for which code therein is disliked by the author;
	or for which they might be overkill).

The focus is on solving problems with a healthy dose of minimalism
	and performance (rarely orthogonal qualities, as it were).

This is *not* a hodgepodge - the code here is quite reliable
	and being used in the real world.

Here's a quick rundown of the contents:

### Control flow; print mechanics - [zed_dbg.h](include/zed_dbg.h)

This header helps with the following annoyances:

-	forgetting to insert a '\n' at the end of an impromptu
		debug `printf` statement
-	spending time commenting/uncommenting print statements,
		no easy way to enable/disable entire categories
		or sets of prints
-	`printf`s inside conditionals increase code size
		for no good reason
-	program throws an error, but forgot to write a print there
-	program throws an error and does do a print,
		but you have to `grep` for it
		because you don't remember which source file it was in
-	kryptonite print statements full of CPP kudge
		like `__FILE__` and `__LINE__`
-	complex control flow in functions handling external state
		e.g. malloc(), open(), fork()
-	time spent making print output marginally legible so the Dev Ops people
		don't poison your coffee
-	needing to pretty-print large arrays of bytes in hex,
		for post-mortem analysis

Some usage examples are in [zed_dbg_test.c](test/zed_dbg_test.c);
	[zed_dbg.h](include/zed_dbg.h) itself is fairly well commented also.

### fast, reliable 32-bit AND 64-bit hashing - [fnv.h](include/fnv.h)

If you need a non-crypto hash, odds are you want [FNV1A](https://en.wikipedia.org/wiki/Fowler%E2%80%93Noll%E2%80%93Vo_hash_function).

This is the simplest, least-hassle implementation I know of.

Usage examples in [fnv_test.c](test/fnv_test.c).

An `fnvsum` utility is also provided for hashing files (or stdin),
	on the pattern of `md5sum`.
See the [fnvsum man page](man/fnvsum.md)

If you would like to use `fnvsum` without installing:

```bash
$ ./bootstrap.py
$ # hash stdin:
$ echo -n '' | build-release/util/fnvsum
cbf29ce484222325  -
$ # hash a file:
$ build-release/util/fnvsum /path/to/some/file
```

### straightforward integer math - [nmath.h](include/nmath.h)

Sometimes what you DON'T want is floating-point math.

This library has sane implementations for:

-	integer ceiling division (i.e.: if not evenly divisible, return +1)
-	get the next power of 2 (or current number if a power of 2)
-	return the next higher multiple of a number (or itself if evenly divisible)
-	get the position of the lowest '1' bit in an integer

Oh, and they're all inlines so your compiler can reason about local variables
	and turn out decent assembly.

Every function is shown used in [nmath_test.c](test/nmath_test.c)

### generating and parsing hexadecimal strings vis-a-vis integers and bytefields

That's done by [hx2b.h](include/hx2b.h) and [b2hx.h](include/b2hx.h).

Check out [hex2bin2hex_test.c](test/hex2bin2hex_test.c) for the full monte.

### proper RNG which isn't a hassle to set-up and use - [pcg_rand.h](include/pcg_rand.h)

This is a simple, clean, fast implementation of [PCG](http://www.pcg-random.org/).

That matters more than you may think - and anyways this API is simple and clean.

Check out the test code at [pcg_rand_test.c](test/pcg_rand_test.c).

### a very fast growing LIFO - [lifo.h](include/lifo.h)

The fastest possible way I know to push/pop pointer-sized values onto a stack,
	which grows (reallocates memory) as necessary,
	and can be kept around between function calls.

### accurately timing blocks of code

Use the `nlc_timing_start()` and `nlc_timing_stop()` macros in [nonlibc.h](include/nonlibc.h).

See e.g.: [fnv_test.c](test/fnv_test.c) for usage examples.

### other odds-and-ends

-	visibility and inlining macros in [nonlibc.h](include/nonlibc.h)
-	some lock-free/atomics in [atop.h](include/atop.h)

## Grokking the codebase

Set yourself up so you can look up and jump to/from functions/symbols
	inside your text editor or IDE.

As an example, I use [vim](http://www.vim.org/) with
	[cscope](http://cscope.sourceforge.net/) support,
	and you'll see that [bootstrap.py](./bootstrap.py) generates
	cscope files from the sources.

Then start looking at the example code and related comments in the `test` dir;
	you'll be able to jump to function definitions/implementations
	and read their documentation in context with the usage in the test.

Communication is always welcome, feel free to send a pull request
	or drop me a line at <sirio.bm@gmail.com>.

### You Complete Me

The author uses [YCM](https://github.com/Valloric/YouCompleteMe).

You will notice that the [ycm config file](.ycm_extra_conf.py)
	contains `compilation_database_folder='./build-debug'`.

A successful build by ninja yields the file `build-debug/compile_commands.json`,
	which means YCM should pretty much work out-of-the-box.

## Will it compile on my system

Quite likely.

This library uses the [Meson build system](http://mesonbuild.com/index.html),
	so things will be straightforward.

Try running `./bootstrap.py` - chances are things will work
	automagically and you'll find yourself with everything built and all tests running.

### I'm on Windows || It doesn't work

Don't despair. Things should still work with a little manual twiddling:

-	[install ninja](https://ninja-build.org/)
-	[install meson](http://mesonbuild.com/Getting-meson.html)
-	run: `meson --buildtype debugoptimized build-debug`
-	`cd` into `build-debug` and run `ninja test`
-	optional: you can also run the tests under `valgrind` with
		`mesontest --wrap='valgrind --leak-check=full'`
		(from inside `build-debug` of course)

## I want to link my program against this library

Get the library building as above, then run:

```bash
cd build-release && ninja all && sudo ninja install
```

Et voilà - now use [pkg-config](https://www.freedesktop.org/wiki/Software/pkg-config/)
	to configure inclusion and linkage in your build system du jour.

To verify things are kosher, check if you get some useful output from:
	`pkg-config --modversion nonlibc`.

If you're using [Meson](http://mesonbuild.com/index.html) to build your project,
	it can now find and link to this library automatically,
	once you insert the following stanza in your `meson.build` file:

```meson
nonlibc = dependency('nonlibc')
```

... and then add the `dependencies : nonlibc` stanza to the `executable` declaration(s)
		in your `meson.build` file(s); e.g.:

```meson
executable('demo', 'test.c', dependencies : nonlibc)
```

## I want to statically include this library in my program

This is a very good idea:

-	The build system won't have to build
		[position-independent code](https://en.wikipedia.org/wiki/Position-independent_code)
		which will give a measurable speedup.
-	Compiler can discard unused objects and reduce memory footprint.

The easiest way is if you also build your project with [Meson](http://mesonbuild.com/index.html).

In that case, I recommend you do the following:

-	`mkdir -p subprojects`
-	`git remote add nonlibc https://github.com/siriobalmelli/nonlibc.git`
-	`git subtree add --prefix=subprojects/nonlibc nonlibc master`
-	Add the following line to your toplevel `meson.build` file:

```meson
nonlibc = dependency('nonlibc', fallback : ['nonlibc', 'nonlibc_dep'])
```

-	Add the `dependencies : nonlibc` stanza to the `executable` declaration(s)
		in your `meson.build` file(s); e.g.:

```meson
executable('demo', 'test.c', dependencies : nonlibc)
```

If you're not using Meson, but [Linking](#i-want-to-link-against-this-library)
	is not an option for you,
	drop me a line and I'll see if I can help.

## Cross-compilation

Cross compilation is tested for ARM so far, using [cross_arm.txt](toolchain/cross_arm.txt):

```bash
meson --cross-file=toolchain/cross_arm.txt --buildtype=plain build-arm
cd build-arm
ninja
```

We still depend on a bunch of `libc` symbols at link-time, though
	(oh, the irony).

A solution for this is in the offing.

Stay tuned.

## [docs about the docs](docs/documentation.md)

... covers how this documentation is structured and how to hack on it.

## [Hacking Tips](docs/hacking_tips.md)

Covers the coding style and rationale.

## [TODO list](docs/TODO.md)

All pending things go there.
