/*
 * slist.h
 * singly linked list implementation
 */
#include <stdlib.h>
#include <stdbool.h>

#define list_consume(l, fn) for (void *h; (h = list_pop(l)); fn(h));

struct list {
	void                *head;
	struct list         *tail;
};

typedef struct list *list_t;

static bool list_next(struct list **, void **);

/* O(1). Return the list head. */
static inline void *list_head(struct list *l)
{
	return l->head;
}

/* O(1). Push an element to the front of the list. */
static void list_push(struct list **tail, void *head)
{
	struct list *l = (struct list *)malloc(sizeof(*l));

	l->tail = *tail;
	l->head = head;

	*tail = l;
}

/* O(1). Pop an element from the front of the list. */
static void *list_pop(struct list **l)
{
	if (! *l) return NULL;

	struct list *k = *l;
	void        *h   = k->head;

	*l = (*l)->tail;
	free(k);

	return h;
}

/* O(n). Remove an element from the list. Returns true if the element was
 * removed. */
static bool list_remove(struct list **l, void *e)
{
	struct list *k    = *l;
	struct list *prev = NULL;

	while (k) {
		if (k->head == e) {
			if (prev) {
				prev->tail = k->tail;
				free(k);
			} else {
				list_pop(l);
			}
			return true;
		}
		prev = k;
		k = k->tail;
	}
	return false;
}

/* Iterate over a list. Returns false when there are no more elements. */
static bool list_next(struct list **l, void **h)
{
	if (*l && (*l)->head) {
		*h = (*l)->head;
		*l = (*l)->tail;
		return true;
	}
	return false;
}

/* O(n). Return the number of elements in the list. */
static size_t list_length(struct list *l)
{
	size_t len = 0;

	while (l) {
		len ++;
		l = l->tail;
	}
	return len;
}

/* O(n). Free the list. */
static void list_free(struct list *l)
{
	if (! l) return;

	struct list *k;

	do {
		k = l;
		l = l->tail;
		free(k);
	} while (l);
}

