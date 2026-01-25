#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "commands.h"

const struct Command* commands[];

void __exit(struct State *_, char **args) {
	exit(0);
}

struct Command exit_cmd = {
	.name = "exit",
	.func = &__exit
};

void _echo(struct State *_, char **args) {
	for (int i = 1; args[i] != NULL; i++) {
		printf("%s", args[i]);
		if (args[i + 1] != NULL) {
			printf(" ");
		}
	}
	printf("\n");
}

struct Command echo_cmd = {
	.name = "echo",
	.func = &_echo
};

void type(struct State *state, char **args) {
	char* name = args[1]; // first argument is the command name

	for (int i = 0; commands[i] != NULL; i++) {
		if (strcmp(name, commands[i]->name) == 0) {
			printf("%s is a shell builtin\n", name);
			return;
		}
	}

	char* cmd = get_command(name);
	char* exe = get_executable(*state, cmd);
	if (exe != NULL) {
		printf("%s is %s\n", name, exe);
		free(exe);
	} else {
		printf("%s: not found\n", name);
	}

	free(cmd);
}

struct Command type_cmd = {
	.name = "type",
	.func = &type
};

void _pwd(struct State *state, char **args) {
	printf("%s\n", state->cwd);
}

struct Command pwd_cmd = {
	.name = "pwd",
	.func = &_pwd
};

void _cd(struct State *state, char **args) {
	const char* path = args[1];

	if (chdir(path) == 0) {
		char* new_cwd = getcwd(NULL, 0);
		state->cwd = new_cwd;
	} else {
		printf("cd: %s: No such file or directory\n", path);
	}
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
