/** codegen.c
 *
 * It generates code for the MEPA VM, cool enough for now.
 *
 * It provides an interface based on a stack based VM (ie. no
 * register-based-x86-friendly interface.)
 */
#include <stdlib.h>
#include <stdio.h>
#include <stdarg.h>
#include <string.h>
#include <errno.h>

#include "codegen.h"

#define ERROR	0
#define OK	1

/* aligned with enum codegen_relcmp */
static const char *relcmp_mnemonics[] = {
	"CMIG", /* equal */
	"CMDG", /* different */
	"CMMA", /* greater than */
	"CMAG",  /* greater or equal than */
	"CMME", /* less than */
	"CMEG" /* less or equal than */
};

const char *scope_names[] = { "global", "param", "local" };

void codegen_dump_error(struct codegen_state *cs, FILE *stream)
{
	switch (cs->error) {
	case CODEGEN_NOERROR:
		fputs("WTF? no pending error!", stream);
		break;
	case CODEGEN_WRITE_ERROR:
		fprintf(stream, "write error: %s\n", strerror(errno));
		break;
	}
}

void codegen_set_error(struct codegen_state *cs, enum codegen_error error)
{
	cs->error = error;
}

struct codegen_state *init_codegen_state(FILE *out)
{
	struct codegen_state *cs;

	cs = (struct codegen_state*) malloc(sizeof(struct codegen_state));
	if (!cs)
		return NULL;

	cs->out = out;
	cs->error = CODEGEN_NOERROR;
	cs->next_global_addr = 0;
	cs->next_local_addr = 0;
	cs->next_label = 0;

	return cs;
}

void destroy_codegen_state(struct codegen_state *cs)
{
	free(cs);
}

int codegenf(struct codegen_state *cs, const char *format, ...)
{
	va_list ap;

	va_start(ap, format);
	
	if (!cs->out)
		return OK;

	if (vfprintf(cs->out, format, ap) < 0) {
		codegen_set_error(cs, CODEGEN_WRITE_ERROR);
		return ERROR;
	}

	if (fputc('\n', cs->out) == EOF) {
		codegen_set_error(cs, CODEGEN_WRITE_ERROR);
		return ERROR;
	}

	return OK;
}

int codegen_program_prolog(struct codegen_state *cs)
{
	return codegenf(cs, "INPP\nDSVS _start");
}

int codegen_program_epilog(struct codegen_state *cs)
{
	return codegenf(cs, "PARA");
}

int codegen_begin_main_block(struct codegen_state *cs)
{
	return codegenf(cs, "_start:");
}

int codegen_procedure_prolog(struct codegen_state *cs,
		struct codegen_object *obj, int k)
{
	return codegenf(cs, "L%lu:\n"
			    "ENPR %d", obj->address, k);
}

int codegen_procedure_epilog(struct codegen_state *cs,
		int k, size_t params_offset, size_t locals_offset)
{
	/* TODO it should dealloc based on the variable size! */
	if (locals_offset &&
	    !codegenf(cs, "DMEM %lu\t\t; dealloc locals", locals_offset))
		return ERROR;

	return codegenf(cs, "RTPR %d, %lu", k, params_offset);
}

int codegen_push_const(struct codegen_state *cs, int value)
{
	return codegenf(cs, "CRCT %d", value);
}

int codegen_push_const_int(struct codegen_state *cs, int value)
{
	return codegen_push_const(cs, value);
}

int codegen_push_const_char(struct codegen_state *cs, char value)
{
	return codegen_push_const(cs, value);
}

int codegen_push_const_real(struct codegen_state *cs, float value)
{
	/* Actually this is not supported at all, our virtual machine
	 * doesn't handle real numbers. The semantic code should warn about
	 * it for now. (FIXME) */
	return codegen_push_const(cs, value);
}

int codegen_sum_values(struct codegen_state *cs)
{
	/* problem: no knoledge about the values that have been pushed to
	 * the stack, with such interface it wouldn't be easy to port it to
	 * the so-dreamed i386. */
	return codegenf(cs, "SOMA");
}

