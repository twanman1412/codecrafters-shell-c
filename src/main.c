#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <sys/types.h>
#include <sys/stat.h>

#ifdef _WIN32
#define os_pathsep ";"
#else
#define os_pathsep ":"
#endif

void echo(const char *str);
void type(const char *name);

char* path;
char** paths;
char** get_paths();

int main(int argc, char *argv[]) {
	// Flush after every printf
	setbuf(stdout, NULL);
	path= getenv("PATH");

	while (1) {
		printf("$ ");

		char input[256] = {0};
		fgets(input, sizeof(input), stdin);

		input[strcspn(input, "\n")] = 0;

		if (strcmp(input, "exit") == 0) {
			break;
		} else if (strcspn(input, "echo ") == 0) {
			echo(input + 5);
			continue;
		} else if (strcspn(input, "type ") == 0) {
			type(input + 5);
			continue;
		}
		printf("%s: command not found\n", input);
	}

	return 0;
}

void echo(const char *str) {
	printf("%s\n", str);
}

void type(const char *name) {
	if (strcmp(name, "echo") == 0) {
		printf("echo is a shell builtin\n");
	} else if (strcmp(name, "exit") == 0) {
		printf("exit is a shell builtin\n");
	} else if (strcmp(name, "type") == 0) {
		printf("type is a shell builtin\n");
	} else {
		if (paths == NULL) paths = get_paths();
		int i = 0;
		char* path = paths[i];
		while (path != NULL) {
			// Allocate space for full path + slash + name + null terminator
			char* fullpath = malloc(strlen(path) + strlen(name) + 2);
			sprintf(fullpath, "%s/%s", path, name);

			struct stat perm;
			if (stat(fullpath, &perm) == 0 && perm.st_mode & S_IXUSR) {
				printf("%s is %s\n", name, fullpath);
				free(fullpath);
				return;
			}

			free(fullpath);
			path = paths[++i];
		}

		printf("%s: not found\n", name);
	}
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
