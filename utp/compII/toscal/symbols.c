#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include "string_list.h"
#include "symbols.h"
#include "parameters.h"
#include "hash.h"

/* Crappy memory allocation scheme:
 *
 * - global symbols are destroyed along with the symbol table;
 * - local symbols (local variables only) are destroyed when the parser
 *   ends reading/checking the function/procedure;
 * - function parameters (SCOPE_PARAMS) are freed along with the symbol
 *   table (ie. in the same moment globals are freed) , BUT they can still
 *   be referenced by ->parameters of functions.
 */

struct symbol_table *init_symbol_table()
{
	struct symbol_table *st;

	st = (struct symbol_table*) malloc(sizeof(struct symbol_table));
	if (!st)
		return NULL;

	st->symbols = hash_init(SYMBOL_TABLE_HASH_SIZE);
	if (!st->symbols) {
		free(st);
		return NULL;
	}

	return st;
}

void destroy_symbol(struct symbol *sym)
{
	parameters_iter_t iter;
	struct symbol *psym;
	
	free(sym->name);
	if (sym->parameters) {
		for_each_parameter(sym->parameters, iter, psym)
			destroy_symbol(psym);
		destroy_parameters(sym->parameters);
	}
	free(sym);
}

void destroy_symbol_table(struct symbol_table *st)
{
	hash_iter_t iter;
	struct symbol *sym;

	for_each_hash_value(st->symbols, iter, sym)
		destroy_symbol(sym);
	hash_free(st->symbols);
	free(st);
}

/** Gets the symbol structure from a given symbol name */
struct symbol *symbol_table_get(struct symbol_table *st, const char *name,
		size_t size)
{
	struct symbol *sym;

	sym = (struct symbol*) hash_get(st->symbols, name, size, 0);

	return sym;
}

/** Adds a new symbol to the symbol table
 *
 * Note it doesn't check whether it already exists in the hash table, it
 * will overwrite one that already exists.
 */
struct symbol* add_symbol(struct symbol_table *st,
		const char *name, size_t size,
		enum symbol_types symtype, 
		enum scope_types scope,
		struct type *type,
		struct object value,
		struct symbol *parent)
{
	struct symbol *sym;

	sym = (struct symbol*) malloc(sizeof(struct symbol));
	if (!sym)
		goto error;

	sym->name = (char*) malloc(size + 1);
	if (!sym->name)
		goto error_name;
	strncpy(sym->name, name, size + 1);
	sym->size = size;

	sym->symtype = symtype;
	sym->scope = scope;
	sym->lexscope = 0;
	sym->initialized = 0;
	sym->referenced = 0;
	sym->type = type;
	sym->value = value;

	sym->parameters = NULL;
	sym->locals = 0;

	sym->parent = parent;
	if (parent)
		sym->lexscope = parent->lexscope;

	if (!hash_put(st->symbols, name, size, sym, 0))
		goto error_name;

	return sym;

error_name:
	free(sym->name);
error:
	free(sym);
	return NULL;
}

/* no need to drop individual symbols for now */
void purge_locals(struct symbol_table *st, int lexscope)
{
	hash_iter_t iter;
	struct symbol *sym;

	for_each_hash_value(st->symbols, iter, sym)
		if (sym->scope == SCOPE_LOCAL && sym->lexscope == lexscope
				&& sym->symtype != SYMTYPE_FUNCTION
		  		&& sym->symtype != SYMTYPE_PROCEDURE) {
			hash_pop(st->symbols, sym->name, sym->size, 0);
			destroy_symbol(sym);
		}
}

/* Removes the references to the function parameters symbols, but doesn't
 * destroy them. */
void deref_params(struct symbol_table *st, int lexscope)
{
	hash_iter_t iter;
	struct symbol *sym;

	for_each_hash_value(st->symbols, iter, sym)
		if (sym->scope == SCOPE_PARAMS && sym->lexscope == lexscope)
			hash_pop(st->symbols, sym->name, sym->size, 0);
}

void find_unreferenced_symbols(struct symbol_table *st, int lexscope, void *state,
		const char *context, symbol_warnf_t warnf)
{

	hash_iter_t iter;
	struct symbol *sym;

	for_each_hash_value(st->symbols, iter, sym)
		if (sym->lexscope == lexscope
				&& sym->symtype != SYMTYPE_FUNCTION
				&& sym->symtype != SYMTYPE_PROCEDURE
				&& !sym->referenced)
			warnf(state, context, sym->name);
}
