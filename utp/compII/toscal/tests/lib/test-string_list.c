#include <stdio.h>
#include "string_list.h"

main()
{
	size_t size;
	char *value;
	struct string_list *sl;
	string_list_iter_t iter;

	sl = create_string_list();

	printf("no iter\n");
	string_list_foreach(sl, iter, value, size)
		printf("iter: %s - %u\n", value, size);
	printf("end no iter\n");

	string_list_add(sl, "foo", 3);
	string_list_add(sl, "bar", 3);
	string_list_add(sl, "baz", 3);
	string_list_add(sl, "bliz", 4);

	printf("iter 4 items\n");
	string_list_foreach(sl, iter, value, size)
		printf("iter: %s - %u\n", value, size);
	printf("end iter\n");
	destroy_string_list(sl);
}
