#ifndef inc_symbols_h
#define inc_symbols_h

#include "string_list.h"
#include "hash.h"
#include "type.h"
#include "codegen.h"

#define SYMBOL_TABLE_HASH_SIZE	1024

enum scope_types {
	SCOPE_LOCAL,
	SCOPE_PARAMS
};

enum symbol_table_errors {
	SYMBOL_OK,
	SYMBOL_ADD_ERROR,
	SYMBOL_REDECLARED,
	SYMBOL_INVALID_TYPE
};

/* TODO consider moving all the object-related data to another structure
 * and refer it here just as an "object structure" */
struct symbol {
	char *name;
	size_t size;
	enum symbol_types symtype;
	enum scope_types scope;
	int lexscope;
	int initialized;
	int referenced; /* whether it has been referenced in the code or not */
	struct type *type;
	struct object value;
	struct codegen_object codeobj;

	struct parameters *parameters;
	size_t locals;

	struct symbol *parent;
};

struct symbol_table {
	struct hash_table *symbols;
};

struct symbol_table *init_symbol_table();
void destroy_symbol_table(struct symbol_table *st);
void destroy_symbol(struct symbol *sym);
struct symbol* add_symbol(struct symbol_table *st,
		const char *name, size_t size,
		enum symbol_types symtype, 
		enum scope_types scope,
		struct type *type,
		struct object value,
		struct symbol *parent);
struct symbol *symbol_table_get(struct symbol_table *st, const char *name,
		size_t size);
/* no need to drop individual symbols for now */
void purge_locals(struct symbol_table *st, int lexscope);
void deref_params(struct symbol_table *st, int lexscope);

typedef void (*symbol_warnf_t)(void*, const char *, const char *);
void find_unreferenced_symbols(struct symbol_table *st, int lexscope,
		void *state, const char *context, symbol_warnf_t warnf);

typedef hash_iter_t symbol_table_iter_t;

#define for_each_symbol(table, iter, sym) \
	for_each_hash_value(table->symbols, iter, sym)

#endif
