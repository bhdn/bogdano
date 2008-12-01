/** semantic.c
 *
 * The semantic checker code.
 *
 * Most of the functions here are called by the parser as soon as some data
 * in the syntatic checker is available, such as a list of variables to be
 * declared or a variable being used, and so on.
 *
 * If you want to understand how one of these functions is used, check out
 * the parser code that calls it.
 *
 */
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <assert.h>

#include "semantic.h"
#include "string_list.h"
#include "symbols.h"
#include "type.h"
#include "parameters.h"
#include "codegen.h"

#define ERROR	0
#define OK	1	

void semantic_dump_error(struct semantic_state *ss, FILE *stream)
{
	switch (ss->error) {
	case SEMANTIC_SUCCESS:
		fputs("WTF? there is no pending error!", stream);
		break;

	case SEMANTIC_SYSTEM_ERROR:
		fprintf(stream, "system error: %s\n", strerror(errno));
		break;

	case SEMANTIC_INVALID_TYPE:
		fprintf(stream, "invalid type: %s\n", ss->error_arg);
		break;

	case SEMANTIC_ALREADY_DEFINED:
		fprintf(stream, "symbol already defined: %s\n", ss->error_arg);
		break;

	case SEMANTIC_UNDEFINED_SYMBOL:
		fprintf(stream, "referenced an undefined symbol: %s\n", ss->error_arg);
		break;

	case SEMANTIC_INVALID_TYPE_CONVERSION:
		fprintf(stream, "invalid type conversion between %s\n", ss->error_arg);
		break;

	case SEMANTIC_WRONG_NUMBER_PARAMETERS:
		fprintf(stream, "wrong number of arguments for %s\n", ss->error_arg);
		break;

	case SEMANTIC_INVALID_PARAMETER_TYPE:
		fprintf(stream, "invalid parameter type: %s\n", ss->error_arg);
		break;

	case SEMANTIC_INVALID_FUNC_ASSIGNMENT:
		fprintf(stream, "cannot assign value to function: %s\n", ss->error_arg);
		break;

	case SEMANTIC_INVALID_CALLABLE:
		fprintf(stream, "the symbol is not a function or "
				"procedure: %s\n", ss->error_arg);
		break;

	case SEMANTIC_INVALID_COND_TYPE:
		fprintf(stream, "invalid type %s as an conditional expression\n",
				ss->error_arg);
		break;

	case SEMANTIC_CONST_ASSIGN_ERROR:
		fprintf(stream, "can't assign to a constant: %s\n",
				ss->error_arg);
		break;

	case SEMANTIC_INVALID_BYREF_ARG:
		fprintf(stream, "invalid symbol passed by reference: %s\n",
				ss->error_arg);
		break;

	case SEMANTIC_CODEGEN_ERROR:
		fputs("code generator error: ", stream);
		codegen_dump_error(ss->codegen, stream);
		break;
	}
}

struct semantic_state *init_semantic_state(struct codegen_state *codegen)
{
	struct semantic_state *ss;

	ss = (struct semantic_state*) malloc(sizeof(struct semantic_state));
	if (!ss)
		return NULL;

	ss->symbols = init_symbol_table();
	if (!ss) {
		free(ss);
		return NULL;
	}

	ss->codegen = codegen;
	ss->scope = SCOPE_LOCAL;
	ss->error = SEMANTIC_SUCCESS;
	ss->error_arg[0] = '\0';
	ss->byref_pending = 0;
	ss->types = default_scalar_types;
	ss->ntypes = NUM_SCALAR_TYPES;
	ss->main_proc = NULL;
	ss->proc = NULL;
	ss->warning_stream = NULL;
	ss->debug_stream = NULL;

	return ss;
}

void destroy_semantic_state(struct semantic_state *ss)
{
	destroy_symbol_table(ss->symbols);
	free(ss);
}

void semantic_set_error(struct semantic_state *ss,
		enum semantic_error error, const char *error_arg)
{
	ss->error = error;
	if (error_arg)
		strncpy(ss->error_arg, error_arg, SEMANTIC_MAX_ERROR_ARG);
}

void sem_warning(struct semantic_state *ss, enum semantic_warnings type,
		const char *warn_arg)
{
	FILE *ws;

	ws = ss->warning_stream;
	if (!ws)
		return;

	/* FIXME warnings without line number are almost meaningless!! */

	fputs("warning: ", ws);

	switch (type) {
	case SEMANTIC_CONVERSION_DATA_LOSS:
		fprintf(ws, "possible data loss in conversion between %s\n",
				warn_arg);
		break;
	case SEMANTIC_SPARE_VARIABLE:
		fprintf(ws, "unused variable on %s\n", warn_arg);
		break;

	case SEMANTIC_STRANGE_NEGATIVE:
		fprintf(ws, "strange inversion of %s\n", warn_arg);
		break;

	case SEMANTIC_USING_NOT_INITIALIZED:
		fprintf(ws, "using variable not initialized: %s\n",
				warn_arg);
		break;
	}
}

