#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include <sys/types.h>
#include <sys/stat.h>
#include <sys/wait.h>

#include "main.h"
#include "commands.h"

#ifdef _WIN32
#define os_pathsep ";"
#else
#define os_pathsep ":"
#endif

extern const struct Command* commands[];

int main(int argc, char *argv[]) {
	// Flush after every printf
	setbuf(stdout, NULL);

	// ======= Get PATH entries =======

	int count = 0;
	char* path = getenv("PATH");
	char* path_copy = strdup(path); // strtok modifies the string
	char* token = strtok(path_copy, os_pathsep);
	while (token != NULL) {
		count++;
		token = strtok(NULL, os_pathsep);
	}
	free(path_copy);

	// Allocate array of pointers + NULL terminator
	char** paths = malloc((count + 1) * sizeof(char*));
	path_copy = strdup(path);
	token = strtok(path_copy, os_pathsep);
	int index = 0;
	while (token != NULL) {
		paths[index++] = strdup(token);
		token = strtok(NULL, os_pathsep);
	}
	paths[index] = NULL;
	free(path_copy);

	// ======= State setup =======

	struct State state = {
		.cwd = getcwd(NULL, 0),
		.paths = paths
	};

	// ======= Main loop =======

main:
	while (1) {
		printf("$ ");

		char input[256] = {0};
		fgets(input, sizeof(input), stdin);

		input[strcspn(input, "\n")] = 0;
		char*  cmd  = get_command(input);
		char** args = get_arguments(input);


		for (int i = 0; commands[i] != NULL; i++) {
			if (strcmp(cmd, commands[i]->name) == 0) {
				commands[i]->func(&state, args);
				free(cmd);
				for (int j = 0; args[j] != NULL; j++) {
					free(args[j]);
				}
				free(args);
				goto main;
			}
		}

		char* exe = get_executable(state, cmd);
		if (exe != NULL) {
			pid_t pid = fork();
			if (pid == 0) {
				// Child process
				execv(exe, args);
				exit(0);
			} else if (pid > 0) {
				// Parent process
				int status;
				waitpid(pid, &status, 0);
			} else {
				// Fork failed
				perror("fork failed");
				exit(-1);
			}

			free(exe);
			free(args);
			continue;
		}

		printf("%s: command not found\n", input);
	}

	free(paths);
	return 0;
}

char* get_command(const char* input) {
	char* input_copy = strdup(input);
	char* token = strtok(input_copy, " ");
	char* command = strdup(token);
	free(input_copy);
	return command;
}

char** get_arguments(const char* input) {
	char* input_copy = strdup(input);
	 
	int count = 0;
	char* token = strtok(input_copy, " "); // Command name is the first argument
	while (token != NULL) {
		count++;
		token = strtok(NULL, " ");
	}

	// Allocate array of pointers + NULL terminator
	char** result = malloc((count + 1) * sizeof(char*));
	input_copy = strdup(input);
	token = strtok(input_copy, " "); // Skip command
	int index = 0;
	while (token != NULL) {
		result[index++] = strdup(token);
		token = strtok(NULL, " ");
	}
	result[index] = NULL;

	free(input_copy);

	return result;
}

char* get_executable(struct State state, const char* name) {
	int i = 0;
	char* path = state.paths[i];
	while (path != NULL) {
		// Allocate space for full path + slash + name + null terminator
		char* fullpath = malloc(strlen(path) + strlen(name) + 2);
		sprintf(fullpath, "%s/%s", path, name);

		struct stat perm;
		if (stat(fullpath, &perm) == 0 && perm.st_mode & S_IXUSR) {
			return fullpath;
		}

		free(fullpath);
		path = state.paths[++i];
	}

	return NULL;
}
