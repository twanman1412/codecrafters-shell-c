#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <stdbool.h>
#include <termios.h>

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

	char* out = malloc(MAX_OUTPUT_SIZE * sizeof(char));
	char* err = malloc(MAX_OUTPUT_SIZE * sizeof(char));

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
				char* completion = get_autocomplete(input);
				if (completion != NULL) {
					strcpy(input, completion);
					input[strlen(completion)] = ' ';
					free(completion);
					printf("\r$ %s", input);
					// Move cursor to end
					i = strlen(input);
					for (int i = strlen(completion) - strlen(input); i > 1; i--) {
						printf("\x1B[C"); // Move cursor right
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
		char*  cmd  = args[0];

		for (int i = 0; args[i] != NULL; i++) {
			char *arg = args[i];
			if (strcmp(arg, ">") == 0 || strcmp(arg, "1>") == 0 ||
					strcmp(arg, ">>") == 0 || strcmp(arg, "1>>") == 0) {

				bool append = false;
				if (strcmp(arg, ">>") == 0 || strcmp(arg, "1>>") == 0) {
					append = true;
				}

				char* filename = args[i + 1];
				if (filename == NULL) {
					sprintf(err, "Syntax error: expected filename after %s\n", arg);
					free(args);
					goto main;
				}
				if (args[i + 2] != NULL) {
					sprintf(err, "Syntax error: too many arguments after %s\n", arg);
					free(args);
					goto main;
				}

				char** new_args = malloc((i + 1) * sizeof(char*));
				for (int j = 0; j < i; j++) {
					new_args[j] = args[j];
				}
				new_args[i] = NULL;

				execute_command(&state, new_args, out, err);
				if (strlen(err) > 0) {
					fputs(err, stderr);
					err[0] = '\0';
				}

				FILE *file;
				if (append) {
					file = fopen(filename, "a");
				} else {
					file = fopen(filename, "w");
				}
				if (file == NULL) {
					sprintf(err, "Error: could not open %s for writing\n", filename);
					free(args);
					free(new_args);
					goto main;
				}

				fputs(out, file);
				fclose(file);
				out[0] = '\0';
				free(args);
				free(new_args);
				goto main;
			}

			if (strcmp(arg, "2>") == 0 || strcmp(arg, "2>>") == 0) {

				bool append = false;
				if (strcmp(arg, "2>>") == 0) {
					append = true;
				}

				char* filename = args[i + 1];
				if (filename == NULL) {
					sprintf(err, "Syntax error: expected filename after %s\n", arg);
					free(args);
					goto main;
				}
				if (args[i + 2] != NULL) {
					sprintf(err, "Syntax error: too many arguments after %s\n", arg);
					free(args);
					goto main;
				}

				char** new_args = malloc((i + 1) * sizeof(char*));
				for (int j = 0; j < i; j++) {
					new_args[j] = args[j];
				}
				new_args[i] = NULL;

				execute_command(&state, new_args, out, err);
				if (strlen(out) > 0) {
					fputs(out, stdout);
					out[0] = '\0';
				}

				FILE *file;
				if (append) {
					file = fopen(filename, "a");
				} else {
					file = fopen(filename, "w");
				}

				if (file == NULL) {
					sprintf(err, "Error: could not open %s for writing\n", filename);
					free(args);
					free(new_args);
					goto main;
				}

				fputs(err, file);
				fclose(file);
				out[0] = '\0';
				free(args);
				free(new_args);
				goto main;
			}
		}

		execute_command(&state, args, out, err);
		if (strlen(err) > 0) {
			fputs(err, stderr);
			err[0] = '\0';
		}

		if (strlen(out) > 0) {
			fputs(out, stdout);
			out[0] = '\0';
		}
	}

	free(paths);
	tcsetattr(STDIN_FILENO, TCSANOW, &oldt);
	return 0;
}

void execute_command(struct State *state, char** args, char* out, char* err) {
	char* cmd = args[0];

	for (int i = 0; commands[i] != NULL; i++) {
		if (strcmp(cmd, commands[i]->name) == 0) {
			commands[i]->func(state, args, out, err);
			return;
		}
	}

	char* exe = get_executable(*state, cmd);
	if (exe != NULL) {

		int stdout_pipe[2];
		int stderr_pipe[2];

		if (pipe(stdout_pipe) == -1 || pipe(stderr_pipe) == -1) {
			perror("pipe failed");
			exit(-1);
		}
		
		pid_t pid = fork();
		if (pid == 0) {
			// Close read end
			close(stdout_pipe[0]); 
			close(stderr_pipe[0]);

			// Redirect stdout and stderr to pipes
			if (dup2(stdout_pipe[1], STDOUT_FILENO) == -1) {
				perror("dup2 stdout failed");
				exit(-1);
			}
			if (dup2(stderr_pipe[1], STDERR_FILENO) == -1) {
				perror("dup2 stderr failed");
				exit(-1);
			}

			// Close the write ends after duplicating
			close(stdout_pipe[1]);
			close(stderr_pipe[1]);

			// Execute the command
			execv(exe, args);

			// If execv returns, there was an error
			exit(-1);
		} else if (pid > 0) {
 			// Close write end
			close(stdout_pipe[1]); 
			close(stderr_pipe[1]);

			int status;
			waitpid(pid, &status, 0);

			// Read stdout
			ssize_t nbytes = read(stdout_pipe[0], out, MAX_OUTPUT_SIZE - 1);
			if (nbytes >= 0) {
				out[nbytes] = '\0';
			} else {
				out[0] = '\0';
			}
			close(stdout_pipe[0]);

			nbytes = read(stderr_pipe[0], err, MAX_OUTPUT_SIZE - 1);
			if (nbytes >= 0) {
				err[nbytes] = '\0';
			} else {
				err[0] = '\0';
			}
			close(stderr_pipe[0]);

		} else {
			// Fork failed
			perror("fork failed");
			exit(-1);
		}

		return;
	}

	sprintf(err, "%s: command not found\n", cmd);
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

char* get_autocomplete(const char* input) {

	char* cmd = strdup(input);
	if (!cmd) return NULL;

	// No internal commands longer than 7 characters
	char* match = malloc(256 * sizeof(char));
	match[0] = '\0';

	for (int i = 0; commands[i] != NULL; i++) {
		if (strncmp(cmd, commands[i]->name, strlen(cmd)) == 0) {
			if (strlen(match) == 0) {
				sprintf(match, "%s", commands[i]->name);
			} else {
				return NULL;
			}
		}
	}

	return match;
}
