#include "fnv.h"

/*	fnv_hash64()
Perform, or continue, a 64-bit FNV1A hash.

If 'hash' is NULL, then begin the hash with 'basis' and move on from there.
Otherwise, load *hash and continue hashing.

The reason for a uint64_t* input as opposed to a straight integer is that
	'0' is theoretically a valid intermediate value, but we would be testing for
	it and (incorrectly) setting it to "basis" again.

If called with NO data to hash, simply returns the initializer ;)
*/
uint64_t	fnv_hash64(uint64_t *hash, const void *data, size_t data_len)
{
	static const uint64_t prime = 1099511628211u;

	/* get start state, if NULL then initialize */
	uint64_t h;
	if (hash)
		h = *hash;
	else
		h = 14695981039346656037u;
	if (!data)
		return h;
	const uint8_t *d = data;

	/* hash the things! */
	for (size_t i=0; i < data_len; i++)
		h = (d[i] ^h) * prime;

	return h;
}

uint32_t	fnv_hash32(uint32_t *hash, const void *data, size_t data_len)
{
	static const uint32_t prime = 16777619u;

	/* get start state, if NULL then initialize */
	uint32_t h;
	if (hash)
		h = *hash;
	else
		h = 2166136261u;
	if (!data)
		return h;
	const uint8_t *d = data;

	/* hash the things! */
	for (size_t i=0; i < data_len; i++)
		h = (d[i] ^h) * prime;

	return h;
}
