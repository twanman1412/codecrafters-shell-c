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

const struct Command* commands[] = {
	&exit_cmd,
	&echo_cmd,
	&type_cmd,
	NULL
};
