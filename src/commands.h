#include "main.h"

struct Command {
	char* name;
	void (*func)(struct State *state, const char** args, FILE* in, FILE* out, FILE* err);
};
