# Subprojects directory

The `meson` build system will look here for dependencies (e.g.: `nonlibc`)
	which are not found through `dpkg`.

Please don't include dependencies as subtrees of this Git repo.

One development solution is to have the dependency's Git repo cloned somewhere
	else on your machine and symlink to it inside `subprojects`.

Example:

```bash
$ ln -s ~/nonlibc subprojects/
$ ls -lha subprojects/
total 32
drwxr-xr-x   4 luser  luser   136B Jan 29 12:35 .
drwxr-xr-x  33 luser  luser   1.1K Jan 29 12:30 ..
lrwxr-xr-x   1 luser  luser    28B Jan 29 12:35 nonlibc -> /home/luser/nonlibc
$
```