int sem_alloc_codeobj(struct semantic_state *ss, struct symbol *sym)
{
	/* TODO array support */
	/* TODO global variables? */
	enum codegen_objscope scope;

	if (ss->scope == SCOPE_LOCAL)
		scope = CODEGEN_SCOPE_LOCAL;
	else /* SCOPE_PARAMS */
		scope = CODEGEN_SCOPE_PARAM;
		
	if (!codegen_alloc_object(ss->codegen, &sym->codeobj, scope,
				sym->lexscope,
				sym->symtype == SYMTYPE_REF)) {
		semantic_set_error(ss, SEMANTIC_CODEGEN_ERROR, NULL);
		return ERROR;
	}

	return OK;
}

int sem_decl_sym_list(struct semantic_state *ss, struct string_list *names,
		struct type *type, enum symbol_types symtype)
{
	char *name;
	size_t size;
	struct symbol *sym;
	string_list_iter_t iter;

	if (ss->byref_pending)
		/* the following symbols should be parameters that will be
		 * passed by reference */
		symtype = SYMTYPE_REF;

	string_list_foreach(names, iter, name, size) {
		sym = symbol_table_get(ss->symbols, name, size);
		if (sym) {
			semantic_set_error(ss, SEMANTIC_ALREADY_DEFINED, name);
			return ERROR;
		}
		sym = add_symbol(ss->symbols, name, size, symtype,
				ss->scope, type, type->reference,
				ss->proc);
		if (!sem_alloc_codeobj(ss, sym))
			return ERROR;
		sym->lexscope = ss->proc->lexscope;
		if (ss->scope == SCOPE_PARAMS) {
			/* if we are inside the declaration of the
			 * parameters of a function/procedure, add them to
			 * a paramter list so that we don't lose the order
			 * between them */
			if (!parameters_add(ss->proc->parameters, sym)) {
				semantic_set_error(ss, SEMANTIC_SYSTEM_ERROR, NULL);
				return ERROR;
			}
		}
		else
			ss->proc->locals++;
		if (!sym) {
			semantic_set_error(ss, SEMANTIC_SYSTEM_ERROR, NULL);
			return ERROR;
		}
	}

	return OK;
}

int sem_decl_var_list(struct semantic_state *ss, struct string_list *names,
		sem_ref_t *rval)
{
	return sem_decl_sym_list(ss, names, rval->type, SYMTYPE_VAR);
}

static struct symbol *sem_decl_const(struct semantic_state *ss,
		const char *name, size_t size, struct object value)
{
	struct symbol *sym;
	struct type *type;

	sym = symbol_table_get(ss->symbols, name, size);
	if (sym) {
		semantic_set_error(ss, SEMANTIC_ALREADY_DEFINED, name);
		return NULL;
	}

	type = &ss->types[value.type];

	sym = add_symbol(ss->symbols, name, size, SYMTYPE_CONST,
			ss->scope, type, value, ss->proc);
	if (!sym) {
		semantic_set_error(ss, SEMANTIC_SYSTEM_ERROR, NULL);
		return NULL;
	}

	return sym;
}

int sem_decl_const_int(struct semantic_state *ss, const char *name,
		size_t size, int value)
{
	struct object objvalue;

	objvalue.type = TYPE_INTEGER;
	objvalue.scalar.integer = value;

	if (!sem_decl_const(ss, name, size, objvalue))
		return ERROR;

	return OK;
}

int sem_decl_const_char(struct semantic_state *ss, const char *name,
		size_t size, char value)
{
	struct object objvalue;

	objvalue.type = TYPE_CHAR;
	objvalue.scalar.ch = value;

	if (!sem_decl_const(ss, name, size, objvalue))
		return ERROR;

	return OK;
}

int sem_decl_const_real(struct semantic_state *ss, const char *name,
		size_t size, float value)
{
	struct object objvalue;

	objvalue.type = TYPE_REAL;
	objvalue.scalar.ch = value;

	if (!sem_decl_const(ss, name, size, objvalue))
		return ERROR;

	return OK;
}

int sem_find_type(struct semantic_state *ss, const char *name,
		size_t size, sem_ref_t *rval)
{
	struct type *found;

	found = parse_scalar_type_name(ss->types, ss->ntypes, name, size);
	if (found->reference.type == TYPE_INVALID) {
		semantic_set_error(ss, SEMANTIC_INVALID_TYPE, name);
		return ERROR;
	}
	rval->type = found;

	return OK;
}

int sem_hold_var(struct semantic_state *ss, const char *name, size_t size,
		sem_ref_t *hold)
{
	struct symbol *sym;

	sym = symbol_table_get(ss->symbols, name, size);
	if (!sym) {
		semantic_set_error(ss, SEMANTIC_UNDEFINED_SYMBOL, name);
		return ERROR;
	}
	sym->referenced = 1;
	hold->symbol = sym;

	return OK;
}

int sem_funcall_prolog(struct semantic_state *ss, sem_ref_t *var)
{
	if (var->symbol->symtype == SYMTYPE_PROCEDURE)
		/* We don't need to generated prolog code for procedures */
		return OK;
	else if (var->symbol->symtype != SYMTYPE_FUNCTION) {
		semantic_set_error(ss, SEMANTIC_INVALID_CALLABLE,
				var->symbol->name);
		return ERROR;
	}
	if (!codegen_funcall_prolog(ss->codegen,
				&var->symbol->codeobj)) {
		semantic_set_error(ss, SEMANTIC_CODEGEN_ERROR, NULL);
		return ERROR;
	}

	return OK;
}

