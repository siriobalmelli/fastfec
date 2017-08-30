---
title: 'man: ncp'
order: 1
---

# NCP 1 2017-08-22 nonlibc POSIX

## NAME

ncp - `cp` replacement using zero-copy I/O

## SYNOPSIS

```bash
ncp [SOURCE_FILE] [DEST_FILE]
```

## DESCRIPTION

Copy SOURCE_FILE to DEST_FILE using `nmem` (zero-copy I/O).

## OPTIONS

TODO: none implemented at the moment

## EXIT STATUS

0 on success

## EXAMPLE

```bash
$ cp /a/file /a/dest
$
```

## AUTHORS

Sirio Balmelli; Balmelli Analog & Digital

## SEE ALSO

nmem(3)

This utility is part of the [nonlibc](https://github.com/siriobalmelli/nonlibc) library.
