# nonlibc
Collection of standard-not-standard utilities for the discerning C programmer.


## Yeah, but what does it do?
The functions in this library solve or alleviate a bunch of commonplace
	problems/annoyances related to writing programs in C.

This is stuff not addressed in `libc` or `GLib`
	(or for which code therein is disliked by the author;
	or for which they might be overkill).

The focus is on solving problems with a healthy dose of minimalism
	and performance (rarely orthogonal qualities, as it were).

This is *not* a hodgepodge - the code here is quite reliable
	and being used in the real world.

For commented usage examples, see the `.c` files in `test`. \
Another place to pick up usage data is the relevant header file:
	it contains comments on usage and often has inline
	helper functions which show usage.

Communication is always welcome, feel free to send a pull request
	or drop me a line at <https://github.com/siriobalmelli>.


## Will it compile on my system?
Quite likely. \
This library uses the [Meson build system](http://mesonbuild.com/index.html),
	so things will be straightforward.

If you're on a POSIX, try running `./bootstrap.sh` - chances are things will work
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


## I want to link against this library
Get the library building as above, then run: `cd build-release && ninja all && sudo ninja install`. \
You should now get some useful output from: `pkg-config --modversion nonlibc`.

Et voilà - now use [pkg-config](https://www.freedesktop.org/wiki/Software/pkg-config/)
	to configure inclusion and linkage in your build system du jour.

If you happen to be using Meson, things are even simpler, just use TODO


## I want to statically include this library in my build
I'm flattered.

TODO


## Hacking Notes
A few tips about this code:
-	tabs!
-	tabs are 8 spaces
-	keep things clean by prefixing function groups with 4-letter faux-namespaces
-	handle end-of-function cleanup with gotos
-	when free()ing shared values, use an atomic swap
-	when in doubt, have a function return a status code as 'int'; 0 == success
-	when allocating, return a pointer; NULL == fail
-	when calling libc, probably best to return an FD; <1 == fail
-	all hail the [Linux kernel coding style](https://01.org/linuxgraphics/gfx-docs/drm/process/coding-style.html)
-	```
	/* A proper comment block:
	Lines start on the tab.
	Long lines are split
		and indented like code.
	*/
	```
-	Believe in the 80-character margin. Ignore it when it makes the code clearer.
-	Eschew the '_l' suffix convention.
	Rather, explicitly suffix functions with '32' or '64' when multiple word
		lengths are being used.
-	`.c` files only #include their related `.h` file, which includes anything else.
-	Visibility is important: <https://gcc.gnu.org/wiki/Visibility> - see the
		relevant helper macros in `nonlibc.h`.
	Also, feel free to suffix internal helper functions with an underscore '_'
		as a hint to other coders.
-	use test programs to show intended usage
-	Header files should list functions in the order in which they appear in the
		corresponding `.c` file.
	

## TODO
-	integrate valgrind and code coverage testing
-	test ARM cross-compilation
-	integration as static library; document
-	turn the Hacking Notes list into brief snippets
-	evaluate licensing - is GPL2 the least restrictive?
