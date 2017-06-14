# nonlibc
Collection of standard-not-standard utilities for the discerning C programmer

A few notes:
-	tabs!
-	tabs are 8 spaces
-	keep things clean by prefixing function groups with 4-letter faux-namespaces
-	handle end-of-function cleanup with gotos
-	when free()ing shared values, use an atomic swap
-	when in doubt, have a function return a status code as 'int'; 0 == success
-	when allocating, return a pointer; NULL == fail
-	when calling libc, probably best to return an FD; <1 == fail
-	all hail the Linux Kernel Style guide
-	```/*	A proper comment:
	Lines start on the tab.
	Long lines are split and indented
		like code.
	*/```
-	Believe in the 80-character margin. Ignore it when it makes the code clearer.

## TODO
-	nitpick coding style
-	turn the notes list into brief snippets
-	document each of the utilities separately
