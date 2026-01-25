#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include <sys/types.h>
#include <sys/stat.h>
#include <sys/wait.h>

#ifdef _WIN32
#define os_pathsep ";"
#else
#define os_pathsep ":"
#endif

void echo(char **args);
void type(char **args);

char* path;
char** paths;
char** get_paths();

char* get_command(const char* input);
char** get_arguments(const char* input);
char* get_executable(const char* name);

int main(int argc, char *argv[]) {
	// Flush after every printf
	setbuf(stdout, NULL);
	path= getenv("PATH");

	while (1) {
		printf("$ ");

		char input[256] = {0};
		fgets(input, sizeof(input), stdin);

		input[strcspn(input, "\n")] = 0;
		const char* cmd = get_command(input);
		char** args = get_arguments(input);

		if (strcmp(cmd, "exit") == 0) {
			break;
		} else if (strcmp(cmd, "echo") == 0) {
			echo(args);
			continue;
		} else if (strcmp(cmd, "type") == 0) {
			type(args);
			continue;
		} 

		char* exe = get_executable(cmd);
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

void echo(char **args) {
	for (int i = 1; args[i] != NULL; i++) {
		printf("%s", args[i]);
		if (args[i + 1] != NULL) {
			printf(" ");
		}
	}
	printf("\n");
}

void type(char **args) {
	char* name = args[1]; // first argument is the command name
	if (strcmp(name, "echo") == 0) {
		printf("echo is a shell builtin\n");
	} else if (strcmp(name, "exit") == 0) {
		printf("exit is a shell builtin\n");
	} else if (strcmp(name, "type") == 0) {
		printf("type is a shell builtin\n");
	} else {
		char* cmd = get_command(name);
		char* exe = get_executable(cmd);
		if (exe != NULL) {
			printf("%s is %s\n", name, exe);
			free(exe);
		} else {
			printf("%s: not found\n", name);
		}

		free(cmd);
	}
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

char* get_executable(const char* name) {
	if (paths == NULL) paths = get_paths();
	int i = 0;
	char* path = paths[i];
	while (path != NULL) {
		// Allocate space for full path + slash + name + null terminator
		char* fullpath = malloc(strlen(path) + strlen(name) + 2);
		sprintf(fullpath, "%s/%s", path, name);

		struct stat perm;
		if (stat(fullpath, &perm) == 0 && perm.st_mode & S_IXUSR) {
			return fullpath;
		}

		free(fullpath);
		path = paths[++i];
	}

	return NULL;
}

char** get_paths() {
	int count = 0;
	char* path_copy = strdup(path); // strtok modifies the string
	char* token = strtok(path_copy, os_pathsep);
	while (token != NULL) {
		count++;
		token = strtok(NULL, os_pathsep);
	}
	free(path_copy);

	// Allocate array of pointers + NULL terminator
	char** result = malloc((count + 1) * sizeof(char*));
	path_copy = strdup(path);
	token = strtok(path_copy, os_pathsep);
	int index = 0;
	while (token != NULL) {
		result[index++] = strdup(token);
		token = strtok(NULL, os_pathsep);
	}
	result[index] = NULL;
	free(path_copy);

	return result;
}
