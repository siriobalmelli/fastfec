# Fast FEC [![Build Status](https://travis-ci.org/siriobalmelli/fastfec.svg?branch=master)](https://travis-ci.org/siriobalmelli/fastfec)

The Fast (and barebones) FEC library.

This library implements a basic (and fast) LDGM staircase FEC.
For some background data and math, please see:

- Design, Evaluation and Comparison of Four Large Block FEC Codecs \
    Vincent Roca, Christoph Neumann - 19 May 2006 \
    <https://hal.inria.fr/inria-00070770/document>

- Systems of Linear Equations: Gaussian Elimination \
    <http://www.sosmath.com/matrix/system1/system1.html>

## Design

The design goal is a FEC codec optimized for:

- speed and memory footprint
- large objects: depending on 'fec_ratio' (see def. below),
    anything 2GB or less should be OK.
- low fec_ratios (i.e.: `<1.2` ... aka `<20%`)

For these reasons it was chosen NOT to use gaussian elimination at the decoder.

## Thread-Safety

NONE

## Nomenclature

### FEC: Forward Error Correction

The practice of generating some additional data at the
    "sender", to be sent along with the original data,
    so that "receiver" can re-compute data lost
    in transit (as long as it is under a certain
    percentage).

### LDGM: Low Density Generator Matrix
Describes the method for implementing this type of FEC,
    where computations to be done on the data
    are represented in a "low density" (aka: sparse)
    matrix, which is also then used to GENERATE the
    FEC data (therefore: "generator").

### symbol: A single "unit" of data in FEC.
It is assumed that "symbol" is the payload of a single
    network packet, therefore `<=9KB` and `>=1KB`.

### variables

| variable       | name                                     | description, notes                          |
| -------------- | ---------------------------------------- | ------------------------------------------- |
| `k`            | number of source symbols                 | source is what must be reconstructed at rx  |
| `n`            | total number of symbols                  | "k" source symbols and "p" parity symbols   |
| `p`            | number of parity symbols                 | (created by XORing source syms) `p = n - k` |
| `esi`          | Encoding Symbol ID                       | zero-indexed                                |
| `fec_ratio`    | `n = fec_ratio * k`                      | Example: "1.1" == "10% FEC"                 |
| `inefficiency` | "number of symbols needed to decode" / k | always greater than 1                       |

Note that inefficiency denotes how many MORE symbols than source are needed
to allow decoding.
A perfect codec (e.g. reed-solomon) has inefficiency == 1,
but the tradeoff of LDGM is (slight) inefficiency.

# BUGS

None logged yet

# TODO
- check assembly: what does memory clobber of xor change?
- 0-copy I/O in nonlibc; replace in ffec
- 64-bit performance (extremely large blocks?)

# IDEAS
- split up seeds across many packets when transmitting:
    less overhead per-packet; depend on MANY of them arriving
- perhaps derive 2x 64-bit seeds from a single 64-bit number?
