#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <stdbool.h>
#include <termios.h>
#include <dirent.h>

#include <sys/types.h>
#include <sys/stat.h>
#include <sys/wait.h>

#include "main.h"
#include "commands.h"
#include "utils.h"

#ifdef _WIN32
#define os_pathsep ";"
#else
#define os_pathsep ":"
#endif

#define MAX_OUTPUT_SIZE 4096

extern const struct Command* commands[];

int main(int argc, char *argv[]) {
	// Flush after every printf
	setbuf(stdout, NULL);

	struct termios oldt, newt;
	tcgetattr(STDIN_FILENO, &oldt);
	newt = oldt;

	newt.c_lflag &= ~(ICANON | ECHO); 
	tcsetattr(STDIN_FILENO, TCSANOW, &newt);

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
		.paths = paths,
		.exit = false
	};

	// ======= Main loop =======
main:
	while (!state.exit) {
		printf("$ ");

		char input[256] = {0};
		int i = 0;
		char c = getchar();
		while (c != '\n') {
			if (c == 0x1B) { // escape sequences
				getchar(); // Skip the next char 
				getchar(); // Skip the next char
				c = getchar(); // Get new char
				continue;
			}

			if (c == '\t' || c == 0x09) { // Tab character
				char** completions = get_autocomplete(state, input);
				if (completions != NULL || completions[0] == NULL) {
					if (completions[0] != NULL && completions[1] == NULL) {
						// Single match, autocomplete
						int len = strlen(completions[0]);
						int input_len = strlen(input);
						for (int j = input_len; j < len; j++) {
							input[i++] = completions[0][j];
							putchar(completions[0][j]);
						}

						input[i++] = ' ';
						putchar(' ');

						input[i] = '\0';

						free(completions[0]);
						free(completions);

						c = getchar();
						continue;
					} else {
						// Multiple matches
						char* lcp = longest_common_prefix(completions);
						if (lcp != NULL && strlen(lcp) > strlen(input)) {
							// Autocomplete to longest common prefix
							int len = strlen(lcp);
							int input_len = strlen(input);
							for (int j = input_len; j < len; j++) {
								input[i++] = lcp[j];
								putchar(lcp[j]);
							}
							free(lcp);

							input[i] = '\0';

							c = getchar();
							continue;
						}

						printf("\x07");
						c = getchar();
						if (c != '\t') {
							continue;
						}

						sort_strings(completions);
						printf("\n");
						for (int j = 0; completions[j] != NULL; j++) {
							printf("%s  ", completions[j]);
							free(completions[j]);
						}
						free(completions);
						printf("\n$ %s", input);
						c = getchar();
						continue;
					}
				}

				printf("\x07"); // Bell character
				c = getchar();
				continue; // Skip adding tab to input
			}
			if (c == '\b' || c == 0x08 || c == 0x7F) { // Backspaces
				if (i > 0) {
					i--;
					input[i] = '\0';
					printf("\r$ %s ", input); // Overwrite last char
					printf("\b"); // Move back one space
				}
				c = getchar();
				continue; // Skip adding backspace to input	
			}

			input[i++] = c;
			putchar(c);
			c = getchar();
		}

		putchar('\n');
		if (strlen(input) == 0) {
			continue;
		}

		char** args = get_arguments(input);
		parse_and_execute_command(&state, (const char**) args, stdin, stdout, stderr);
	}

	free(paths);
	tcsetattr(STDIN_FILENO, TCSANOW, &oldt);
	return 0;
}

