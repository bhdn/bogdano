#ifndef inc_slist_h
#define inc_slist_h

struct string_item {
	char *value;
	size_t size;
	struct string_item *next;
};

struct string_list {
	struct string_item *first;
	struct string_item *last;
};

/* just to not leak a implementation detail to the users of the
 * string_list_foreach macro */
typedef struct string_item* string_list_iter_t;

struct string_list *create_string_list();
char *string_list_add(struct string_list *sl, const char *value,
		size_t size);
/* no need to remove items from the list, at least for now */
void destroy_string_list(struct string_list *sl);

#define string_list_foreach(sl, iter, ptr, sizeval) \
	  for (iter = sl->first, \
			ptr = iter ? iter->value : 0, \
			sizeval = iter ? iter->size : 0; \
		iter; \
		  iter = iter->next, \
		       ptr = iter ? iter->value : 0, \
		       sizeval = iter ? iter->size : 0)

#endif
