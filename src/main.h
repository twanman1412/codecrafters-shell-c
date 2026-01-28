#pragma once

struct State {
	char* cwd;
	char** paths;
};

void execute_command(struct State *state, char** args, char* out, char* err);

char* get_command(const char* input);
char** get_arguments(const char* input);

char* get_executable(struct State state, const char* name);