int sem_funcall_cleanup(struct semantic_state *ss, sem_ref_t *var, 
		int leftover)
{
	/* Function calls that leave results that are not consumed from the
	 * stack should be cleaned. */
	if (leftover && var->symbol->symtype == SYMTYPE_FUNCTION)
		if (!codegen_funcall_cleanup(ss->codegen,
					&var->symbol->codeobj)) {
			semantic_set_error(ss, SEMANTIC_CODEGEN_ERROR, NULL);
			return ERROR;
		}

	return OK;
}

int sem_call_function(struct semantic_state *ss, sem_ref_t *var,
		sem_ref_t *holdret)
{
	/* _prolog and _cleanup functions will take care of the remaining
	 * differences between functions and procedures. */
	holdret->type = var->symbol->type;

	if (!codegen_call_function(ss->codegen, &var->symbol->codeobj,
				ss->proc->lexscope)) {
		semantic_set_error(ss, SEMANTIC_CODEGEN_ERROR, NULL);
		return ERROR;
	}

	return OK;
}

static int check_func_assign(struct semantic_state *ss, struct symbol *symbol)
{
	/* Don't allow assigning a function when we are not in its own
	 * scope of code */
	if (symbol->symtype == SYMTYPE_FUNCTION &&
			ss->proc != symbol) {
		semantic_set_error(ss, SEMANTIC_INVALID_FUNC_ASSIGNMENT,
				symbol->name);
		return ERROR;
	}

	return OK;
}

int sem_var_assignment(struct semantic_state *ss, sem_ref_t *var,
		sem_ref_t *rval)
{
	char msg[BUFSIZ];

	if (var->symbol->symtype == SYMTYPE_CONST) {
		semantic_set_error(ss, SEMANTIC_CONST_ASSIGN_ERROR,
				var->symbol->name);
		return ERROR;
	}

	if (var->symbol->type != rval->type) {
		sprintf(msg, "%s and %s", var->symbol->type->name,
				rval->type->name);
		semantic_set_error(ss, SEMANTIC_INVALID_TYPE_CONVERSION,
				msg);
		return ERROR;
	}

	if (!check_func_assign(ss, var->symbol))
		return ERROR;

	var->symbol->initialized = 1;

	if (var->symbol->symtype == SYMTYPE_REF) {
		if (!codegen_store_ref(ss->codegen, &var->symbol->codeobj)) {
			semantic_set_error(ss, SEMANTIC_CODEGEN_ERROR, NULL);
			return ERROR;
		}
	}
	else if (!codegen_store_object(ss->codegen, &var->symbol->codeobj)) {
		semantic_set_error(ss, SEMANTIC_CODEGEN_ERROR, NULL);
		return ERROR;
	}

	return OK;
}

static int sem_get_const(struct semantic_state *ss, struct symbol *symbol)
{
	int error;

	switch (symbol->type->reference.type) {
	case TYPE_INTEGER:
		error = codegen_push_const_int(ss->codegen,
				symbol->value.scalar.integer);
		break;
	case TYPE_REAL:
		error = codegen_push_const_real(ss->codegen,
				symbol->value.scalar.real);
		break;
	case TYPE_CHAR:
		error = codegen_push_const_char(ss->codegen,
				symbol->value.scalar.ch);
		break;
	default:
		break;
	}

	if (!error) {
		semantic_set_error(ss, SEMANTIC_CODEGEN_ERROR, NULL);
		return ERROR;
	}

	return OK;
}

static int sem_get_ref(struct semantic_state *ss, struct symbol *symbol)
{
	if (!codegen_fetch_ref(ss->codegen, &symbol->codeobj)) {
		semantic_set_error(ss, SEMANTIC_CODEGEN_ERROR, NULL);
		return ERROR;
	}

	return OK;
}

int sem_get_var(struct semantic_state *ss, sem_ref_t *var, 
		sem_ref_t *rval)
{
	int error;
	char msg[BUFSIZ];
	struct symbol *symbol;

	symbol = var->symbol;
	rval->type = symbol->type;

	/* covers the case a function without parameters is referred in a
	 * expression */
	if (symbol->symtype == SYMTYPE_FUNCTION) {
		if (symbol->parameters->count > 0) {
			sprintf(msg, "%s (expected %d, found 0)", symbol->name,
					symbol->parameters->count);
			semantic_set_error(ss, SEMANTIC_WRONG_NUMBER_PARAMETERS,
					msg);
			return ERROR;
		}
		error = codegen_funcall_prolog(ss->codegen,
				&var->symbol->codeobj);
		if (error)
			error = codegen_call_function(ss->codegen,
					&var->symbol->codeobj,
					ss->proc->lexscope);
	}
	else if (symbol->symtype == SYMTYPE_CONST) {
		if (!sem_get_const(ss, symbol))
			return ERROR;
	}
	else if (symbol->symtype == SYMTYPE_REF) {
		if (!sem_get_ref(ss, symbol))
			return ERROR;
	}
	else {
		/* Warn about the variable being used without being
		 * initialized. */
		if (!symbol->initialized
				&& symbol->scope != SCOPE_PARAMS
				&& symbol->lexscope >= ss->proc->lexscope) {
			sprintf(msg, "%s on %s", symbol->name,
					ss->proc->name);
			sem_warning(ss, SEMANTIC_USING_NOT_INITIALIZED,
					msg);
		}

		error = codegen_fetch_object(ss->codegen,
				&var->symbol->codeobj);
	}

	if (!error)
		semantic_set_error(ss, SEMANTIC_CODEGEN_ERROR, NULL);

	return error;
}

