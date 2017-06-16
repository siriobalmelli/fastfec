# nonlibc
Collection of standard-not-standard utilities for the discerning C programmer.
This is *not* a hodgepodge, nor is the code haphazard - the code here is quite reliable.

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
	

## TODO
-	nitpick coding style
-	turn the notes list into brief snippets
-	include docs from each of the utilities separately?
-	evaluate licensing - is GPL2 least restrictive?
