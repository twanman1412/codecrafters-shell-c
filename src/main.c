#include <stdio.h>
#include <string.h>

int main(int argc, char *argv[]) {
  // Flush after every printf
  setbuf(stdout, NULL);

  printf("$ ");

  char input[256] = {0};
  fgets(input, sizeof(input), stdin);

  input[strcspn(input, "\n")] = 0;
  printf("%s: command not found\n", input);

  return 0;
}
