#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <stdbool.h>

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
		char** args = get_arguments(input);
		char*  cmd  = args[0];


		for (int i = 0; commands[i] != NULL; i++) {
			if (strcmp(cmd, commands[i]->name) == 0) {
				commands[i]->func(&state, args);
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

			free(args);
			continue;
		}

		printf("%s: command not found\n", cmd);
	}

	free(paths);
	return 0;
}

char** get_arguments(const char* input) {
    char* cmd = strdup(input); 
    if (!cmd) return NULL;

    int max_args = strlen(input) / 2 + 1; // Max possible args
    char** args = calloc(max_args + 1, sizeof(char*));
    
    int arg_idx = 0;
    char* src = cmd;
    char* dst = cmd;

    bool in_single_quote = false;
	bool in_double_quote = false;

	bool escape = false;

    bool new_arg_started = true;

    while (*src) {
		if (escape) {
            if (new_arg_started) {
				args[arg_idx++] = dst;
                new_arg_started = false;
            }

			*dst++ = *src++;
			escape = false;
			continue;
		}

		if (*src == '\\' && !in_single_quote && !in_double_quote) {
			escape = true;
			src++; // Skip the backslash
			continue;
		} else if (*src == '\\' && in_double_quote) {
			if (*(src + 1) == '\"' || *(src + 1) == '\\') {
				escape = true;
				src++; // Skip the backslash
				continue;
			}
		}

        if (*src == '\'' && !in_double_quote) {
            in_single_quote = !in_single_quote;
            src++; // Skip the quote in the output
            continue;
        }

		if (*src == '\"' && !in_single_quote) {
			in_double_quote = !in_double_quote;
			src++; // Skip the quote in the output
			continue;
		}

        if (*src == ' ' && !in_single_quote && !in_double_quote) {
            if (!new_arg_started) {
                *dst++ = '\0'; // Terminate the previous argument
                new_arg_started = true;
            }
            src++; // Skip the space
        } else {
            // If we just finished a gap, this is the start of a new arg
            if (new_arg_started) {
                args[arg_idx++] = dst; // Store the address of the write head
                new_arg_started = false;
            }
            
            *dst++ = *src++; 
        }
    }
    
    *dst = '\0';
    
    if (in_single_quote) {
        fprintf(stderr, "Unterminated quote\n");
        free(cmd);
        free(args);
        return NULL;
    }

	if (in_double_quote) {
		fprintf(stderr, "Unterminated quote\n");
		free(cmd);
		free(args);
		return NULL;
	}

	if (escape) {
		fprintf(stderr, "Unterminated escape\n");
		free(cmd);
		free(args);
		return NULL;
	}

    return args;
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