int sem_put_char(struct semantic_state *ss, char value,
		sem_ref_t *rval)
{ 
	rval->type = &ss->types[TYPE_CHAR];

	if (!codegen_push_const_char(ss->codegen, value)) {
		semantic_set_error(ss, SEMANTIC_CODEGEN_ERROR, NULL);
		return ERROR;
	}

	return OK;
}

int sem_put_integer(struct semantic_state *ss, int value,
		sem_ref_t *rval)
{ 
	rval->type = &ss->types[TYPE_INTEGER];

	if (!codegen_push_const_int(ss->codegen, value)) {
		semantic_set_error(ss, SEMANTIC_CODEGEN_ERROR, NULL);
		return ERROR;
	}

	return OK;
}

int sem_put_real(struct semantic_state *ss, float value,
		sem_ref_t *rval)
{ 
	rval->type = &ss->types[TYPE_REAL];

	if (!codegen_push_const_real(ss->codegen, value)) {
		semantic_set_error(ss, SEMANTIC_CODEGEN_ERROR, NULL);
		return ERROR;
	}

	return OK;
}

enum object_types choose_type(enum object_types left, enum object_types right)
{
	enum object_types chosen;

	if (left == TYPE_REAL || right == TYPE_REAL)
		chosen = TYPE_REAL;
	else if ((left == TYPE_INTEGER || left == TYPE_CHAR) &&
	         (right == TYPE_INTEGER || right == TYPE_CHAR))
		chosen = TYPE_INTEGER;
	else
		chosen = TYPE_INVALID;

	return chosen;
}

int sem_value_type(struct semantic_state *ss, sem_ref_t *left,
		sem_ref_t *right, sem_ref_t *rval)
{
	enum object_types chosen;
	char msg[BUFSIZ];
		
	chosen = choose_type(left->type->reference.type,
			right->type->reference.type);
	if (chosen == TYPE_INVALID) {
		sprintf(msg, "%s and %s", left->type->name, right->type->name);
		semantic_set_error(ss, SEMANTIC_INVALID_TYPE_CONVERSION, msg);
		return ERROR;
	}

	rval->type = &ss->types[chosen];

	return OK;
}

int sem_sum_values(struct semantic_state *ss, sem_ref_t *left,
		sem_ref_t *right, sem_ref_t *rval)
{
	if (sem_value_type(ss, left, right, rval) == ERROR)
		return ERROR;

	if (!codegen_sum_values(ss->codegen)) {
		semantic_set_error(ss, SEMANTIC_CODEGEN_ERROR, NULL);
		return ERROR;
	}

	return OK;
}

int sem_subt_values(struct semantic_state *ss, sem_ref_t *left,
		sem_ref_t *right, sem_ref_t *rval)
{
	if (sem_value_type(ss, left, right, rval) == ERROR)
		return ERROR;

	if (!codegen_sub_values(ss->codegen)) {
		semantic_set_error(ss, SEMANTIC_CODEGEN_ERROR, NULL);
		return ERROR;
	}

	return OK;
}

int sem_mul_values(struct semantic_state *ss, sem_ref_t *left,
		sem_ref_t *right, sem_ref_t *rval)
{
	if (sem_value_type(ss, left, right, rval) == ERROR)
		return ERROR;

	if (!codegen_mul_values(ss->codegen)) {
		semantic_set_error(ss, SEMANTIC_CODEGEN_ERROR, NULL);
		return ERROR;
	}

	return OK;
}

int sem_div_values(struct semantic_state *ss, sem_ref_t *left,
		sem_ref_t *right, sem_ref_t *rval)
{
	if (sem_value_type(ss, left, right, rval) == ERROR)
		return ERROR;

	if (!codegen_div_values(ss->codegen)) {
		semantic_set_error(ss, SEMANTIC_CODEGEN_ERROR, NULL);
		return ERROR;
	}

	return OK;
}

/* It is not the specification of the language */
int sem_mod_values(struct semantic_state *ss, sem_ref_t *left,
		sem_ref_t *right, sem_ref_t *rval)
{
	if (sem_value_type(ss, left, right, rval) == ERROR)
		return ERROR;

	if (!codegen_mod_values(ss->codegen)) {
		semantic_set_error(ss, SEMANTIC_CODEGEN_ERROR, NULL);
		return ERROR;
	}

	return OK;
}

/* aligned with semantic_cmp_operators */
static const enum codegen_relcmp relcmp_codegen[] = {
	CODEGEN_RELCMP_EQUAL,
	CODEGEN_RELCMP_DIFFERENT,
	CODEGEN_RELCMP_GREATERTHAN,
	CODEGEN_RELCMP_GREATEREQTHAN,
	CODEGEN_RELCMP_LESSTHAN,
	CODEGEN_RELCMP_LESSEQTHAN
};

