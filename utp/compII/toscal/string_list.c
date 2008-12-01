#include <string.h>
#include <stdlib.h>

#include "string_list.h"

struct string_list *create_string_list()
{
	struct string_list *sl;

	sl = (struct string_list*) malloc(sizeof(struct string_list));
	if (!sl)
		return NULL;
	sl->first = NULL;
	sl->last = NULL;

	return sl;
}

char *string_list_add(struct string_list *sl, const char *value, size_t size)
{
	struct string_item *item;

	item = (struct string_item*) malloc(sizeof(struct string_item));
	if (!item)
		return NULL;

	item->value = (char*) malloc(size + 1);
	if (!item->value) {
		free(item);
		return NULL;
	}
	item->size = size;
	item->next = NULL;

	strncpy(item->value, value, size + 1);

	if (sl->last)
		sl->last->next = item;
	sl->last = item;
	if (!sl->first)
		sl->first = item;

	return item->value;
}

void destroy_string_list(struct string_list *sl)
{
	struct string_item *current, *next;

	current = sl->first;
	while (current) {
		next = current->next;
		free(current->value);
		free(current);
		current = next;
	}
	free(sl);
}