int codegen_sub_values(struct codegen_state *cs)
{
	/* the same note as above */
	return codegenf(cs, "SUBT");
}

int codegen_or_values(struct codegen_state *cs)
{
	return codegenf(cs, "DISJ");
}

int codegen_and_values(struct codegen_state *cs)
{
	return codegenf(cs, "CONJ");
}

int codegen_mul_values(struct codegen_state *cs)
{
	return codegenf(cs, "MULT");
}

int codegen_div_values(struct codegen_state *cs)
{
	return codegenf(cs, "DIVI");
}

int codegen_mod_values(struct codegen_state *cs)
{
	return codegenf(cs, "MODU");
}

int codegen_relcmp_values(struct codegen_state *cs, enum codegen_relcmp op)
{
	return codegenf(cs, relcmp_mnemonics[op]);
}

int codegen_alloc_address(struct codegen_state *cs,
		struct codegen_object *obj)
{
	obj->address = cs->next_label++;
	return OK;
}

int codegen_alloc_object(struct codegen_state *cs,
		struct codegen_object *codeobj,
		enum codegen_objscope scope, int k, int ref)
{
	codeobj->type = CODEGEN_OBJ_INT; /* FIXME */
	int error;

	codeobj->scope = scope;
	codeobj->k = k;
	codeobj->ref = ref;

	switch (scope) {
	case CODEGEN_SCOPE_GLOBAL:
	case CODEGEN_SCOPE_LOCAL:
		codeobj->index = cs->next_local_addr++;
		break;
	case CODEGEN_SCOPE_PARAM:
		codeobj->index = cs->next_param_addr++;
		break;
	}

	return error;
}

int codegen_inst_object(struct codegen_state *cs,
		struct codegen_object *obj)
{
	int error;

	/* FIXME use object size */

	switch (obj->scope) {
	case CODEGEN_SCOPE_GLOBAL:
	case CODEGEN_SCOPE_LOCAL:
		error = codegenf(cs, "AMEM 1\t\t; local var");
		break;
	case CODEGEN_SCOPE_PARAM:
		error = codegenf(cs, "\t\t; allocated param var at %d",
				obj->index - CODEOBJ_ARGS_BP_OFFSET);
		break;
	}

	return error;
}

void codegen_set_params(struct codegen_state *cs, int count)
{
	cs->next_param_addr = -count;
}

static int objaddr(struct codegen_object *obj)
{
	int addr;

	if (obj->scope == CODEGEN_SCOPE_PARAM)
		addr = obj->index - CODEOBJ_ARGS_BP_OFFSET;
	else
		addr = obj->index;

	return addr;
}

static int objk(struct codegen_object *obj)
{
	return obj->k;
}

int codegen_fetch_object(struct codegen_state *cs,
		struct codegen_object *obj)
{
	int error;

	error = codegenf(cs, "CRVL %d, %d\t; %s var",
			objk(obj), objaddr(obj),
			scope_names[obj->scope]);
	return error;
}

int codegen_store_object(struct codegen_state *cs,
		struct codegen_object *obj)
{
	return codegenf(cs, "ARMZ %d, %d\t; %s var",
			objk(obj), objaddr(obj),
			scope_names[obj->scope]);
}

int codegen_fetch_ref(struct codegen_state *cs,
		struct codegen_object *obj)
{
	return codegenf(cs, "CRVI %d, %d", objk(obj), objaddr(obj));
}

int codegen_store_ref(struct codegen_state *cs,
		struct codegen_object *obj)
{
	return codegenf(cs, "ARMI %d, %d", objk(obj), objaddr(obj));
}

int codegen_put_ref(struct codegen_state *cs,
		struct codegen_object *obj)
{
	return codegenf(cs, "CREN %d, %d", objk(obj), objaddr(obj));
}

int codegen_funcall_prolog(struct codegen_state *cs,
		struct codegen_object *obj)
{
	/* TODO alloc the size needed by obj */
	return codegenf(cs, "AMEM 1");
}

