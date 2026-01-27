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
	char* cmd = args[1]; // first argument is the command name

	for (int i = 0; commands[i] != NULL; i++) {
		if (strcmp(cmd, commands[i]->name) == 0) {
			printf("%s is a shell builtin\n", cmd);
			return;
		}
	}

	char* exe = get_executable(*state, cmd);
	if (exe != NULL) {
		printf("%s is %s\n", cmd, exe);
		free(exe);
	} else {
		printf("%s: not found\n", cmd);
	}
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

	if (strncmp(path, "/", 1) == 0) {
		if (chdir(path) == 0) {
			char* new_cwd = getcwd(NULL, 0);
			state->cwd = new_cwd;
			return;
		} 

		printf("cd: %s: No such file or directory\n", path);
		return; 
	}

	if (strncmp(path, "~", 1) == 0) {
		const char* home = getenv("HOME");

		char new_path[1024];
		snprintf(new_path, sizeof(new_path), "%s/%s", home, path + 1);

		if (chdir(new_path) == 0) {
			char* new_cwd = getcwd(NULL, 0);
			state->cwd = new_cwd;
			return;
		}

		printf("cd: %s: No such file or directory\n", path);
		return;
	}

	char new_path[1024];
	snprintf(new_path, sizeof(new_path), "%s/%s", state->cwd, path);

	if (chdir(new_path) == 0) {
		char* new_cwd = getcwd(NULL, 0);
		state->cwd = new_cwd;
		return;
	}

	printf("cd: %s: No such file or directory\n", path);
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