void parse_and_execute_command(struct State *state, const char** args, FILE* in, FILE* out, FILE* err) {
	const char* cmd = args[0];

	for (int i = 0; args[i] != NULL; i++) {
		const char *arg = args[i];
		if (strcmp(arg, ">") == 0 || strcmp(arg, "1>") == 0 ||
				strcmp(arg, ">>") == 0 || strcmp(arg, "1>>") == 0) {

			bool append = false;
			if (strcmp(arg, ">>") == 0 || strcmp(arg, "1>>") == 0) {
				append = true;
			}

			const char* filename = args[i + 1];
			if (filename == NULL) {
				fprintf(stderr, "Syntax error: expected filename after %s\n", arg);
				free(args);
				return;
			}
			if (args[i + 2] != NULL) {
				fprintf(stderr, "Syntax error: too many arguments after %s\n", arg);
				free(args);
				return;
			}

			const char** new_args = malloc((i + 1) * sizeof(char*));
			for (int j = 0; j < i; j++) {
				new_args[j] = args[j];
			}
			new_args[i] = NULL;

			FILE *file;
			if (append) {
				file = fopen(filename, "a");
			} else {
				file = fopen(filename, "w");
			}
			if (file == NULL) {
				fprintf(stderr, "Error: could not open %s for writing\n", filename);
				free(args);
				free(new_args);
				return;
			}

			execute_command(state, new_args, in, file, err);

			fclose(file);
			free(args);
			free(new_args);
			return;
		}

		if (strcmp(arg, "2>") == 0 || strcmp(arg, "2>>") == 0) {

			bool append = false;
			if (strcmp(arg, "2>>") == 0) {
				append = true;
			}

			const char* filename = args[i + 1];
			if (filename == NULL) {
				fprintf(stderr, "Syntax error: expected filename after %s\n", arg);
				free(args);
				return;
			}
			if (args[i + 2] != NULL) {
				fprintf(stderr, "Syntax error: too many arguments after %s\n", arg);
				free(args);
				return;
			}

			const char** new_args = malloc((i + 1) * sizeof(char*));
			for (int j = 0; j < i; j++) {
				new_args[j] = args[j];
			}
			new_args[i] = NULL;

			FILE *file;
			if (append) {
				file = fopen(filename, "a");
			} else {
				file = fopen(filename, "w");
			}

			if (file == NULL) {
				fprintf(stderr, "Error: could not open %s for writing\n", filename);
				free(args);
				free(new_args);
				return;
			}

			execute_command(state, new_args, in, out, file);

			fclose(file);
			free(args);
			free(new_args);
			return;
		}

		if (strcmp(arg, "|") == 0) {
			const char** left_args = malloc((i + 1) * sizeof(char*));
			int j;
			for (j = 0; j < i; j++) {
				left_args[j] = args[j];
			}
			left_args[j] = NULL;

			int k; for(k = 0; args[k] != NULL; k++);

			const char** right_args = malloc((k - i) * sizeof(char*));
			for (int j = i + 1, k = 0; args[j] != NULL; j++) {
				right_args[k++] = args[j];
			}
			right_args[k] = NULL;

			int pipefd[2];
			if (pipe(pipefd) == -1) {
				perror("pipe failed");
				exit(-1);
			}

			pid_t left_pid = fork();
			if (left_pid == 0) {
				// Left child: write to pipe
				dup2(pipefd[1], STDOUT_FILENO);
				close(pipefd[0]);
				close(pipefd[1]);
				execute_command(state, left_args, in, stdout, err);
				exit(0);
			}

			pid_t right_pid = fork();
			if (right_pid == 0) {
				// Right child: read from pipe
				dup2(pipefd[0], STDIN_FILENO);
				close(pipefd[0]);
				close(pipefd[1]);
				parse_and_execute_command(state, right_args, stdin, out, err);
				exit(0);
			}

			// Parent closes both ends
			close(pipefd[0]);
			close(pipefd[1]);
			waitpid(left_pid, NULL, 0);
			waitpid(right_pid, NULL, 0);

			free(args);
			free(left_args);
			free(right_args);
			return;
		}
	}

	execute_command(state, args, in, out, err);
}

void execute_command(struct State *state, const char** args, FILE* in, FILE* out, FILE* err) {
	const char* cmd = args[0];

	for (int i = 0; commands[i] != NULL; i++) {
		if (strcmp(cmd, commands[i]->name) == 0) {
			commands[i]->func(state, args, in, out, err);
			return;
		}
	}

	char* exe = get_executable(*state, cmd);
	if (exe != NULL) {
		pid_t pid = fork();
		if (pid == 0) {
			// Redirect stdin
			if (in && fileno(in) != STDIN_FILENO) {
				if (dup2(fileno(in), STDIN_FILENO) == -1) {
					perror("dup2 stdin failed");
					exit(-1);
				}
			}

			// Redirect stdout
			if (out && fileno(out) != STDOUT_FILENO) {
				if (dup2(fileno(out), STDOUT_FILENO) == -1) {
					perror("dup2 stdout failed");
					exit(-1);
				}
			}

			// Redirect stderr
			if (err && fileno(err) != STDERR_FILENO) {
				if (dup2(fileno(err), STDERR_FILENO) == -1) {
					perror("dup2 stderr failed");
					exit(-1);
				}
			}

			// Execute the command
			execv(exe, (char**) args);

			// If execv returns, there was an error
			exit(-1);
		} else if (pid > 0) {
			int status;
			waitpid(pid, &status, 0);
		} else {
			// Fork failed
			perror("fork failed");
			exit(-1);
		}

		return;
	}

	fprintf(err, "%s: command not found\n", cmd);
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

char** get_autocomplete(struct State state, const char* input) {

	char* cmd = strdup(input);
	if (!cmd) return NULL;

	char** matches = malloc(256 * sizeof(char*));
	int idx = 0;

	for (int i = 0; commands[i] != NULL; i++) {
		if (strncmp(cmd, commands[i]->name, strlen(cmd)) == 0) {
			matches[idx++] = strdup(commands[i]->name);
		}
	}

	if (idx > 0) {
		free(cmd);
		matches[idx] = NULL;
		return matches;
	}

	for (int i = 0; state.paths[i] != NULL; i++) {
		char* path = state.paths[i];

		DIR* dir = opendir(path);
		if (dir == NULL) {
			continue;
		}

		struct dirent* entry;
		while ((entry = readdir(dir)) != NULL) {
			if (strncmp(cmd, entry->d_name, strlen(cmd)) == 0) {
				char* fullpath = malloc(strlen(path) + strlen(entry->d_name) + 2);
				sprintf(fullpath, "%s/%s", path, entry->d_name);

				struct stat perm;
				if (stat(fullpath, &perm) == 0 && perm.st_mode & S_IXUSR) {
					matches[idx++] = strdup(entry->d_name);
				}
				free(fullpath);
			}
		}
		closedir(dir);
	}

	free(cmd);
	matches[idx] = NULL;
	return matches;
}
