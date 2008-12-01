#ifndef inc_input_h
#define inc_input_h

#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>

#define INPUT_EOF	0
#define INPUT_ERROR	1

struct input_state {
	FILE *stream;
	size_t lineno;
	size_t linepos;
	size_t last_linepos; /* holds the position in the last line, should
				be used to help the error messages */
	int first;
	int current;
	int last;
};

int input_next(struct input_state *is);
struct input_state *init_input_state(FILE *stream);
void close_input_state(struct input_state *is);
int input_step_back(struct input_state *is);
void input_dump_position(struct input_state*, FILE *stream);

#endif
