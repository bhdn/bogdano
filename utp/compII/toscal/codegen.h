/** The codegen module provides an abstract layer to generate executable
 * machine code.
 *
 * Its interface relies on the concept of a stack-based virtual machine
 * (actually it is based on the MEPA VM, but we hope to be generic enough
 * to allow code generation to other archictectures as well.)
 *
 */
#ifndef inc_codegen_h
#define inc_codegen_h

#include <stdio.h>

/* The space in the stack memory between function parameters and the word
 * pointed by the base pointer, it contains the information saved from the
 * caller function. */
#define CODEOBJ_ARGS_BP_OFFSET	3 

enum codegen_error {
	CODEGEN_NOERROR,
	CODEGEN_WRITE_ERROR
};

enum codegen_relcmp {
	CODEGEN_RELCMP_EQUAL = 0,
	CODEGEN_RELCMP_DIFFERENT,
	CODEGEN_RELCMP_GREATERTHAN,
	CODEGEN_RELCMP_GREATEREQTHAN,
	CODEGEN_RELCMP_LESSTHAN,
	CODEGEN_RELCMP_LESSEQTHAN
};

enum codegen_objtype {
	CODEGEN_OBJ_INT,
	CODEGEN_OBJ_REAL,
	CODEGEN_OBJ_CHAR
};

enum codegen_objscope {
	CODEGEN_SCOPE_GLOBAL,
	CODEGEN_SCOPE_PARAM,
	CODEGEN_SCOPE_LOCAL
};

#define CODEGEN_INT_SIZE	1
#define CODEGEN_REAL_SIZE	1
#define CODEGEN_CHAR_SIZE	1

struct codegen_state {
	enum codegen_error error;
	FILE *out;
	size_t next_global_addr;
	int next_local_addr;
	int next_param_addr;
	size_t next_label;
};

typedef struct  {
	size_t jump_label;
}codegen_cond_t, codegen_repeat_t;

typedef struct {
	size_t loop_label;
	size_t leave_label;
}codegen_while_t;

struct codegen_object {
	enum codegen_objtype type;
	enum codegen_objscope scope;
	int k;
	int index;
	int ref;
	size_t address; /* for labels */
};

void codegen_dump_error(struct codegen_state *cs, FILE *stream);
struct codegen_state *init_codegen_state(FILE *out);
void destroy_codegen_state(struct codegen_state *cs);

int codegen_program_prolog(struct codegen_state *cs);
int codegen_program_epilog(struct codegen_state *cs);
int codegen_begin_main_block(struct codegen_state *cs);

int codegen_push_address(struct codegen_state *cs, size_t address);
int codegen_push_const_int(struct codegen_state *cs, int value);
int codegen_push_const_char(struct codegen_state *cs, char value);
int codegen_push_const_real(struct codegen_state *cs, float value);

int codegen_pop_address(struct codegen_state *cs, size_t address);

int codegen_procedure_prolog(struct codegen_state *cs, struct codegen_object *obj,
		int k);
int codegen_procedure_epilog(struct codegen_state *cs,
		int k, size_t params_offset, size_t locals_offset);

int codegen_cond_prolog(struct codegen_state *cs, codegen_cond_t *cond);
int codegen_cond_eval(struct codegen_state *cs, codegen_cond_t *cond);
int codegen_cond_else(struct codegen_state *cs, codegen_cond_t *cond);
int codegen_cond_epilog(struct codegen_state *cs, codegen_cond_t *cond);

int codegen_input(struct codegen_state *cs);
int codegen_output(struct codegen_state *cs);

int codegen_sum_values(struct codegen_state *cs);
int codegen_sub_values(struct codegen_state *cs);
int codegen_or_values(struct codegen_state *cs);
int codegen_and_values(struct codegen_state *cs);
int codegen_mul_values(struct codegen_state *cs);
int codegen_div_values(struct codegen_state *cs);
int codegen_mod_values(struct codegen_state *cs);
int codegen_relcmp_values(struct codegen_state *cs, enum codegen_relcmp op);

int codegen_alloc_object(struct codegen_state *cs,
		struct codegen_object *codeobj,
		enum codegen_objscope scope, int k, int ref);
int codegen_inst_object(struct codegen_state *cs,
		struct codegen_object *obj);
void codegen_set_params(struct codegen_state *cs, int count);

int codegen_fetch_object(struct codegen_state *cs,
		struct codegen_object *obj);
int codegen_store_object(struct codegen_state *cs,
		struct codegen_object *obj);
int codegen_call_function(struct codegen_state *cs,
		struct codegen_object *obj, int k);
int codegen_funcall_prolog(struct codegen_state *cs,
		struct codegen_object *obj);
int codegen_funcall_cleanup(struct codegen_state *cs,
		struct codegen_object *obj);
int codegen_funcall_cleanup(struct codegen_state *cs,
		struct codegen_object *obj);
int codegen_invert_value(struct codegen_state *cs);

int codegen_while_prolog(struct codegen_state *cs, codegen_while_t *while_);
int codegen_while_eval(struct codegen_state *cs, codegen_while_t *while_);
int codegen_while_epilog(struct codegen_state *cs, codegen_while_t *while_);

int codegen_repeat_prolog(struct codegen_state *cs, codegen_repeat_t *repeat);
int codegen_repeat_eval(struct codegen_state *cs, codegen_repeat_t *repeat);

int codegen_read_object(struct codegen_state *cs, struct codegen_object *obj);
int codegen_read_ref(struct codegen_state *cs, struct codegen_object *obj);
int codegen_write_value(struct codegen_state *cs);

int codegen_set_label(struct codegen_state *cs, struct codegen_object *obj);
int codegen_inst_label(struct codegen_state *cs, struct codegen_object *obj,
		int k, size_t locals_offset);
int codegen_goto_label(struct codegen_state *cs, struct codegen_object *obj);
int codegen_goto_far_label(struct codegen_state *cs,
		struct codegen_object *obj, int srck, int dstk);

int codegen_not_value(struct codegen_state *cs);

int codegen_alloc_address(struct codegen_state *cs, struct codegen_object *obj);
int codegen_reset_locals(struct codegen_state *cs);

int codegen_fetch_ref(struct codegen_state *cs,
		struct codegen_object *obj);
int codegen_store_ref(struct codegen_state *cs,
		struct codegen_object *obj);
int codegen_put_ref(struct codegen_state *cs,
		struct codegen_object *obj);
#endif
