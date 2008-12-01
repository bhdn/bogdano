#ifndef inc_type_h
#define inc_type_h

#include <string.h>

#define NR_TYPES	6

enum object_types {
	TYPE_INVALID = 0,
	TYPE_VOID,
	TYPE_INTEGER,
	TYPE_REAL,
	TYPE_CHAR,
	TYPE_STRING
};

enum symbol_types {
	SYMTYPE_VAR,
	SYMTYPE_CONST,
	SYMTYPE_PROCEDURE,
	SYMTYPE_FUNCTION,
	SYMTYPE_LABEL,
	SYMTYPE_REF
};

struct scalar_value {
	union {
		char ch;
		int integer;
		float real;
	};
};

struct object {
	enum object_types type;
	union {
		struct scalar_value scalar;
		/* string? arrays? */
	};
};

struct type {
	char *name;
	size_t name_size;
	struct object reference;
};

#ifndef CRAP
extern struct type default_scalar_types[];
#endif

#define NUM_SCALAR_TYPES	5

struct type *parse_scalar_type_name(struct type *types, size_t ntypes,
		const char *name, size_t size);

#endif
