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

char* longest_common_prefix(char** strings) {
	if (strings == NULL || strings[0] == NULL) return NULL;

	char* prefix = strdup(strings[0]);
	if (prefix == NULL) return NULL;

	for (int i = 1; strings[i] != NULL; i++) {
		int j = 0;
		while (prefix[j] && strings[i][j] && prefix[j] == strings[i][j]) {
			j++;
		}
		prefix[j] = '\0'; // Truncate prefix
		if (prefix[0] == '\0') {
			break; // No common prefix
		}
	}

	return prefix;
}
