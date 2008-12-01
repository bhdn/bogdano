#include <stdio.h>

#include "string_list.h"
#include "hash.h"
#include "type.h"
#include "symbols.h"

#define SYMSCOPE(sym) \
	(sym->scope == SCOPE_LOCAL ? "local" : "global")

int main()
{
	hash_iter_t iter;
	struct symbol_table *st;
	struct symbol *sym;
	struct string_list *names;

	st = init_symbol_table();
	if (!st) {
		fprintf(stderr, "puts, failed to alloc st\n");
		return 1;
	}

	add_symbol(st, "joao", 4, TYPE_INTEGER, SYMTYPE_VAR, NULL, SCOPE_GLOBAL);
	add_symbol(st, "pedro", 5, TYPE_INTEGER, SYMTYPE_VAR, NULL, SCOPE_GLOBAL);
	add_symbol(st, "maria", 5, TYPE_INTEGER, SYMTYPE_VAR, NULL, SCOPE_GLOBAL);
	add_symbol(st, "ilia", 4, TYPE_INTEGER, SYMTYPE_VAR, NULL, SCOPE_GLOBAL);
	add_symbol(st, "dvar", 4, TYPE_INTEGER, SYMTYPE_VAR, NULL, SCOPE_LOCAL);
	add_symbol(st, "glag", 4, TYPE_INTEGER, SYMTYPE_VAR, NULL, SCOPE_LOCAL);
	add_symbol(st, "dvoz", 4, TYPE_INTEGER, SYMTYPE_VAR, NULL, SCOPE_LOCAL);
	
	for_each_hash_value(st->symbols, iter, sym) {
		printf("symbol added: %s\n", sym->name);
	}

	purge_locals(st);
	printf("purged locals\n");

	for_each_hash_value(st->symbols, iter, sym)
		printf("symbol: %s, scope: %s\n", sym->name, SYMSCOPE(sym));

	names = create_string_list();
	if (!names) {
		fprintf(stderr, "puts, failed string_list\n");
		return 2;
	}

	string_list_add(names, "a", 1);
	string_list_add(names, "cond", 4);
	string_list_add(names, "blargh", 6);
	string_list_add(names, "puts", 4);

	add_symbols(st, names, "integer", 7, SYMTYPE_VAR, SCOPE_LOCAL);
	printf("add symbols\n");

	for_each_hash_value(st->symbols, iter, sym)
		printf("symbol: %s, scope: %s\n", sym->name, SYMSCOPE(sym));

	purge_locals(st);
	printf("purged locals again\n");

	for_each_hash_value(st->symbols, iter, sym)
		printf("symbol: %s, scope: %s\n", sym->name, SYMSCOPE(sym));

	destroy_string_list(names);
	destroy_symbol_table(st);
}
