#include <stdio.h>
#include <string.h>

#include "input.h"
#include "tokenize.h"
#include "parser.h"
#include "semantic.h"
#include "codegen.h"

int main(int argc, char *argv[])
{
	int i;
	struct input_state *input;
	struct semantic_state *semantic;
	struct parser_state *parser;
	struct codegen_state *codegen;
	FILE *source = NULL;

	input = init_input_state(stdin);
	if (!input) {
		perror("allocating the input state");
		goto failed;
	}

	codegen = init_codegen_state(stdout);
	if (!codegen) {
		perror("allocating codegen state");
		goto failed;
	}

	semantic = init_semantic_state(codegen);
	if (!semantic) {
		perror("allocating semantic state");
		goto failed;
	}
	semantic->warning_stream = stderr;

	parser = init_parser_state(input, semantic);
	if (!parser) {
		perror("while allocationg parser state");
		goto failed;
	}

	for (i = 1; i < argc; i++) {
		if (argv[i][0] == '-') {
			switch (argv[i][1]) {
			case 'd':
				parser->debug_stream = stdout;
				break;
			case 't':
				parser->dump_tokens = 1;
				break;
			case 'S':
				parser->semantic_check = 0;
				break;
			case 'z':
				semantic->debug_stream = stdout;
				break;
			case 'W':
				semantic->warning_stream = NULL;
				break;
			case 'C':
				codegen->out = NULL;
				break;
			default:
				fprintf(stderr, "invalid option %s\n", argv[i]);
				goto failed;
			}
		}
		else if (!source) {
			source = fopen(argv[i], "r");
			if (!source) {
				perror(argv[i]);
				goto failed;
			}
			input->stream = source;
		}
	}

	if (input->stream == stdin)
		fputs("reading from stdin\n", stderr);

	if (!parser_check(parser)) {
		parser_dump_error(parser, stderr);
		goto failed;
	}

	/* FIXME memory is not being freed */

	return 0;
failed:
	return 1;
}
