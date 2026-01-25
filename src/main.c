#include <stdio.h>
#include <string.h>

int main(int argc, char *argv[]) {
	// Flush after every printf
	setbuf(stdout, NULL);

	while (1) {
		printf("$ ");

		char input[256] = {0};
		fgets(input, sizeof(input), stdin);

		input[strcspn(input, "\n")] = 0;

		if (strcmp(input, "exit") == 0) {
			break;
		} else if (strcspn(input, "echo ") == 0) {
			printf("%s\n", input + 5);
			continue;
		} else if (strcspn(input, "type ") == 0) {
			const char *name = input + 5;
			if (strcmp(name, "echo") == 0) {
				printf("echo is a shell builtin\n");
				continue;
			} else if (strcmp(name, "exit") == 0) {
				printf("exit is a shell builtin\n");
				continue;
			} else if (strcmp(name, "type") == 0) {
				printf("type is a shell builtin\n");
				continue;
			} else {
				printf("%s: not found\n", name);
				continue;
			}
		}
		printf("%s: command not found\n", input);
	}

	return 0;
}
