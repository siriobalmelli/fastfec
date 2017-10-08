---
title: TODO
order: 0
---

# TODO

These are in order by priority:

-	overloaded size_t functions for nmath
-	clean up staticity, const'ness, purity; add to hacking tips and visibility macros?
-	add library to WrapDB; submit to Meson site for inclusion
-	man pages
-	integrate code coverage testing
-	code optional replacements for required libc calls (e.g.: on ARM) ?
-	evaluate licensing - is GPL2 the least restrictive?
-	*view* any source files linked in the documentation (with syntax highlight),
		don't try and download it (facepalm)
-	Move option parsing to Argp ... which likely means meson-wrapping it
		since it doesn't come standard with LibC on all systems?!

## TODO: lifo

-	slow on Darwin ;(
		(lack of mremap getting me down)

## TODO: bootstrap.py

-	document
-	building on windows

## TODO: man pages

-	nmem(3)
-	fnv(3)
-	n_dirname(3); n_basename(3); n_join(3)
