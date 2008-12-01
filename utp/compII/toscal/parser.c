/** 
 * parser.c - The analyzer lives here
 *
 * It just fetches tokens from a given input source and tries to check if
 * they match with the language proposed to our task.
 *
 * (Yes, it could be implemented as a table of actions and gotos, as in 
 * http://en.wikipedia.org/wiki/LR_parser)
 *
 * The language a simplified version of Pascal. Nothing special here.
 */

#include <stdio.h>
#include <string.h>
#include <errno.h>

#include "input.h"
#include "tokenize.h"
#include "parser.h"
#include "symbols.h"
#include "string_list.h"

#define NEXT_TOKEN	do { \
	if (!fetch_next_token(ps->input, &ps->current)) { \
		if (ps->current.type != TOK_EOF) { \
			parser_error(ps, PARSER_TOKENIZE_ERROR, NULL); \
			return ERROR; \
		} \
	} \
	if (ps->dump_tokens) \
		dump_token(&ps->current, stdout); \
	if (ps->debug_stream) \
		fprintf(ps->debug_stream,\
			"fetched %s at %s\n", ps->current.name, __FUNCTION__); \
	} while(0)

#define EXPECT_TOKEN(_type)	do { \
	if (ps->current.type != _type) { \
		parser_error(ps, PARSER_UNEXPECTED_TOKEN, #_type); \
		return ERROR; \
	} \
	if (ps->debug_stream) \
		fprintf(ps->debug_stream,\
			"ensured %s at %s\n", #_type, __FUNCTION__); \
	} while (0)

#define EXPECT_STATE(function)	do {\
	if (ps->debug_stream) \
		fprintf(ps->debug_stream, "on state: " #function "\n");\
	if (function(ps) == ERROR) \
		return ERROR;\
	if (ps->debug_stream) \
		fprintf(ps->debug_stream, "leaving " #function "\n"); \
	} while (0)

#define EXPECT_STATE_VALUE(function, arg)	do {\
	if (ps->debug_stream) \
		fprintf(ps->debug_stream, "on state: " #function "\n");\
	if (function(ps, arg) == ERROR) \
		return ERROR;\
	if (ps->debug_stream) \
		fprintf(ps->debug_stream, "leaving " #function "\n"); \
	} while (0)

#define IS_RELATIONALOP(token_type) \
	(token_type == TOK_EQUAL \
	    || token_type == TOK_DIFFERENT \
	    || token_type == TOK_LESSTHAN \
	    || token_type == TOK_LESSEQTHAN \
	    || token_type == TOK_GREATERTHAN \
	    || token_type == TOK_GREATEREQTHAN)

#define SEMANTIC_HOOK(fcall) do { \
	if (ps->semantic->debug_stream)\
		fprintf(ps->semantic->debug_stream, "%s\n", #fcall); \
	if (ps->semantic_check && fcall == ERROR) { \
		parser_error(ps, PARSER_SEMANTIC_ERROR, NULL); \
		return ERROR; \
	} } while(0)

#define NEGVAL(x, val) (x ? -val : val)

#define ERROR	0
#define OK	1


int state_S(struct parser_state *ps);

/**
 * Writes a parser error message in a given stream
 *
 * TODO try to use the information into the input state to find out what
 * kind of error really happened.
 */
void parser_dump_error(struct parser_state *ps, FILE *stream)
{
	fputs("error: ", stream);

	switch (ps->error) {
	case PARSER_UNEXPECTED_TOKEN:
		input_dump_position(ps->input, stream);
		fprintf(stream, ": unexpected token %s, expected %s\n",
				ps->current.name, ps->expected);
		break;

	case PARSER_READ_ERROR:
		fprintf(stream, "read error: %s\n", strerror(errno));
		break;

	case PARSER_SYSTEM_ERROR:
		fprintf(stream, "system error: %s\n", strerror(errno));
		break;

	case PARSER_TOKENIZE_ERROR:
		fprintf(stream, "tokenizer error: ");
		tokenizer_dump_error(&ps->current, ps->input, stream);
		break;

	case PARSER_SEMANTIC_ERROR:
		input_dump_position(ps->input, stream);
		fprintf(stream, ": semantic error: ");
		semantic_dump_error(ps->semantic, stream);
		break;

	case PARSER_SUCCESS:
		fputs("uh?\n", stream);
		break;

	}
}

struct parser_state *init_parser_state(struct input_state *input,
		struct semantic_state *semantic)
{
	struct parser_state *ps;

	ps = (struct parser_state*) malloc(sizeof(struct parser_state));
	if (!ps)
		return NULL;

	ps->input = input;
	ps->semantic = semantic;
	ps->debug_stream = NULL;
	ps->dump_tokens = 0;
	ps->semantic_check = 1;

	return ps;
}

/**
 * Sets the error information in the parser_state structure.
 *
 * If the error message is NULL, the default 'invalid token'-like one will
 * be used (!!).
 */
void parser_error(struct parser_state *ps, enum error_type error,
		char *expected)
{
	if (ps->current.type == TOK_READ_ERROR)
		ps->error = PARSER_READ_ERROR;
	else if (ps->current.type == TOK_PARSE_ERROR)
		ps->error = PARSER_TOKENIZE_ERROR;
	else {
		ps->error = error;
		ps->expected = expected;
	}
}

/**
 * Checks whether a given input_source matches with our pascal-like
 * language.
 */
int parser_check(struct parser_state *ps)
{
	NEXT_TOKEN; /* check the description of state_S about it */

	EXPECT_STATE(state_S);
	EXPECT_TOKEN(TOK_EOF); /* shall we? */
	return OK;
}

/** 
 * Here begins the implementation of the states of the grammar.
 *
 * It contains some helper macros to help implementing the states with a
 * clearer code (not very much).
 */

int state_VariableArrayIndexing(struct parser_state *ps)
{
	EXPECT_TOKEN(TOK_OPENINGBRACKET);
	NEXT_TOKEN;
	EXPECT_TOKEN(TOK_INTEGER);
	NEXT_TOKEN;
	EXPECT_TOKEN(TOK_CLOSINGBRACKET);
	NEXT_TOKEN;
	return OK;
}

int state_Type(struct parser_state *ps, sem_ref_t *rval)
{
	EXPECT_TOKEN(TOK_IDENTIFIER);
	SEMANTIC_HOOK(sem_find_type(ps->semantic, ps->current.repr,
				ps->current.pending, rval));
	NEXT_TOKEN;
	return OK;
}

int state_VariablesList(struct parser_state *ps)
{
	struct string_list *names;
	sem_ref_t rval;

	names = create_string_list();
	if (!names) {
		parser_error(ps, PARSER_SYSTEM_ERROR, NULL);
		return ERROR;
	}

	/* ListaVariaveis -> Variavel { , Variavel } : Tipo */
	while (1) {
		/* EXPECT_STATE(state_DeclOneVariable); */
		EXPECT_TOKEN(TOK_IDENTIFIER);
		if (!string_list_add(names, ps->current.repr, ps->current.pending)) {
			parser_error(ps, PARSER_SYSTEM_ERROR, NULL);
			destroy_string_list(names);
			return ERROR;
		}
		NEXT_TOKEN;

		if (ps->current.type == TOK_COLON)
			break;
		EXPECT_TOKEN(TOK_COMMA);
		NEXT_TOKEN;
	}
	EXPECT_TOKEN(TOK_COLON);
	NEXT_TOKEN;

	EXPECT_STATE_VALUE(state_Type, &rval);
	SEMANTIC_HOOK(sem_decl_var_list(ps->semantic, names, &rval));

	destroy_string_list(names);

	return OK;
}

int state_DeclVar(struct parser_state *ps)
{
	/* DeclVars -> var ListaVariaveis { ; ListaVariaveis } ; */
	EXPECT_TOKEN(TOK_KW_VAR);
	NEXT_TOKEN;
	EXPECT_STATE(state_VariablesList);
	EXPECT_TOKEN(TOK_SEMICOLON);

	while (1) {
		NEXT_TOKEN;

		/* TOK_IDENTIFIER obtained from APS of
		 * state_VariablesList->state_DeclOneVariable.
		 */
		if (ps->current.type != TOK_IDENTIFIER)
			break;

		EXPECT_STATE(state_VariablesList);
		EXPECT_TOKEN(TOK_SEMICOLON);
	}

	return OK;
}

int state_Expression(struct parser_state *ps, sem_ref_t *rval);
int state_ExpressionList(struct parser_state *ps, sem_ref_t *var);

int state_FatorI(struct parser_state *ps, sem_ref_t *rval)
{
	sem_ref_t var;

	EXPECT_TOKEN(TOK_IDENTIFIER);
	SEMANTIC_HOOK(sem_hold_var(ps->semantic, ps->current.repr,
	              ps->current.pending, &var));
	NEXT_TOKEN;

	/* repeated code, keep in sync with Variable */
	if (ps->current.type == TOK_LPARENTHESIS) {
		/* a function call */
		SEMANTIC_HOOK(sem_funcall_prolog(ps->semantic, &var));
		NEXT_TOKEN;
		EXPECT_STATE_VALUE(state_ExpressionList, &var);
		EXPECT_TOKEN(TOK_RPARENTHESIS);
		SEMANTIC_HOOK(sem_call_function(ps->semantic, &var, rval));
		NEXT_TOKEN;
	}
	else  {
		if (ps->current.type == TOK_OPENINGBRACKET)
			/* TODO array support, changes var */
			EXPECT_STATE(state_VariableArrayIndexing);
		SEMANTIC_HOOK(sem_get_var(ps->semantic, &var, rval));
	}
	return OK;
}

int state_Fator(struct parser_state *ps, sem_ref_t *rval)
{
	sem_ref_t left;

	int neg = 0;

	/* Handle the case of negative numbers, which are not handled by
	 * the tokenizer: */
	if (ps->current.type == TOK_MINUS) {
		neg = 1;
		NEXT_TOKEN;

		/* ensure we won't expressions such as "-not foo", or
		 * "-variable" without having proper code generation for it
		 */
		if (!(ps->current.type == TOK_INTEGER ||
				ps->current.type == TOK_REAL ||
				ps->current.type == TOK_CHAR)) {
			parser_error(ps, PARSER_UNEXPECTED_TOKEN,
					"integer, real or char");
			return ERROR;
		}
	}

	if (ps->current.type == TOK_IDENTIFIER)
		EXPECT_STATE_VALUE(state_FatorI, rval);
	else if (ps->current.type == TOK_INTEGER) {
		SEMANTIC_HOOK(sem_put_integer(ps->semantic,
					NEGVAL(neg, ps->current.token.integer),
					rval));
		NEXT_TOKEN;
	}
	else if (ps->current.type == TOK_REAL) {
		SEMANTIC_HOOK(sem_put_real(ps->semantic,
					NEGVAL(neg, ps->current.token.real),
					rval));
		NEXT_TOKEN;
	}
	else if (ps->current.type == TOK_CHAR) {
		SEMANTIC_HOOK(sem_put_char(ps->semantic,
					NEGVAL(neg, ps->current.repr[0]),
					rval));
		NEXT_TOKEN;
	}
	else if (ps->current.type == TOK_LPARENTHESIS) {
		NEXT_TOKEN;
		EXPECT_STATE_VALUE(state_Expression, rval);
		EXPECT_TOKEN(TOK_RPARENTHESIS);
		NEXT_TOKEN;
	}
	else if (ps->current.type == TOK_KW_NOT) {
		NEXT_TOKEN;
		EXPECT_STATE_VALUE(state_Fator, rval);
		left = *rval;
		SEMANTIC_HOOK(sem_not_value(ps->semantic, &left, rval));
	}
	else {
		parser_error(ps, PARSER_UNEXPECTED_TOKEN, "fator tokens");
		return ERROR;
	}

	return OK;
}

int state_Term(struct parser_state *ps, sem_ref_t *rval)
{
	enum token_t operator;
	sem_ref_t left, right;

	EXPECT_STATE_VALUE(state_Fator, &left);
	*rval = left;

	while (ps->current.type == TOK_ASTERISK 
			|| ps->current.type == TOK_KW_DIV
			|| ps->current.type == TOK_KW_AND
			|| ps->current.type == TOK_KW_MOD) {
		operator = ps->current.type;
		NEXT_TOKEN;
		EXPECT_STATE_VALUE(state_Fator, &right);

		switch (operator) {
		case TOK_ASTERISK:
			SEMANTIC_HOOK(sem_mul_values(ps->semantic, &left,
						&right, rval));
			break;
		case TOK_KW_DIV:
			SEMANTIC_HOOK(sem_div_values(ps->semantic, &left,
						&right, rval));
			break;
		case TOK_KW_MOD:
			SEMANTIC_HOOK(sem_mod_values(ps->semantic, &left,
						&right, rval));
			break;
		case TOK_KW_AND:
			SEMANTIC_HOOK(sem_boolcmp_values(ps->semantic,
						SEMANTIC_BOOL_AND, &left,
						&right, rval));
			break;
		default: break;
		}
	}

	return OK;
}

int state_SimpleExpression(struct parser_state *ps, sem_ref_t *rval)
{
	enum token_t oper;
	sem_ref_t left, right;
	int invert = 0;

	/* isn't necessary to have + or - in the first time */
	if (ps->current.type == TOK_PLUS || ps->current.type == TOK_MINUS) {
		if (ps->current.type == TOK_MINUS)
			invert = 1;
		NEXT_TOKEN;

		if (invert) {
			/* instead of generating instructions to invert the
			 * value of a constant integer, do it now */
			invert = 0;
			switch (ps->current.type) {
			case TOK_INTEGER:
				ps->current.token.integer = -ps->current.token.integer;
				break;
			case TOK_REAL:
				ps->current.token.real = -ps->current.token.real;
				break;
			case TOK_CHAR:
				ps->current.repr[0] = -ps->current.repr[0];
				break;
			default:
				/* don't care */
				invert = 1;
				break;
			}
		}
	}

	EXPECT_STATE_VALUE(state_Term, &left);
	*rval = left;

	if (invert)
		SEMANTIC_HOOK(sem_invert_value(ps->semantic, &left));

	while (ps->current.type == TOK_PLUS
			|| ps->current.type == TOK_MINUS
			|| ps->current.type == TOK_KW_OR) {
		oper = ps->current.type;
		NEXT_TOKEN;
		EXPECT_STATE_VALUE(state_Term, &right);
		switch (oper) {
		case TOK_PLUS:
			SEMANTIC_HOOK(sem_sum_values(ps->semantic, &left,
						&right, rval));
			break;
		case TOK_MINUS:
			SEMANTIC_HOOK(sem_subt_values(ps->semantic, &left,
						&right, rval));
			break;
		case TOK_KW_OR:
			SEMANTIC_HOOK(sem_boolcmp_values(ps->semantic,
						SEMANTIC_BOOL_OR, &left,
						&right, rval));
			break;
		default: break;
		}
	}

	return OK;
}

int state_Expression(struct parser_state *ps, sem_ref_t *rval)
{                          
	enum token_t operator;
	enum semantic_cmp_operators semop;
	sem_ref_t left, right;

	/* Expressao -> ExpressaoSimples [ OpRelacao ExpressaoSimples ] */
	EXPECT_STATE_VALUE(state_SimpleExpression, &left);
	*rval = left;

	if (IS_RELATIONALOP(ps->current.type)) {
		operator = ps->current.type;
		NEXT_TOKEN;
		EXPECT_STATE_VALUE(state_SimpleExpression, &right);

		switch (operator) {
		/* keep in sync with IS_RELATIONALOP */
		case TOK_EQUAL:
			semop = SEMANTIC_CMP_EQUAL;
			break;
		case TOK_DIFFERENT:
			semop = SEMANTIC_CMP_DIFFERENT;
			break;
		case TOK_LESSTHAN:
			semop = SEMANTIC_CMP_LESSTHAN;
			break;
		case TOK_LESSEQTHAN:
			semop = SEMANTIC_CMP_LESSEQTHAN;
			break;
		case TOK_GREATERTHAN:
			semop = SEMANTIC_CMP_GREATERTHAN;
			break;
		case TOK_GREATEREQTHAN:
			semop = SEMANTIC_CMP_GREATEREQTHAN;
			break;
		default:
			break;
		}

		SEMANTIC_HOOK(sem_relcmp_values(ps->semantic,
					semop, &left,
					&right, rval));
	}
	
	return OK;
}

int state_Command(struct parser_state *ps);

int state_ConditionalCom(struct parser_state *ps)
{
	sem_ref_t rval;
	sem_ref_t holdpos;

	EXPECT_TOKEN(TOK_KW_IF);
	SEMANTIC_HOOK(sem_cond_prolog(ps->semantic,  &holdpos));
	NEXT_TOKEN;
	EXPECT_STATE_VALUE(state_Expression, &rval);
	SEMANTIC_HOOK(sem_cond_eval(ps->semantic, &rval, &holdpos));
	/* TODO evaluate the value */
	EXPECT_TOKEN(TOK_KW_THEN);
	NEXT_TOKEN;
	EXPECT_STATE(state_Command);
	if(ps->current.type == TOK_KW_ELSE) {
		SEMANTIC_HOOK(sem_cond_else(ps->semantic, &holdpos));
		NEXT_TOKEN;
		EXPECT_STATE(state_Command);
	}
	SEMANTIC_HOOK(sem_cond_epilog(ps->semantic, &holdpos));
	return OK;
}

int state_RepeatCom(struct parser_state *ps)
{
	sem_ref_t rval;
	sem_ref_t holdpos;

	if (ps->current.type == TOK_KW_WHILE) {
		SEMANTIC_HOOK(sem_while_prolog(ps->semantic, &holdpos));
		NEXT_TOKEN;
		EXPECT_STATE_VALUE(state_Expression, &rval);
		SEMANTIC_HOOK(sem_while_eval(ps->semantic, &rval, &holdpos));
		EXPECT_TOKEN(TOK_KW_DO);
		NEXT_TOKEN;
		EXPECT_STATE(state_Command);
		SEMANTIC_HOOK(sem_while_epilog(ps->semantic, &holdpos));
	}
	else if (ps->current.type == TOK_KW_REPEAT) {
		SEMANTIC_HOOK(sem_repeat_prolog(ps->semantic, &holdpos));
		NEXT_TOKEN;

		/* The repeat block is exceptional: it doesn't require more
		 * than one "command" to be grouped using begin and end
		 * keywords. */
		EXPECT_STATE(state_Command);
		while (ps->current.type != TOK_KW_UNTIL) {
			EXPECT_TOKEN(TOK_SEMICOLON);
			NEXT_TOKEN;
			if (ps->current.type == TOK_KW_UNTIL)
				break;
			EXPECT_STATE(state_Command);
		}

		EXPECT_TOKEN(TOK_KW_UNTIL);
		NEXT_TOKEN;
		EXPECT_STATE_VALUE(state_Expression, &rval);
		SEMANTIC_HOOK(sem_repeat_eval(ps->semantic, &rval,
					&holdpos));
	}
	else {
		parser_error(ps, PARSER_UNEXPECTED_TOKEN, MANY_TOKENS);
		return ERROR;
	}
	return OK;
}

int state_BranchingCommand(struct parser_state *ps)
{
	sem_ref_t var;

	EXPECT_TOKEN(TOK_KW_GOTO);
	NEXT_TOKEN;
	EXPECT_TOKEN(TOK_INTEGER);
	SEMANTIC_HOOK(sem_hold_var(ps->semantic, ps->current.repr,
				ps->current.pending, &var));
	SEMANTIC_HOOK(sem_goto_label(ps->semantic, &var));
	NEXT_TOKEN;
	return OK;
}

int state_Const(struct parser_state *ps)
{
	EXPECT_TOKEN(TOK_IDENTIFIER);
	NEXT_TOKEN;
	return OK;
}

int state_DefineConst(struct parser_state *ps)
{   
	size_t size;
	char name[BUFSIZ];
	int neg = 0;

	EXPECT_TOKEN(TOK_IDENTIFIER);

	strncpy(name, ps->current.repr, BUFSIZ-1);
	size = ps->current.pending;

	NEXT_TOKEN;
	EXPECT_TOKEN(TOK_EQUAL);
	NEXT_TOKEN;

	if (ps->current.type == TOK_MINUS) {
		neg = 1;
		NEXT_TOKEN;
	}

	switch (ps->current.type) {
	case TOK_INTEGER:
		SEMANTIC_HOOK(sem_decl_const_int(ps->semantic, name, size,
					NEGVAL(neg, ps->current.token.integer)));
		break;
	case TOK_REAL:
		SEMANTIC_HOOK(sem_decl_const_real(ps->semantic, name, size,
					NEGVAL(neg, ps->current.token.real)));
		break;
	case TOK_CHAR:
		SEMANTIC_HOOK(sem_decl_const_char(ps->semantic, name, size,
					NEGVAL(neg, ps->current.repr[0])));
		break;
	default:
		parser_error(ps, PARSER_UNEXPECTED_TOKEN, MANY_TOKENS);
		return ERROR;
	}

	NEXT_TOKEN;
	return OK;
}

int state_DeclConst(struct parser_state *ps)
{
	EXPECT_TOKEN(TOK_KW_CONST);
	NEXT_TOKEN;
	EXPECT_STATE(state_DefineConst);
	EXPECT_TOKEN(TOK_SEMICOLON);

	while (1) {
		NEXT_TOKEN;

		/* From APS of state_DefineConst */
		if (ps->current.type != TOK_IDENTIFIER)
			break;

		EXPECT_STATE(state_DefineConst);
		EXPECT_TOKEN(TOK_SEMICOLON);
	}

	return OK;
}

int state_DeclLabels(struct parser_state *ps)
{
	EXPECT_TOKEN(TOK_KW_LABEL);
	NEXT_TOKEN;
	EXPECT_TOKEN(TOK_INTEGER);
	SEMANTIC_HOOK(sem_decl_label(ps->semantic,
				ps->current.repr, ps->current.pending));
	NEXT_TOKEN;
	while (ps->current.type == TOK_COMMA) {
		EXPECT_TOKEN(TOK_COMMA);
		NEXT_TOKEN;
		EXPECT_TOKEN(TOK_INTEGER);
		SEMANTIC_HOOK(sem_decl_label(ps->semantic,
					ps->current.repr, ps->current.pending));
		NEXT_TOKEN;
	}
	EXPECT_TOKEN(TOK_SEMICOLON);
	NEXT_TOKEN;
	return OK;
}

int state_ExpressionList(struct parser_state *ps, sem_ref_t *var)
{
	sem_ref_t rval, expritem, ref;
	int ignoreref;
	int loop = 0;

	SEMANTIC_HOOK(sem_begin_expr_list(ps->semantic, var, &expritem, &ref));

	do {
		if (loop)
			NEXT_TOKEN;
		else
			loop = 1;

		ignoreref = 1;

		if (ps->current.type == TOK_IDENTIFIER) {
			/* More crap: give a hint to the semantic checker
			 * that this parameter can be passed as reference
			 * to the called function. */
			SEMANTIC_HOOK(sem_hold_var(ps->semantic,
						ps->current.repr,
						ps->current.pending,
						&ref));
			SEMANTIC_HOOK(sem_check_ref(ps->semantic,
						&expritem, &ref,
						&rval, &ignoreref));
		}

		if (ignoreref)
			EXPECT_STATE_VALUE(state_Expression, &rval);
		else
			NEXT_TOKEN;

		SEMANTIC_HOOK(sem_expr_list_item(ps->semantic, var,
					&expritem, &rval, &ref));

	} while (ps->current.type == TOK_COMMA);

	SEMANTIC_HOOK(sem_end_expr_list(ps->semantic, var, &expritem));

	return OK;
}

int state_AssignmentAfterIdentifier(struct parser_state *ps)
{
	return OK;
}

/**
 * Merge of ProcedureCall and Assignment
 */
int state_Command_ProcedureCallOrAssignment(struct parser_state *ps)
{
	sem_ref_t var, rval;
	/* FIXME split these functions and pass the identifier token as an
	 * argument. 
	 *
	 * We will be in trouble when we start working on the syntax tree,
	 * because Assignment will have to know the identifier being
	 * handled.
	 */
	EXPECT_TOKEN(TOK_IDENTIFIER);
	SEMANTIC_HOOK(sem_hold_var(ps->semantic, ps->current.repr,
				ps->current.pending, &var));
	NEXT_TOKEN;

	if (ps->current.type == TOK_OPENINGBRACKET
			|| ps->current.type == TOK_ASSIGNMENT) {
		/* if (ps->current.type == TOK_OPENINGBRACKET)
		 * 	here we must get the address value */
		/* EXPECT_STATE(state_AssignmentAfterIdentifier); */
		/* repeated code, keep in sync with Variable */
		if (ps->current.type == TOK_OPENINGBRACKET) {
			EXPECT_STATE(state_VariableArrayIndexing);
			/* TODO parse Expression */
			/* TODO something line sem_hold_array_ref(..,
			 * &var) */
		}
		EXPECT_TOKEN(TOK_ASSIGNMENT);
		NEXT_TOKEN;
		EXPECT_STATE_VALUE(state_Expression, &rval);
		SEMANTIC_HOOK(sem_var_assignment(ps->semantic, &var,
					&rval));
	}
	else {
		/* a procedure or function call */

		SEMANTIC_HOOK(sem_funcall_prolog(ps->semantic, &var));

		if (ps->current.type == TOK_LPARENTHESIS) {
			NEXT_TOKEN;
			EXPECT_STATE_VALUE(state_ExpressionList, &var);
			EXPECT_TOKEN(TOK_RPARENTHESIS);
			NEXT_TOKEN;
		}

		/* a procedure or function call without parameters */
		SEMANTIC_HOOK(sem_call_function(ps->semantic, &var,
					&rval));

		SEMANTIC_HOOK(sem_funcall_cleanup(ps->semantic, &var, 1));
	}

	return OK;
}

int state_Parameters(struct parser_state *ps)
{
	if (ps->current.type == TOK_KW_VAR) {
		EXPECT_TOKEN(TOK_KW_VAR); /* just to see it in the dump */
		SEMANTIC_HOOK(sem_set_byref_param(ps->semantic));
		NEXT_TOKEN;
	}

	EXPECT_STATE(state_VariablesList);
	SEMANTIC_HOOK(sem_set_byval_param(ps->semantic));
	return OK;
}

int state_Block(struct parser_state*);

int state_DeclProcedure(struct parser_state *ps)
{ 
	sem_ref_t var;

	EXPECT_TOKEN(TOK_KW_PROCEDURE);
	NEXT_TOKEN;
	EXPECT_TOKEN(TOK_IDENTIFIER);
	SEMANTIC_HOOK(sem_decl_procedure(ps->semantic, ps->current.repr,
				ps->current.pending, &var));
	NEXT_TOKEN;

	if (ps->current.type == TOK_LPARENTHESIS) {
		/* shouldn't we have a separated state for it */
		EXPECT_TOKEN(TOK_LPARENTHESIS);
		NEXT_TOKEN;
		SEMANTIC_HOOK(sem_begin_params(ps->semantic, &var));
		EXPECT_STATE(state_Parameters);
		while (ps->current.type != TOK_RPARENTHESIS) {
			EXPECT_TOKEN(TOK_SEMICOLON);
			NEXT_TOKEN;
			EXPECT_STATE(state_Parameters);
		}
		EXPECT_TOKEN(TOK_RPARENTHESIS);
		NEXT_TOKEN;

		SEMANTIC_HOOK(sem_finish_params(ps->semantic, &var));
	}

	EXPECT_TOKEN(TOK_SEMICOLON);
	NEXT_TOKEN;
	EXPECT_STATE(state_Block);

	SEMANTIC_HOOK(sem_finish_procedure(ps->semantic, &var));

	return OK;
}

int state_DeclFunction(struct parser_state *ps)
{
	sem_ref_t var, type;

	/* TODO merge the repeated code between procedure and function */
	EXPECT_TOKEN(TOK_KW_FUNCTION);
	NEXT_TOKEN;
	EXPECT_TOKEN(TOK_IDENTIFIER);
	SEMANTIC_HOOK(sem_decl_function(ps->semantic, ps->current.repr,
				ps->current.pending, &var));
	NEXT_TOKEN;

	if (ps->current.type == TOK_LPARENTHESIS) {
		/* shouldn't we have a separated state for it */
		EXPECT_TOKEN(TOK_LPARENTHESIS);
		NEXT_TOKEN;
		SEMANTIC_HOOK(sem_begin_params(ps->semantic, &var));
		EXPECT_STATE(state_Parameters);
		while (ps->current.type != TOK_RPARENTHESIS) {
			EXPECT_TOKEN(TOK_SEMICOLON);
			NEXT_TOKEN;
			EXPECT_STATE(state_Parameters);
		}
		EXPECT_TOKEN(TOK_RPARENTHESIS);
		NEXT_TOKEN;

		SEMANTIC_HOOK(sem_finish_params(ps->semantic, &var));
	}

	EXPECT_TOKEN(TOK_COLON);
	NEXT_TOKEN;
	EXPECT_STATE_VALUE(state_Type, &type);

	SEMANTIC_HOOK(sem_function_type(ps->semantic, &var, &type));

	EXPECT_TOKEN(TOK_SEMICOLON);
	NEXT_TOKEN;
	EXPECT_STATE(state_Block);

	SEMANTIC_HOOK(sem_finish_procedure(ps->semantic, &var));

	return OK;
}

int state_DeclSub(struct parser_state *ps)
{
	while (ps->current.type == TOK_KW_PROCEDURE 
			|| ps->current.type == TOK_KW_FUNCTION) {
		if (ps->current.type == TOK_KW_PROCEDURE)
			EXPECT_STATE(state_DeclProcedure);
		else
			EXPECT_STATE(state_DeclFunction);
		EXPECT_TOKEN(TOK_SEMICOLON);
		NEXT_TOKEN;
	}
	return OK;
}

int state_ReadCommand(struct parser_state *ps)
{
	sem_ref_t var;

	EXPECT_TOKEN(TOK_KW_READ);
	NEXT_TOKEN;
	EXPECT_TOKEN(TOK_LPARENTHESIS);
	NEXT_TOKEN;

	while (ps->current.type != TOK_RPARENTHESIS) {
		if (ps->current.type != TOK_IDENTIFIER) {
			parser_error(ps, PARSER_UNEXPECTED_TOKEN,
					"a scalar variable");
			return ERROR;
		}

		SEMANTIC_HOOK(sem_hold_var(ps->semantic, ps->current.repr,
					ps->current.pending, &var));
		SEMANTIC_HOOK(sem_read_var(ps->semantic, &var));

		NEXT_TOKEN;
		if (ps->current.type == TOK_RPARENTHESIS)
			break;
		EXPECT_TOKEN(TOK_COMMA);

		NEXT_TOKEN;
	}
	EXPECT_TOKEN(TOK_RPARENTHESIS);
	
	NEXT_TOKEN;
	return OK;
}

int state_WriteCommand(struct parser_state *ps)
{
	sem_ref_t rval;

	EXPECT_TOKEN(TOK_KW_WRITE);
	NEXT_TOKEN;
	EXPECT_TOKEN(TOK_LPARENTHESIS);
	NEXT_TOKEN;

	while (ps->current.type != TOK_RPARENTHESIS) {

		EXPECT_STATE_VALUE(state_Expression, &rval);
		SEMANTIC_HOOK(sem_write_value(ps->semantic, &rval));

		if (ps->current.type == TOK_RPARENTHESIS)
			break;
		EXPECT_TOKEN(TOK_COMMA);

		NEXT_TOKEN;
	}
	EXPECT_TOKEN(TOK_RPARENTHESIS);
	
	NEXT_TOKEN;
	return OK;
}

int state_CommandBlock(struct parser_state*);

int state_Command(struct parser_state *ps)
{
	sem_ref_t var;

	/* Comando -> [Label:] Atribuicao | ComandoComposto */
	if (ps->current.type == TOK_INTEGER) {
		/* Label "instantiation" */
		SEMANTIC_HOOK(sem_hold_var(ps->semantic, ps->current.repr,
					ps->current.pending, &var));
		NEXT_TOKEN;
		EXPECT_TOKEN(TOK_COLON);
		SEMANTIC_HOOK(sem_inst_label(ps->semantic, &var));
		NEXT_TOKEN;
	}

	if (ps->current.type == TOK_KW_BEGIN)
		EXPECT_STATE(state_CommandBlock);
	else if (ps->current.type == TOK_IDENTIFIER)
		EXPECT_STATE(state_Command_ProcedureCallOrAssignment);
	else if (ps->current.type == TOK_KW_IF)
		EXPECT_STATE(state_ConditionalCom);
	else if (ps->current.type == TOK_KW_WHILE
			|| ps->current.type == TOK_KW_REPEAT)
		EXPECT_STATE(state_RepeatCom);
	else if (ps->current.type == TOK_KW_GOTO)
		EXPECT_STATE(state_BranchingCommand);
	else if (ps->current.type == TOK_KW_READ)
		EXPECT_STATE(state_ReadCommand);
	else if (ps->current.type == TOK_KW_WRITE)
		EXPECT_STATE(state_WriteCommand);
	else {
		parser_error(ps, PARSER_UNEXPECTED_TOKEN,
				"begin or an assignment");
		return ERROR;
	}
	return OK;
}

int state_CommandBlock(struct parser_state *ps)
{
	/* ComComposto -> begin Comando { ; Comando } end */
	EXPECT_TOKEN(TOK_KW_BEGIN);

	NEXT_TOKEN;
	EXPECT_STATE(state_Command);

	while (ps->current.type != TOK_KW_END) {
		EXPECT_TOKEN(TOK_SEMICOLON);
		/* TODO signal of crappy specification here:
		 * The grammar doens't allow us to have begin command; end.
		 * The last Command entry should never have the semicolon,
		 * otherwise it is an error.
		 */
		/*
		if (ps->current.type == TOK_KW_END)
			break; */
		NEXT_TOKEN;

		if (ps->current.type == TOK_KW_END)
			break;

		EXPECT_STATE(state_Command);
	}

	EXPECT_TOKEN(TOK_KW_END);

	NEXT_TOKEN;
	return OK;
}

int state_Block(struct parser_state *ps)
{
	if (ps->current.type == TOK_KW_LABEL)
		EXPECT_STATE(state_DeclLabels);
	if (ps->current.type == TOK_KW_CONST)
		EXPECT_STATE(state_DeclConst);
	if (ps->current.type == TOK_KW_VAR)
		EXPECT_STATE(state_DeclVar);
	if (ps->current.type == TOK_KW_PROCEDURE
			|| ps->current.type == TOK_KW_FUNCTION)
		EXPECT_STATE(state_DeclSub);

	SEMANTIC_HOOK(sem_begin_code_block(ps->semantic));

	EXPECT_STATE(state_CommandBlock);

	return OK;
}

/**
 * The initial state S.
 *
 * State functions always expect to have their first token already fetched
 * by the caller, this way we can have a consistent calling protocol
 * between states.
 *
 * AND State functions always leave one token fetched to be used by the
 * caller (ie. you don't need to use NEXT_TOKEN after calling a state).
 */
int state_S(struct parser_state *ps)
{
	EXPECT_TOKEN(TOK_KW_PROGRAM);
	NEXT_TOKEN;
	EXPECT_TOKEN(TOK_IDENTIFIER);

	SEMANTIC_HOOK(sem_init_program(ps->semantic, ps->current.repr,
				ps->current.pending));

	NEXT_TOKEN;

	if (ps->current.type == TOK_LPARENTHESIS) {
		EXPECT_TOKEN(TOK_LPARENTHESIS);
		NEXT_TOKEN;
		EXPECT_TOKEN(TOK_IDENTIFIER);
		NEXT_TOKEN;
		while (ps->current.type == TOK_COMMA) {
			EXPECT_TOKEN(TOK_COMMA);
			NEXT_TOKEN;
			EXPECT_TOKEN(TOK_IDENTIFIER);
			NEXT_TOKEN;
		}
		EXPECT_TOKEN(TOK_RPARENTHESIS);
		NEXT_TOKEN;
	}

	EXPECT_TOKEN(TOK_SEMICOLON);

	NEXT_TOKEN;
	EXPECT_STATE(state_Block);
	EXPECT_TOKEN(TOK_DOT);

	SEMANTIC_HOOK(sem_finish_program(ps->semantic));

	NEXT_TOKEN;
	return OK;
}
