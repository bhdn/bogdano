#include <string.h>

#define CRAP
#include "type.h"

const struct type default_scalar_types[] = {
	{"<invalid>", 9, {.type = TYPE_INVALID}}, 
	{"<void>", 6, {.type = TYPE_VOID}},
	{"integer", 6, {.type = TYPE_INTEGER, {.scalar = {{.integer = 0}}}}},
	{"real", 4, {.type = TYPE_REAL, {.scalar = {{.real = 0.0}}}}},
	{"char", 4, {.type = TYPE_CHAR, {.scalar = {{.ch = 0}}}}},
};


struct type *parse_scalar_type_name(struct type *types, size_t ntypes,
		const char *name, size_t size)
{
	size_t i;

	for (i = 0; i < ntypes; i++)
		if (strncmp(name, types[i].name, size) == 0)
			return &types[i];

	return &types[TYPE_INVALID];
}

