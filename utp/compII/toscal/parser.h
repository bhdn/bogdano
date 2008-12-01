#ifndef inc_parser_h
#define inc_parser_h

#include "input.h"
#include "tokenize.h"
#include "semantic.h"

enum error_type {
	PARSER_SUCCESS,
	PARSER_UNEXPECTED_TOKEN = 1,
	PARSER_READ_ERROR,
	PARSER_TOKENIZE_ERROR,
	PARSER_SYSTEM_ERROR,
	PARSER_SEMANTIC_ERROR
};

#define MANY_TOKENS	"(many tokens here)"

struct parser_state {
	struct input_state *input;
	struct token next;
	struct token current;
	enum error_type error;
	char *expected;
	FILE *debug_stream;
	int dump_tokens;
	int semantic_check;
	struct semantic_state *semantic;
};

struct parser_state *init_parser_state(struct input_state *input,
		struct semantic_state *semantic);
void parser_dump_error(struct parser_state *ps, FILE *stream);
int parser_check(struct parser_state *ps);

#endif