int sem_relcmp_values(struct semantic_state *ss,
		enum semantic_cmp_operators operator,
		sem_ref_t *left, sem_ref_t *right,
		sem_ref_t *rval)
{
	/* as we don't have a bool type, all relational operations will
	 * result in integer values */
	rval->type = &ss->types[TYPE_INTEGER];

	if (!codegen_relcmp_values(ss->codegen, relcmp_codegen[operator])) {
		semantic_set_error(ss, SEMANTIC_CODEGEN_ERROR, NULL);
		return ERROR;
	}

	return OK;
}

int sem_boolcmp_values(struct semantic_state *ss,
		enum semantic_bool_operators operator,
		sem_ref_t *left, sem_ref_t *right,
		sem_ref_t *rval)
{
	enum codegen_error error;

	rval->type = &ss->types[TYPE_INTEGER];

	if (operator == SEMANTIC_BOOL_OR)
		error = codegen_or_values(ss->codegen);
	else if (operator == SEMANTIC_BOOL_AND)
		error = codegen_and_values(ss->codegen);

	if (!error) {
		semantic_set_error(ss, SEMANTIC_CODEGEN_ERROR, NULL);
		return ERROR;
	}

	return OK;
}

int sem_not_value(struct semantic_state *ss, sem_ref_t *left,
		sem_ref_t *rval)
{
	rval->type = &ss->types[TYPE_INTEGER];

	if (!codegen_not_value(ss->codegen)) {
		semantic_set_error(ss, SEMANTIC_CODEGEN_ERROR, NULL);
		return ERROR;
	}

	return OK;
}

int sem_decl_procedure(struct semantic_state *ss,
		const char *name, size_t size,
		sem_ref_t *var)
{
	struct symbol *sym;

	sym = symbol_table_get(ss->symbols, name, size);
	if (sym) {
		semantic_set_error(ss, SEMANTIC_ALREADY_DEFINED, name);
		return ERROR;
	}

	sym = add_symbol(ss->symbols, name, size, SYMTYPE_PROCEDURE,
			ss->scope, &ss->types[TYPE_VOID],
			ss->types[TYPE_VOID].reference,
			ss->proc);
	if (!sym) {
		semantic_set_error(ss, SEMANTIC_SYSTEM_ERROR, NULL);
		return ERROR;
	}

	if (!codegen_alloc_address(ss->codegen, &sym->codeobj)) {
		semantic_set_error(ss, SEMANTIC_CODEGEN_ERROR, NULL);
		return ERROR;
	}

	if (!codegen_reset_locals(ss->codegen)) {
		semantic_set_error(ss, SEMANTIC_CODEGEN_ERROR, NULL);
		return ERROR;
	}

	/* add_symbol creates a symbol that inherits parent's lexscope,
	 * but procedures and functions should have lexscope + 1. */
	sym->lexscope = ss->proc->lexscope + 1;
	ss->proc = sym;

	sym->parameters = create_parameters();
	if (!sym->parameters) {
		semantic_set_error(ss, SEMANTIC_SYSTEM_ERROR, NULL);
		destroy_symbol(sym);
		return ERROR;
	}
	var->symbol = sym;

	return OK;
}

int sem_decl_function(struct semantic_state *ss,
		const char *name, size_t size,
		sem_ref_t *var)
{
	if (sem_decl_procedure(ss, name, size, var) == ERROR)
		return ERROR;

	var->symbol->symtype = SYMTYPE_FUNCTION;
	
	/* As it is a function, it will return something on -1. Set the
	 * default value */
	var->symbol->codeobj.scope = CODEGEN_SCOPE_PARAM;
	var->symbol->codeobj.index = -1;
	var->symbol->codeobj.k = var->symbol->lexscope;

	return OK;
}

int sem_function_type(struct semantic_state *ss,
		sem_ref_t *var, sem_ref_t *rval)
{
	var->symbol->type = rval->type;

	return OK;
}

int sem_check_ref(struct semantic_state *ss, sem_ref_t *expritem,
		sem_ref_t *ref, sem_ref_t *rval, int *ignore)
{
	struct symbol *sym = ref->symbol;

	*ignore = 1;
	ref->symbol = NULL;
	rval->type = NULL;
	
	if (!expritem->params.iter)
		/* Empty list of parameters, something is wrong, let
		 * expr_list_item handle it */
		return OK;

	if (expritem->params.iter->symbol->symtype != SYMTYPE_REF)
		return OK;

	if (sym->symtype != SYMTYPE_VAR) {
		semantic_set_error(ss, SEMANTIC_INVALID_BYREF_ARG,
				sym->name);
		return ERROR;
	}

	*ignore = 0;
	ref->symbol = sym;
	rval->type = sym->type;

	return OK;
}

/** sem_begin_expr_list
 *
 * It is called when the parser starts to check the parameters used to call
 * a function or procedure. It will setup the iterator that will be use to
 * check the type of each parameter being called.
 *
 * Each parameter itself will be passed to the function sem_expr_list_item,
 * which will really check the types.
 */
