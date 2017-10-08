---
title: 'ncp(1) nonlibc | General Commands Manual'
order: 1
---

# NAME

ncp - `cp` replacement using zero-copy I/O

# SYNOPSIS

Copy one file:

```bash
ncp [OPTION]... SOURCE_FILE DEST_FILE
```

Copy multiple files into a directory:

```bash
ncp [OPTION]... SOURCE_FILE... DEST_DIR
```

# DESCRIPTION

Copy SOURCE_FILE to DEST_FILE using `nmem` (zero-copy I/O).

Typically >20% faster for large files.

# OPTIONS

## -v | --verbose

list each file being copied

## -f | --force

overwrite destination file(s) if existing

## -h | --help

print usage and exit

# EXIT STATUS

0 on success

# EXAMPLE

```bash
$ cp /a/file /a/dest
$
```

# AUTHORS

Sirio Balmelli; Balmelli Analog & Digital

# SEE ALSO

`ncp` uses the nmem(3) library for zero-copy I/O

This utility is part of the [nonlibc](https://github.com/siriobalmelli/nonlibc) library.
