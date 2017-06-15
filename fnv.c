#include "fnv.h"

/*	fnv_hash_l()
Perform, or continue, a 64-bit FNV1A hash.

If 'hash' is NULL, then begin the hash with 'basis' and move on from there.
Otherwise, load *hash and continue hashing.

The reason for a uint64_t* input as opposed to a straight integer is that
	'0' is theoretically a valid intermediate value, but we would be testing for
	it and (incorrectly) setting it to "basis" again.

If called with NO data to hash, simply returns the initializer ;)
*/
uint64_t	fnv_hash_l(uint64_t *hash, const void *data, uint64_t data_len)
{
	static const uint64_t prime = 1099511628211ul;

	/* get start state, if null then initialize */
	uint64_t h;
	if (hash)
		h = *hash;
	else
		h = 14695981039346656037ul;
	if (!data)
		return h;
	const uint8_t *d = data;

	/* Assume a high percentage of data is 64-bit aligned.
	Unroll loop accordingly.
	*/
	uint64_t i = 0;
	uint64_t numiter = data_len & ~0x7ul;
	while (i < numiter) {
		h = (d[i++] ^h) * prime;
		h = (d[i++] ^h) * prime;
		h = (d[i++] ^h) * prime;
		h = (d[i++] ^h) * prime;
		h = (d[i++] ^h) * prime;
		h = (d[i++] ^h) * prime;
		h = (d[i++] ^h) * prime;
		h = (d[i++] ^h) * prime;
	}
	/* odd number of bytes at end */
	while (i < data_len)
		h = (d[i++] ^h) * prime;

	return h;
}

uint32_t	fnv_hash(uint32_t *hash, const void *data, uint64_t data_len)
{
	static const uint32_t prime = 16777619;

	/* get start state, if null then initialize */
	uint32_t h;
	if (hash)
		h = *hash;
	else
		h = 2166136261u;
	if (!data)
		return h;
	const uint8_t *d = data;

	/* Assume a high percentage of data is 64-bit aligned.
	Unroll loop accordingly.
	*/
	uint64_t i = 0;
	uint64_t numiter = data_len & ~0x7ul;
	while (i < numiter) {
		h = (d[i++] ^h) * prime;
		h = (d[i++] ^h) * prime;
		h = (d[i++] ^h) * prime;
		h = (d[i++] ^h) * prime;
		h = (d[i++] ^h) * prime;
		h = (d[i++] ^h) * prime;
		h = (d[i++] ^h) * prime;
		h = (d[i++] ^h) * prime;
	}
	/* odd number of bytes at end */
	while (i < data_len)
		h = (d[i++] ^h) * prime;

	return h;
}
