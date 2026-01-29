#pragma once

#include <stdio.h>

struct State {
	char* cwd;
	char** paths;
	bool exit;
};

void execute_command(struct State *state, char** args, FILE* in, FILE* out, FILE* err);

char* get_command(const char* input);
char** get_arguments(const char* input);

char* get_executable(struct State state, const char* name);

char** get_autocomplete(struct State state, const char* input);
