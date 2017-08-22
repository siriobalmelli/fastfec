---
title: TODO
order: 0
---

# TODO

-	man pages
		(and how best to integrate with markdown docs readable when browsing github?)
-	turn the Hacking Notes list into brief snippets
-	evaluate licensing - is GPL2 the least restrictive?
-	integrate code coverage testing
-	add library to WrapDB; submit to Meson site for inclusion
-	code optional replacements for required libc calls (e.g.: on ARM) ?
-	reimplement malloc(); realloc(); free(); memcpy() to use
		zero-copy I/O primitves behind the scenes
		(see <https://dvdhrm.wordpress.com/tag/memfd/>)
-	Evaluate the usefulness of `atop`,
		and *"when free()ing shared values, use an atomic swap"*
