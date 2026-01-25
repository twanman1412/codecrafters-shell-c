#pragma once

struct State {
	char* cwd;
	char** paths;
};

char* get_command(const char* input);
char** get_arguments(const char* input);

char* get_executable(struct State state, const char* name);
