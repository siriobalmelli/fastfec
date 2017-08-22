---
title: 'man: fnvsum'
order: 1
---

# FNVSUM 1 2017-08-22 nonlibc POSIX

## NAME

fnvsum - hash input using 64-bit FNV1a

## SYNOPSIS

```bash
fnvsum [OPTIONS] [FILE | -]
```

## DESCRIPTION

Calculate the 64-bit FNV1a hash of FILE;
	read from standard input if no FILE or when FILE is `-`

The hash function is [FNV1a](https://en.wikipedia.org/wiki/Fowler%E2%80%93Noll%E2%80%93Vo_hash_function).

## OPTIONS

### -? | -h

print usage details

## EXIT STATUS

0 on success

## EXAMPLE

Hash from stdin:

```bash
$ echo -n '/the/enemy/gate/is/down' | fnvsum
7814fb571359f23e  -
$
```

Hash a file:

```bash
$ echo -n '/the/enemy/gate/is/down' > a_file.txt
$ fnvsum a_file.txt
7814fb571359f23e  a_file.txt
$
```

## AUTHORS

Sirio Balmelli; Balmelli Analog & Digital

## SEE ALSO

fnv(3)

This utility is part of the [nonlibc](https://github.com/siriobalmelli/nonlibc) library.
