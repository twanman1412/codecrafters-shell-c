#pragma once

#include <stdio.h>

struct State {
	char* cwd;
	char** paths;
	bool exit;
};

void parse_and_execute_command(struct State *state, const char** args, FILE* in, FILE* out, FILE* err);
void execute_command(struct State *state, const char** args, FILE* in, FILE* out, FILE* err);

char** get_arguments(const char* input);

char* get_executable(struct State state, const char* name);

char** get_autocomplete(struct State state, const char* input);
