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
	  }
	  printf("%s: command not found\n", input);
  }

  return 0;
}
