#include <stdio.h>
#include <stdlib.h>
#include "zed_dbg.h"

/* put this at the start of any block to force the debug level */
#ifdef Z_BLK_LVL
#undef Z_BLK_LVL
#endif
#define Z_BLK_LVL 2

void some_func(int foo)
{
	Z_inf(2, "foo = %d", foo);
}

/* put this at the footer of any such section */
#undef Z_BLK_LVL
#define Z_BLK_LVL 0

void some_other_func(int foo)
{
	Z_inf(2, "foo = %d", foo);
}

int main()
{
	int err_cnt = 0;
	Z_log_info("test info: %d", 42);

	/* selective debug */
	Z_inf(2, "shouldn't INF");
	some_func(3); // SHOULD inf
	some_other_func(9); // should NOT inf

	Z_inf(9, "debug info");

	int error = 5;
	Z_err_if(error == 5, "bad");

	int really_bad = 1;
	Z_die_if(really_bad, "");
out:
	return 0;
}
