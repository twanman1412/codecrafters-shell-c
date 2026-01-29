#include "utils.h"
#include <string.h>
#include <unistd.h>

void sort_strings(char** strings) {
	if (strings == NULL) return;

	for (int i = 0; strings[i] != NULL; i++) {
		for (int j = i + 1; strings[j] != NULL; j++) {
			if (strcmp(strings[i], strings[j]) > 0) {
				char* temp = strings[i];
				strings[i] = strings[j];
				strings[j] = temp;
			}
		}
	}
}
