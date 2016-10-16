/*	pointers.c
Solution to the problem of "if" test when adding/removing elements to a
	doubly-linked list ... without using '**indirect' al-la-Torvalds ;)

(c) 2016 Sirio Balmelli
*/

#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>

struct elem {
	struct elem	*prev; /* must coincide with 'head->last' */
	struct elem	*next;
	/* elem-specific data from here */
	char		data[32];
};

struct head {
	/* The gimmick is to match pointer positions between 'elem' and 'head':
		prev	<->	last
		next	<->	first
	Then, when initializing 'head', just have it point to itself - see setup().
	*/
	struct elem	*last;
	struct elem	*first;
	/* head-specific data from here */
	int		count;
};

static struct head *head = NULL;


struct elem *el_new()
{
	struct elem *ret = calloc(1, sizeof(struct elem));
	/* The previous "tail" is our "previous",
		and we are their "next".
	NOTE: (parentheses) are only for legibility.
	*/
	ret->prev = head->last;
	(ret->prev)->next = ret;

	/* We always insert at "tail", so
		our "next" is to loop back to the head.
	*/
	ret->next = (struct elem *)head;
	head->last = ret;

	/* head accounting and return */
	head->count++;
	return ret;
}

void el_del(struct elem *del)
{
	/* pretty straightforward */
	(del->next)->prev = del->prev;
	(del->prev)->next = del->next;

	head->count--;
	free(del);
}


__attribute__((constructor)) void setup()
{
	head = malloc(sizeof(struct head));
	/* Force a cast here, so that head can be treated like just another cell
		when adding or deleting.
	*/
	head->last = head->first = (struct elem *)head;
	head->count = 0;
}
__attribute__((destructor)) void teardown()
{
	while (head->count)
		el_del(head->last);
	free(head);
}


int main()
{
	struct elem *one = el_new();
	el_del(one);
	one = NULL;

	return 0;
}
