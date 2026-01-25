#include "main.h"

struct Command {
	char* name;
	void (*func)(struct State *state, char** args);
};
