#ifndef inc_parameters_h
#define inc_parameters_h

#include "symbols.h"

struct parameter {
	struct symbol *symbol;
	struct parameter *next;
};

typedef struct parameter* parameters_iter_t;

struct parameters {
	size_t count;	
	struct parameter *first;
	struct parameter *last;
};

struct parameters *create_parameters();
struct parameter *parameters_add(struct parameters *p, struct symbol *symbol);
void destroy_parameters(struct parameters *p);
void parameters_begin_iter(struct parameters *p, parameters_iter_t *iter);
struct symbol *parameters_next_symbol(struct parameters *p,
		parameters_iter_t *iter);

#define for_each_parameter(params, iter, sym) \
	for ((iter = params->first);\
	     (sym = parameters_next_symbol(params, &iter));)

#endif /* inc_parameters_h */
