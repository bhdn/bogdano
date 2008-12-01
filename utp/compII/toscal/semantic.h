#ifndef inc_semantic_h
#define inc_semantic_h

#include <stdio.h>

#include "string_list.h"
#include "symbols.h"
#include "parameters.h"
#include "type.h"
#include "codegen.h"

#define SEMANTIC_MAX_ERROR_ARG	BUFSIZ

#define ERROR	0
#define SUCCESS	1

enum semantic_error {
	SEMANTIC_SUCCESS,
	SEMANTIC_SYSTEM_ERROR,
	SEMANTIC_INVALID_TYPE,
	SEMANTIC_ALREADY_DEFINED,
	SEMANTIC_UNDEFINED_SYMBOL,
	SEMANTIC_INVALID_TYPE_CONVERSION,
	SEMANTIC_WRONG_NUMBER_PARAMETERS,
	SEMANTIC_INVALID_PARAMETER_TYPE,
	SEMANTIC_INVALID_FUNC_ASSIGNMENT,
	SEMANTIC_INVALID_CALLABLE,
	SEMANTIC_INVALID_COND_TYPE,
	SEMANTIC_INVALID_BYREF_ARG,
	SEMANTIC_CONST_ASSIGN_ERROR,
	SEMANTIC_CODEGEN_ERROR
};

enum semantic_warnings {
	SEMANTIC_CONVERSION_DATA_LOSS,
	SEMANTIC_SPARE_VARIABLE,
	SEMANTIC_USING_NOT_INITIALIZED,
	SEMANTIC_STRANGE_NEGATIVE
};

/* Note when writting a simple compiler: don't do it, don't repeat yourself */
/* Note when writting a complex compiler: do it, looks cool */

enum semantic_cmp_operators {
	SEMANTIC_CMP_EQUAL,
	SEMANTIC_CMP_DIFFERENT,
	SEMANTIC_CMP_GREATERTHAN,
	SEMANTIC_CMP_GREATEREQTHAN,
	SEMANTIC_CMP_LESSTHAN,
	SEMANTIC_CMP_LESSEQTHAN
};

enum semantic_bool_operators {
	SEMANTIC_BOOL_AND,
	SEMANTIC_BOOL_OR,
	SEMANTIC_BOOL_NOT, 
};

/** This bizarre struct is used to hold "semantic-specific" values inside
 * the stack of the parser code.
 */
typedef struct {
	union {
		struct symbol *symbol;
		struct type *type;	
		struct {
			parameters_iter_t iter;
			size_t current;
		}params;
		codegen_cond_t cond;
		codegen_while_t while_;
		codegen_repeat_t repeat;
	};
}sem_ref_t;

struct semantic_state {
	enum semantic_error error;
	char error_arg[SEMANTIC_MAX_ERROR_ARG];

	struct symbol_table *symbols;
	enum scope_types scope;

	int byref_pending;

	struct type *types;
	size_t ntypes;

	struct symbol *proc;
	struct symbol *main_proc;

	struct codegen_state *codegen;

	FILE *warning_stream;
	FILE *debug_stream;
};

void semantic_dump_error(struct semantic_state *ss, FILE *stream);
struct semantic_state *init_semantic_state();
void sem_warning(struct semantic_state *ss, enum semantic_warnings type,
		const char *warn_arg);
void semantic_set_error(struct semantic_state *ss,
		enum semantic_error error, const char *error_arg);
int sem_decl_const_int(struct semantic_state *ss, const char *name,
		size_t size, int value);
int sem_decl_const_char(struct semantic_state *ss, const char *name,
		size_t size, char value);
int sem_decl_const_real(struct semantic_state *ss, const char *name,
		size_t size, float real);
int sem_decl_var_list(struct semantic_state *ss, struct string_list *names,
		sem_ref_t *rval);
int sem_find_type(struct semantic_state *ss, const char *name,
		size_t size, sem_ref_t *rval);
int sem_hold_var(struct semantic_state *ss, const char *name, size_t size,
		sem_ref_t *hold);
int sem_call_function(struct semantic_state *ss, sem_ref_t *var,
		sem_ref_t *holdret);
int sem_var_assignment(struct semantic_state *ss, sem_ref_t *var,
		sem_ref_t *rval);
int sem_get_var(struct semantic_state *ss, sem_ref_t *var, 
		sem_ref_t *rval);
int sem_put_integer(struct semantic_state *ss, int value,
		sem_ref_t *rval);
