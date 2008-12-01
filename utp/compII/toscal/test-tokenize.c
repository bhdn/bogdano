#include <stdio.h>
#include <stdlib.h>

#include "input.h"
#include "tokenize.h"

int main(int argc, char *argv[])
{
	FILE *input;
	size_t i;
	struct token tok;
	struct input_state *is;
	int err = 0;

	for (i = 1; i < argc; i++) {

		input = fopen(argv[i], "r");
		if (!input) {
			perror(argv[i]);
			err = 3;
			goto failed_open;
		}

		is = init_input_state(input);
		if (!is) {
			err = 1;
			perror("while allocating input state");
			goto failed_input_stream;
		}

		while (fetch_next_token(is, &tok))
			dump_token(&tok, stdout);

		if (tok.type != TOK_EOF) {
			fputs("error: ", stderr);
			tokenizer_dump_error(&tok, is, stderr);
			err = 2;
		}

		close_input_state(is);
failed_input_stream:
		fclose(input);
	}
failed_open:
	return err;
}
