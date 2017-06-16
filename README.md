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

For usage examples, see the `.c` files in `test`.

Communication is always welcome, feel free to send a pull request
	or drop me a line at <https://github.com/siriobalmelli>.


## I want to link against this library
Fortunately this library uses the [Meson build system](http://mesonbuild.com/index.html),
	so things will be quite straightforward.

If you're on a POSIX, try running `./bootstrap.sh` - chances are things will work
	automagically and you'll find yourself with a `build-release` directory.

### I'm on Windows || It doesn't work
Don't despair. Things should still be pretty straightforward:

-	[install ninja](https://ninja-build.org/)
-	[install meson](http://mesonbuild.com/Getting-meson.html)
-	run: `meson --buildtype release build-release`

### Ok, I have a `build-release` dir
Brilliant. \
Now run: `cd build-release && ninja all && sudo ninja install` \
You should now get useful output from: `pkg-config --modversion nonlibc`.

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
-	all hail the Linux Kernel Style guide
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
-	Visibility is important: <https://gcc.gnu.org/wiki/Visibility>
-	unity builds? yes please
-	use test programs to show intended usage
	

## TODO
-	nitpick coding style
-	turn the Hacking Notes list into brief snippets
-	evaluate licensing - is GPL2 the least restrictive?