int sem_begin_expr_list(struct semantic_state *ss,
		sem_ref_t *var, sem_ref_t *exprlist, sem_ref_t *ref)
{
	parameters_begin_iter(var->symbol->parameters, &exprlist->params.iter);
	exprlist->params.current = 0;
	ref->symbol = NULL;
	return OK;
}

int sem_expr_list_item(struct semantic_state *ss,
		sem_ref_t *var, sem_ref_t *exprlist,
		sem_ref_t *rval, sem_ref_t *ref)
{
	char msg[BUFSIZ];
	struct symbol *sym;
	enum object_types chosen;

	exprlist->params.current++;

	sym = parameters_next_symbol(var->symbol->parameters,
			&exprlist->params.iter);
	if (!sym) {
		/* unexpected end of the parameters list */
		sprintf(msg, "%s (expected %d, found %d)", var->symbol->name,
				var->symbol->parameters->count,
				exprlist->params.current);
		semantic_set_error(ss, SEMANTIC_WRONG_NUMBER_PARAMETERS,
				msg);
		return ERROR;
	}

	chosen = choose_type(rval->type->reference.type,
			sym->type->reference.type);
	if (chosen == TYPE_INVALID) {
		sprintf(msg, "parameter %d (expected %s, found %s)",
				exprlist->params.current,
				sym->type->name, rval->type->name);
		semantic_set_error(ss, SEMANTIC_INVALID_PARAMETER_TYPE,
				msg);
		return ERROR;
	}
	else if (chosen != sym->type->reference.type) {
		sprintf(msg, "%s and %s", ss->types[chosen].name,
				sym->type->name);
		sem_warning(ss, SEMANTIC_CONVERSION_DATA_LOSS, msg);
	}

	if (ref->symbol) {
		/* A parameter to be passed by reference, we must push the
		 * address of ref on the stack. */
		assert(sym->symtype == SYMTYPE_REF);
		if (!codegen_put_ref(ss->codegen, &ref->symbol->codeobj)) {
			semantic_set_error(ss, SEMANTIC_CODEGEN_ERROR, NULL);
			return ERROR;
		}

		ref->symbol = NULL;
	}

	return OK;
}

int sem_end_expr_list(struct semantic_state *ss,
		sem_ref_t *var, sem_ref_t *exprlist)
{
	char msg[BUFSIZ];

	if (exprlist->params.current != var->symbol->parameters->count) {
		sprintf(msg, "%s (expected %d, found %d)", 
				var->symbol->name,
				var->symbol->parameters->count,
				exprlist->params.current);
		semantic_set_error(ss, SEMANTIC_WRONG_NUMBER_PARAMETERS,
				msg);
		return ERROR;
	}

	return OK;
}

/** sem_finish_params 
 *
 * Called when the parser sees there is no more parameters to be declared
 * for the function pointed by rval.
 */
int sem_finish_params(struct semantic_state *ss, 
		sem_ref_t *rval)
{
	parameters_iter_t iter;
	struct symbol *sym;
	int count;

	/* the codegen requires to set the reference value to variables
	 * based on their access relative to the Base Pointer value, which
	 * is negative as they are always placed before bp. */
	count = -rval->symbol->parameters->count;
	for_each_parameter(rval->symbol->parameters, iter, sym)
		sym->codeobj.index = count++;

	/* As now we know the number of parameters of this function, now we
	 * know the address of the return value of this (possibly one)
	 * function. 
	 *
	 * This code seems really intrusive and probably we should not be
	 * dealing with object scopes here, it probably should be moved to
	 * the codegen module.*/
	if (rval->symbol->symtype == SYMTYPE_FUNCTION)
		rval->symbol->codeobj.index = -rval->symbol->parameters->count - 1;

	ss->scope = SCOPE_LOCAL; /* from now on, all variables should be
			          * allocated on the stack */

	return OK;
}

int sem_begin_params(struct semantic_state *ss, sem_ref_t *rval)
{
	ss->scope = SCOPE_PARAMS; /* from now on, all variables are
				   * allocated by the caller of the
				   * function */
	return OK;
}

void sem_warn_unused_symbol(void *state, const char *context,
		const char *name)
{
	char msg[BUFSIZ];
	struct semantic_state *ss;

	ss = (struct semantic_state*) state;

	sprintf(msg, "%s: %s", context, name);
	sem_warning(ss, SEMANTIC_SPARE_VARIABLE, msg);
}

int sem_finish_procedure(struct semantic_state *ss, 
		sem_ref_t *var)
{
	struct symbol *proc = var->symbol;

	find_unreferenced_symbols(ss->symbols, var->symbol->lexscope,
			(void*) ss, proc->name, sem_warn_unused_symbol);

	if (!codegen_procedure_epilog(ss->codegen,
				proc->lexscope,
				proc->parameters->count,
				proc->locals)) {
		semantic_set_error(ss, SEMANTIC_CODEGEN_ERROR, NULL);
		return ERROR;
	}

	/* As have just finished to check this function, we can purge all
	 * the local variables created. The function parameters will be
	 * removed from the symbol table but they will not be destroyed
	 * because they will be used to check the parameters when calling
	 * the function. */
	deref_params(ss->symbols, ss->proc->lexscope);
	purge_locals(ss->symbols, ss->proc->lexscope);

	ss->proc = ss->proc->parent;

	return OK;
}

