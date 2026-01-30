#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "commands.h"

const struct Command* commands[];

void __exit(struct State *state, const char **args, FILE* in, FILE* out, FILE* err) {
	state->exit = true;
}

struct Command exit_cmd = {
	.name = "exit",
	.func = &__exit
};

void _echo(struct State *_, const char **args, FILE* in, FILE* out, FILE* err) {
	int length = 0;
	for (int i = 1; args[i] != NULL; i++) {
		length += strlen(args[i]) + 1; // +1 for space or newline
	}

	char* res = malloc(length + 1); // +1 for null terminator
	char* dst = res;
	for (int i = 1; args[i] != NULL; i++) {
		for (int j = 0; args[i][j] != '\0'; j++) {
			*dst++ = args[i][j];
		}
		if (args[i + 1] != NULL) {
			*dst++ = ' ';
		}
	}
	*dst++ = '\n';
	*dst = '\0';

	fprintf(out, "%s", res);
}

struct Command echo_cmd = {
	.name = "echo",
	.func = &_echo
};

void type(struct State *state, const char **args, FILE* in, FILE* out, FILE* err) {
	const char* cmd = args[1]; // first argument is the command name

	for (int i = 0; commands[i] != NULL; i++) {
		if (strcmp(cmd, commands[i]->name) == 0) {
			fprintf(out, "%s is a shell builtin\n", cmd);
			return;
		}
	}

	char* exe = get_executable(*state, cmd);
	if (exe != NULL) {
		fprintf(out, "%s is %s\n", cmd, exe);
		free(exe);
	} else {
		fprintf(out, "%s: not found\n", cmd);
	}
}

struct Command type_cmd = {
	.name = "type",
	.func = &type
};

void _pwd(struct State *state, const char **args, FILE* in, FILE* out, FILE* err) {
	fprintf(out, "%s\n", state->cwd);
}

struct Command pwd_cmd = {
	.name = "pwd",
	.func = &_pwd
};

void _cd(struct State *state, const char **args, FILE* in, FILE* out, FILE* err) {
	const char* path = args[1];

	if (path == NULL || strcmp(path, "~") == 0) {
		const char* home = getenv("HOME");
		if (chdir(home) == 0) {
			char* new_cwd = getcwd(NULL, 0);
			state->cwd = new_cwd;
			return;
		}

		fprintf(err, "cd: %s: No such file or directory\n", home);
		return;
	}

	if (strncmp(path, "/", 1) == 0) {
		if (chdir(path) == 0) {
			char* new_cwd = getcwd(NULL, 0);
			state->cwd = new_cwd;
			return;
		} 

		fprintf(err, "cd: %s: No such file or directory\n", path);
		return; 
	}

	if (strncmp(path, "~", 1) == 0) {
		const char* home = getenv("HOME");

		char* new_path = malloc(strlen(home) + strlen(path));
		sprintf(new_path, "%s/%s", home, path + 1);

		if (chdir(new_path) == 0) {
			char* new_cwd = getcwd(NULL, 0);
			state->cwd = new_cwd;
			return;
		}

		fprintf(err, "cd: %s: No such file or directory\n", path);
		return;
	}

	char* new_path = malloc(strlen(state->cwd) + strlen(path) + 2);
	sprintf(new_path, "%s/%s", state->cwd, path);

	if (chdir(new_path) == 0) {
		char* new_cwd = getcwd(NULL, 0);
		state->cwd = new_cwd;
		free(new_path);
		return;
	}

	free(new_path);
	fprintf(err, "cd: %s: No such file or directory\n", path);
}

struct Command cd_cmd = {
	.name = "cd",
	.func = &_cd
};

const struct Command* commands[] = {
	&exit_cmd,
	&echo_cmd,
	&type_cmd,
	&pwd_cmd,
	&cd_cmd,
	NULL
};
