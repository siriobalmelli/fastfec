#include "hx2b.h"


/*	hex_burn_leading_()
Ignore any leading '0{0,1}[xh]' sequences in a hex string.
Be careful - leading '0' may be '0xaf' or may just be '0a' (!)

returns (possibly shifted) beginning of string.
*/
const char *hex_burn_leading_(const char *hex)
{
	if (hex[0] == '0' && (hex[1] == 'x' || hex[1] == 'h') )
		hex += 2;
	else if (hex[0] == 'x' || hex[0] == 'h')
		hex++;
	return hex;	
}


/*	hex_parse_nibble_()
Returns value of parsed nibble, -1 on error.
*/
uint8_t hex_parse_nibble_(const char *hex)
{
	/* We take advantage of the fact that sets of characters appear in
		the ASCII table in sequence ;)
	*/
	switch (*hex) {
		case '0'...'9':
			return (*hex) & 0xf;
		case 'A'...'F':
		case 'a'...'f':
			return 9 + ((*hex) & 0xf);
		default:
			return -1;
	}


}


/*	hx2b()
Parse any quantity of hex digits into (at most) 'bytes' bytes.

Expected hex format: '0?[hx]?[0-9,a-f,A-F]+'

Ignores leading '0?[hx]?' sequence.
Parses in little-endian order; ignores TRAILING superfluous hex digits
	(e.g. for 'hex -> 0x11223344' and 'bytes = 2'  :  out[0] = 0x22; out[1] = 0x11)

memset()s 'bytes' output bytes before writing.

Returns numbers of characters/nibbles parsed (excluding ignored leading sequence);
	0 is a valid value if nothing parsed.
*/
size_t hx2b(const char *hex, uint8_t *out, size_t bytes)
{
	/* Burn any leading characters */
	hex = hex_burn_leading_(hex);

	/* How many nibbles are there ACTUALLY?
		... we may be parsing a 64-bit int expressed as only 1 character (LSnibble).
	Therefore, we must parse backwards.
	*/
	int nibble_cnt = 0;
	while (nibble_cnt < bytes * 2) {
		switch (*hex) {
			/* valid hex: check next nibble */
			case '0'...'9':
			case 'A'...'F':
			case 'a'...'f':
				nibble_cnt++;
				hex++;
				break;
			default:
				goto out_while;
		}
	}
out_while:
	/* last symbol was either bad or out of bounds. backtrack */
	hex--;

	/* first clean. then parse */
	memset(out, 0x0, bytes);

	/* parse all the nibbles, from the BACK (LSn of LSB == i=0) */
	for (int i=0; i < nibble_cnt; i++, hex--) {
		uint8_t res = hex_parse_nibble_(hex);
		/* odd nibble == MSn; requires shift */
		if (i & 0x1)
			res <<= 4;
		/* y u bitshift? because 'nibble_cnt / 2 = byte_cnt' */
		out[i >> 1] += res;
	}

	return nibble_cnt;
}