static struct symbol *alloc_main_procedure(struct semantic_state *ss, 
		const char *name, size_t size)
{
	struct symbol *sym;

	sym = add_symbol(ss->symbols, name, size, SYMTYPE_PROCEDURE,
			SCOPE_LOCAL, &ss->types[TYPE_VOID],
			ss->types[TYPE_VOID].reference, NULL);
	sym->codeobj.scope = CODEGEN_SCOPE_LOCAL;
	return sym;
}

int sem_init_program(struct semantic_state *ss, const char *name,
		size_t size)
{
	if (!codegen_program_prolog(ss->codegen)) {
		semantic_set_error(ss, SEMANTIC_CODEGEN_ERROR, NULL);
		return ERROR;
	}

	ss->main_proc = alloc_main_procedure(ss, name, size);
	if (!ss->main_proc) {
		semantic_set_error(ss, SEMANTIC_SYSTEM_ERROR, NULL);
		return ERROR;
	}
	ss->proc = ss->main_proc;

	return OK;
}

int sem_finish_program(struct semantic_state *ss)
{
	find_unreferenced_symbols(ss->symbols,
			ss->proc->lexscope,
			(void*) ss, ss->proc->name,
			sem_warn_unused_symbol);

	if (!codegen_program_epilog(ss->codegen)) {
		semantic_set_error(ss, SEMANTIC_CODEGEN_ERROR, NULL);
		return ERROR;
	}

	return OK;
}

static int sem_begin_procedure_code(struct semantic_state *ss)
{
	symbol_table_iter_t iter;
	struct symbol *sym;

	if (ss->proc != ss->main_proc)
		if (!codegen_procedure_prolog(ss->codegen,
					&ss->proc->codeobj,
					ss->proc->lexscope)) {
			semantic_set_error(ss, SEMANTIC_CODEGEN_ERROR, NULL);
			return ERROR;
		}

	for_each_symbol(ss->symbols, iter, sym)
		if (sym->lexscope == ss->proc->lexscope
				&& sym->symtype == SYMTYPE_VAR)
			if (!codegen_inst_object(ss->codegen, &sym->codeobj)) {
				semantic_set_error(ss, SEMANTIC_CODEGEN_ERROR, NULL);
				return ERROR;
			}

	return OK;
}

int sem_begin_code_block(struct semantic_state *ss)
{
	if (ss->proc == ss->main_proc)
		if (!codegen_begin_main_block(ss->codegen)) {
			semantic_set_error(ss, SEMANTIC_CODEGEN_ERROR, NULL);
			return ERROR;
		}

	sem_begin_procedure_code(ss);

	return OK;
}

int sem_cond_prolog(struct semantic_state *ss, sem_ref_t *holdpos)
{
	if (!codegen_cond_prolog(ss->codegen, &holdpos->cond)) {
		semantic_set_error(ss, SEMANTIC_CODEGEN_ERROR, NULL);
		return ERROR;
	}

	return OK;
}

int sem_cond_eval(struct semantic_state *ss, sem_ref_t *rval,
		sem_ref_t *holdpos)
{
	if (rval->type != &ss->types[TYPE_INTEGER]) {
		semantic_set_error(ss, SEMANTIC_INVALID_COND_TYPE,
				rval->type->name);
		return ERROR;
	}

	if (!codegen_cond_eval(ss->codegen, &holdpos->cond)) {
		semantic_set_error(ss, SEMANTIC_CODEGEN_ERROR, NULL);
		return ERROR;
	}

	return OK;
}

int sem_cond_else(struct semantic_state *ss, sem_ref_t *holdpos)
{
	if (!codegen_cond_else(ss->codegen, &holdpos->cond)) {
		semantic_set_error(ss, SEMANTIC_CODEGEN_ERROR, NULL);
		return ERROR;
	}

	return OK;
}

int sem_cond_epilog(struct semantic_state *ss, sem_ref_t *holdpos)
{
	if (!codegen_cond_epilog(ss->codegen, &holdpos->cond)) {
		semantic_set_error(ss, SEMANTIC_CODEGEN_ERROR, NULL);
		return ERROR;
	}

	return OK;
}

int sem_invert_value(struct semantic_state *ss, sem_ref_t *rval)
{
	enum object_types type;

	type = rval->type->reference.type;
	if (type != TYPE_INTEGER && type != TYPE_REAL)
		sem_warning(ss, SEMANTIC_STRANGE_NEGATIVE,
				rval->type->name);
			
	if (!codegen_invert_value(ss->codegen)) {
		semantic_set_error(ss, SEMANTIC_CODEGEN_ERROR, NULL);
		return ERROR;
	}

	return OK;
}

int sem_while_prolog(struct semantic_state *ss, sem_ref_t *holdpos)
{
	if (!codegen_while_prolog(ss->codegen, &holdpos->while_)) {
		semantic_set_error(ss, SEMANTIC_CODEGEN_ERROR, NULL);
		return ERROR;
	}

	return OK;
}

