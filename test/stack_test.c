#include <zed_dbg.h>
#include <stack.h>
#include <time.h> /* clock() */
#include <stdlib.h> /* getenv() */

/*	test_many()
*/
Z_ret_t	test_many(STACK_MEM_TYPE numiter)
{
	Z_ret_t err_cnt = 0;
	struct stack_t *stk = NULL;
	Z_die_if(!(stk = stack_new()),
		"");

	/* push to stack sequentially */
	for (STACK_MEM_TYPE i=0; i < numiter; i++) {
		STACK_MEM_TYPE ret = stack_push(&stk, i);
		Z_die_if(ret != i,
			"ret %zu != i %zu",
			ret, i);
	}
	
	/* pop off stack; verify sequence */
	STACK_MEM_TYPE pop, remain;
	while ( (remain = stack_pop(stk, &pop)) ) {
		Z_die_if(--numiter != remain,
			"--numiter %zu != remain %zu",
			numiter, remain);
		Z_die_if(pop != remain,
			"pop %zu != remain %zu",
			pop, remain);
	}		
		
out:
	stack_free(stk);
	return err_cnt;
}


#undef Z_LOG_LVL
#define Z_LOG_LVL (Z_err | Z_inf | Z_wrn)

/*	main()
*/
int main()
{
	int err_cnt = 0;

	/* do MUCH less work if VALGRIND environment variable is set */
	int shifts = 4;
	if (!(getenv("VALGRIND")))
		shifts = 21;
	Z_log(Z_err, "env: '%s'", getenv("VALGRIND"));

	STACK_MEM_TYPE numiter = 128;
	for (int i=0; i < shifts; i++, numiter <<= 1) {
		clock_t elapsed = clock();
		err_cnt += test_many(numiter);
		elapsed = clock() - elapsed;
		Z_log(Z_inf, "%zu stack pushes in %fs",
			numiter, (double)elapsed / CLOCKS_PER_SEC);
	}

	return err_cnt;
}