int sem_put_real(struct semantic_state *ss, float value,
		sem_ref_t *rval);
int sem_put_char(struct semantic_state *ss, char value,
		sem_ref_t *rval);
int sem_sum_values(struct semantic_state *ss, sem_ref_t *left,
		sem_ref_t *right, sem_ref_t *rval);
int sem_subt_values(struct semantic_state *ss, sem_ref_t *left,
		sem_ref_t *right, sem_ref_t *rval);
int sem_mul_values(struct semantic_state *ss, sem_ref_t *left,
		sem_ref_t *right, sem_ref_t *rval);
int sem_div_values(struct semantic_state *ss, sem_ref_t *left,
		sem_ref_t *right, sem_ref_t *rval);
int sem_relcmp_values(struct semantic_state *ss,
		enum semantic_cmp_operators operator,
		sem_ref_t *left, sem_ref_t *right,
		sem_ref_t *rval);
int sem_boolcmp_values(struct semantic_state *ss,
		enum semantic_bool_operators operator,
		sem_ref_t *left, sem_ref_t *right,
		sem_ref_t *rval);
int sem_decl_procedure(struct semantic_state *ss,
		const char *name, size_t size,
		sem_ref_t *var);
int sem_decl_function(struct semantic_state *ss,
		const char *name, size_t size,
		sem_ref_t *var);
int sem_function_type(struct semantic_state *ss,
		sem_ref_t *var, sem_ref_t *rval);
int sem_begin_expr_list(struct semantic_state *ss,
		sem_ref_t *var, sem_ref_t *exprlist, sem_ref_t *ref);
int sem_expr_list_item(struct semantic_state *ss,
		sem_ref_t *var, sem_ref_t *exprlist,
		sem_ref_t *rval, sem_ref_t *ref);
int sem_end_expr_list(struct semantic_state *ss,
		sem_ref_t *var, sem_ref_t *exprlist);
int sem_finish_params(struct semantic_state *ss, 
		sem_ref_t *rval);
int sem_finish_procedure(struct semantic_state *ss, 
		sem_ref_t *rval);
int sem_init_program(struct semantic_state *ss, const char *name,
		size_t size);
int sem_finish_program(struct semantic_state *ss);
int sem_begin_code_block(struct semantic_state *ss);
int sem_begin_params(struct semantic_state *ss, sem_ref_t *rval);
int sem_funcall_prolog(struct semantic_state *ss, sem_ref_t *var);
int sem_funcall_cleanup(struct semantic_state *ss, sem_ref_t *var, 
		int leftover);
int sem_cond_prolog(struct semantic_state *ss, sem_ref_t *holdpos);
int sem_cond_eval(struct semantic_state *ss, sem_ref_t *rval,
		sem_ref_t *holdpos);
int sem_cond_else(struct semantic_state *ss, sem_ref_t *holdpos);
int sem_cond_epilog(struct semantic_state *ss, sem_ref_t *holdpos);
int sem_invert_value(struct semantic_state *ss, sem_ref_t *rval);
int sem_while_prolog(struct semantic_state *ss, sem_ref_t *holdpos);
int sem_while_eval(struct semantic_state *ss, sem_ref_t *rval,
		sem_ref_t *holdpos);
int sem_while_epilog(struct semantic_state *ss, sem_ref_t *holdpos);

int sem_repeat_prolog(struct semantic_state *ss, sem_ref_t *holdpos);
int sem_repeat_eval(struct semantic_state *ss, sem_ref_t *rval,
		sem_ref_t *holdpos);

int sem_read_var(struct semantic_state *ss, sem_ref_t *var);
int sem_write_value(struct semantic_state *ss, sem_ref_t *rval);

int sem_decl_label(struct semantic_state *ss, const char *name,
		size_t size);
int sem_inst_label(struct semantic_state *ss, sem_ref_t *var);
int sem_goto_label(struct semantic_state *ss, sem_ref_t *var);

int sem_not_value(struct semantic_state *ss, sem_ref_t *left,
		sem_ref_t *rval);

int sem_set_byref_param(struct semantic_state *ss);
int sem_set_byval_param(struct semantic_state *ss);
int sem_check_ref(struct semantic_state *ss, sem_ref_t *expritem,
		sem_ref_t *ref, sem_ref_t *rval, int *ignore);

int sem_mod_values(struct semantic_state *ss, sem_ref_t *left,
		sem_ref_t *right, sem_ref_t *rval);

#endif /* inc_semantic_h */
