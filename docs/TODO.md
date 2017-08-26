---
title: TODO
order: 0
---

# TODO

These are in order by priority:

-	reimplement malloc(); realloc(); free(); memcpy() to use
		zero-copy I/O primitves behind the scenes
		(see <https://dvdhrm.wordpress.com/tag/memfd/>)
-	Evaluate the usefulness of `atop`,
		and *"when free()ing shared values, use an atomic swap"*
-	add library to WrapDB; submit to Meson site for inclusion
-	man pages
-	integrate code coverage testing
-	code optional replacements for required libc calls (e.g.: on ARM) ?
-	evaluate licensing - is GPL2 the least restrictive?
-	*view* any source files linked in the documentation (with syntax highlight),
		don't try and download it (facepalm)

## TODO: lifo

-	slow on Darwin ;(
		(lack of mremap getting me down)

## TODO: bootstrap.py

-	document
-	building on windows