int codegen_call_function(struct codegen_state *cs,
		struct codegen_object *obj, int k)
{
	return codegenf(cs, "CHPR L%lu, %d", obj->address, k);
}

int codegen_funcall_cleanup(struct codegen_state *cs,
		struct codegen_object *obj)
{
	/* TODO shrink to the size needed by obj */
	return codegenf(cs, "DMEM 1\t\t; func call remainings");
}

int codegen_cond_prolog(struct codegen_state *cs, codegen_cond_t *cond)
{
	cond->jump_label = cs->next_label++;
	return OK;
}

int codegen_cond_eval(struct codegen_state *cs, codegen_cond_t *cond)
{
	return codegenf(cs, "DSVF R%lu", cond->jump_label);
}

int codegen_cond_else(struct codegen_state *cs, codegen_cond_t *cond)
{
	int error;
	size_t next;

	next = cs->next_label++;
	error = codegenf(cs, "DSVS R%lu\n"
			     "R%lu:", next, cond->jump_label);
	cond->jump_label = next;

	return error;
}

int codegen_cond_epilog(struct codegen_state *cs, codegen_cond_t *cond)
{
	return codegenf(cs, "R%lu:", cond->jump_label);
}

int codegen_invert_value(struct codegen_state *cs)
{
	return codegenf(cs, "INVR");
}

int codegen_while_prolog(struct codegen_state *cs, codegen_while_t *while_)
{
	while_->loop_label = cs->next_label++;
	while_->leave_label = cs->next_label++;

	return codegenf(cs, "R%lu:", while_->loop_label);
}

int codegen_while_eval(struct codegen_state *cs, codegen_while_t *while_)
{
	return codegenf(cs, "DSVF R%lu", while_->leave_label);
}

int codegen_while_epilog(struct codegen_state *cs, codegen_while_t *while_)
{
	return codegenf(cs, "DSVS R%lu\n"
			    "R%lu:", while_->loop_label, while_->leave_label);
}

int codegen_repeat_prolog(struct codegen_state *cs, codegen_repeat_t *repeat)
{
	repeat->jump_label = cs->next_label++;

	return codegenf(cs, "R%lu:\t\t; repeat statement", repeat->jump_label);
}

int codegen_repeat_eval(struct codegen_state *cs, codegen_repeat_t *repeat)
{
	return codegenf(cs, "DSVF R%lu\t\t; until statement", repeat->jump_label);
}

int codegen_read_object(struct codegen_state *cs, struct codegen_object *obj)
{
	return codegenf(cs, "LEIT\n"
			    "ARMZ %d, %d\t; read %s var", 
			    objk(obj), objaddr(obj),
			    scope_names[obj->scope]);
}

int codegen_read_ref(struct codegen_state *cs, struct codegen_object *obj)
{
	return codegenf(cs, "LEIT\n"
			    "ARMI %d, %d\t; read ref param var", 
			    objk(obj), objaddr(obj));
}

int codegen_write_value(struct codegen_state *cs)
{
	return codegenf(cs, "IMPR");
}

int codegen_set_label(struct codegen_state *cs, struct codegen_object *obj)
{
	obj->address = cs->next_label++;
	return codegenf(cs, "\t\t; allocated label %lu", obj->address);
}

int codegen_inst_label(struct codegen_state *cs, struct codegen_object *obj,
		int k, size_t locals_offset)
{
	/* U is just a hint for "user-defined" */
	return codegenf(cs, "U%lu:\n"
			    "ENRT %d, %lu", obj->address, k, locals_offset);
}

int codegen_goto_label(struct codegen_state *cs, struct codegen_object *obj)
{
	return codegenf(cs, "DSVS U%lu", obj->address);
}

int codegen_goto_far_label(struct codegen_state *cs,
		struct codegen_object *obj, int srck, int dstk)
{
	return codegenf(cs, "DSVR U%lu, %d, %d", obj->address, dstk, srck);
}

int codegen_not_value(struct codegen_state *cs)
{
	return codegenf(cs, "NEGA");
}

int codegen_reset_locals(struct codegen_state *cs)
{
	cs->next_local_addr = 0;
	return OK;
}