int sem_while_eval(struct semantic_state *ss, sem_ref_t *rval,
		sem_ref_t *holdpos)
{
	if (rval->type != &ss->types[TYPE_INTEGER]) {
		semantic_set_error(ss, SEMANTIC_INVALID_COND_TYPE,
				rval->type->name);
		return ERROR;
	}

	if (!codegen_while_eval(ss->codegen, &holdpos->while_)) {
		semantic_set_error(ss, SEMANTIC_CODEGEN_ERROR, NULL);
		return ERROR;
	}

	return OK;
}

int sem_while_epilog(struct semantic_state *ss, sem_ref_t *holdpos)
{
	if (!codegen_while_epilog(ss->codegen, &holdpos->while_)) {
		semantic_set_error(ss, SEMANTIC_CODEGEN_ERROR, NULL);
		return ERROR;
	}

	return OK;
}

int sem_repeat_prolog(struct semantic_state *ss, sem_ref_t *holdpos)
{
	if (!codegen_repeat_prolog(ss->codegen, &holdpos->repeat)) {
		semantic_set_error(ss, SEMANTIC_CODEGEN_ERROR, NULL);
		return ERROR;
	}

	return OK;
}

int sem_repeat_eval(struct semantic_state *ss, sem_ref_t *rval,
		sem_ref_t *holdpos)
{
	if (rval->type != &ss->types[TYPE_INTEGER]) {
		semantic_set_error(ss, SEMANTIC_INVALID_COND_TYPE,
				rval->type->name);
		return ERROR;
	}

	if (!codegen_repeat_eval(ss->codegen, &holdpos->repeat)) {
		semantic_set_error(ss, SEMANTIC_CODEGEN_ERROR, NULL);
		return ERROR;
	}

	return OK;
}

int sem_read_var(struct semantic_state *ss, sem_ref_t *var)
{
	int success = OK;
	struct symbol *sym = var->symbol;

	if (sym->symtype == SYMTYPE_FUNCTION) {
		if (!check_func_assign(ss, sym))
			return ERROR;
	}
	else if (sym->symtype == SYMTYPE_VAR)
		success = codegen_read_object(ss->codegen, &sym->codeobj);
	else if (sym->symtype == SYMTYPE_REF)
		success = codegen_read_ref(ss->codegen, &sym->codeobj);
	else {
		semantic_set_error(ss, SEMANTIC_CONST_ASSIGN_ERROR,
				sym->name);
		return ERROR;
	}

	if (!success) {
		semantic_set_error(ss, SEMANTIC_CODEGEN_ERROR, NULL);
		return ERROR;
	}

	sym->initialized = 1;

	return OK;
}

int sem_write_value(struct semantic_state *ss, sem_ref_t *rval)
{
	/* TODO use type information to know what to do for each type! */
	if (!codegen_write_value(ss->codegen)) {
		semantic_set_error(ss, SEMANTIC_CODEGEN_ERROR, NULL);
		return ERROR;
	}

	return OK;
}

int sem_decl_label(struct semantic_state *ss, const char *name,
		size_t size)
{
	struct symbol *sym;
	struct type *type;

	sym = symbol_table_get(ss->symbols, name, size);
	if (sym) {
		semantic_set_error(ss, SEMANTIC_ALREADY_DEFINED, name);
		return ERROR;
	}

	type = &ss->types[TYPE_VOID];
	sym = add_symbol(ss->symbols, name, size, SYMTYPE_LABEL, ss->scope,
			type, type->reference, ss->proc);
	if (!sym) {
		semantic_set_error(ss, SEMANTIC_SYSTEM_ERROR, NULL);
		return ERROR;
	}

	if (!codegen_set_label(ss->codegen, &sym->codeobj)) {
		semantic_set_error(ss, SEMANTIC_CODEGEN_ERROR, NULL);
		return ERROR;
	}

	return OK;
}

int sem_inst_label(struct semantic_state *ss, sem_ref_t *var)
{
	/* It should be ensured by the parser */
	assert(var->symbol->symtype == SYMTYPE_LABEL);

	if (!codegen_inst_label(ss->codegen, &var->symbol->codeobj,
				ss->proc->lexscope,
				ss->proc->locals)) {
		semantic_set_error(ss, SEMANTIC_CODEGEN_ERROR, NULL);
		return ERROR;
	}

	return OK;
}

int sem_goto_label(struct semantic_state *ss, sem_ref_t *var)
{
	/* It should be ensured by the parser */
	assert(var->symbol->symtype == SYMTYPE_LABEL);

	if (ss->proc->lexscope != var->symbol->lexscope) {
		/* Jumping out of the scope of this function requires
		 * unwinding the stack to the destionation scope. */
		if (!codegen_goto_far_label(ss->codegen,
				&var->symbol->codeobj,
				ss->proc->lexscope,
				var->symbol->lexscope)) {
			semantic_set_error(ss, SEMANTIC_CODEGEN_ERROR, NULL);
			return ERROR;
		}
	}
	else if (!codegen_goto_label(ss->codegen, &var->symbol->codeobj)) {
		semantic_set_error(ss, SEMANTIC_CODEGEN_ERROR, NULL);
		return ERROR;
	}

	return OK;
}

int sem_set_byref_param(struct semantic_state *ss)
{
	ss->byref_pending = 1;
	return OK;
}

int sem_set_byval_param(struct semantic_state *ss)
{
	ss->byref_pending = 0;
	return OK;
}
