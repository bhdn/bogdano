#include <stdlib.h>

#include "parameters.h"
#include "symbols.h"

struct parameters *create_parameters()
{
	struct parameters *p;

	p = (struct parameters*) malloc(sizeof(struct parameters));
	if (!p)
		return NULL;

	p->count = 0;
	p->first = NULL;
	p->last = NULL;

	return p;
}

struct parameter *parameters_add(struct parameters *p,
		struct symbol *symbol)
{
	struct parameter *new;

	new = (struct parameter*) malloc(sizeof(struct parameter));
	if (!new)
		return NULL;

	new->symbol = symbol;
	new->next = NULL;

	if (!p->first)
		p->first = new;
	if (p->last)
		p->last->next = new;
	p->last = new;

	p->count++;

	return new;
}

void parameters_begin_iter(struct parameters *p, parameters_iter_t *iter)
{
	*iter = p->first;
}

struct symbol *parameters_next_symbol(struct parameters *p,
		parameters_iter_t *iter)
{
	struct parameter *param;
	struct symbol *symbol;

	param = *iter;

	if (!param)
		return NULL; /* end of the parameter list */

	symbol = param->symbol;
	*iter = param->next;

	return symbol;
}

void destroy_parameters(struct parameters *p)
{
	struct parameter *current, *next;

	current = p->first;
	while (current) {
		next = current->next;
		free(current);
		current = next;
	}
	free(p);
}
