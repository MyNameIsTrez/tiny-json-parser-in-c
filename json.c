#include "json.h"

#include <stdio.h>
#include <sys/types.h>

#define MAX_JSON_NODES 420420
#define MAX_CHARACTERS_IN_JSON_FILE 420420

struct json_node json_nodes[MAX_JSON_NODES];

enum json_error json_error;

static char text[MAX_CHARACTERS_IN_JSON_FILE];

bool json_parse(char *json_file_path, struct json_node *returned) {
	json_error = JSON_NO_ERROR;

	FILE *f = fopen(json_file_path, "r");
	if (!f) {
		json_error = JSON_ERROR_FAILED_TO_OPEN_JSON_FILE;
		return false;
	}

	size_t bytes_read = fread(text, sizeof(char), MAX_CHARACTERS_IN_JSON_FILE, f);

	int is_eof = feof(f);
	int err = ferror(f);

    if (fclose(f)) {
		json_error = JSON_ERROR_FAILED_TO_CLOSE_JSON_FILE;
		return false;
    }

	if (bytes_read == 0) {
		json_error = JSON_ERROR_JSON_FILE_IS_EMPTY;
		return false;
	}
	if (!is_eof || bytes_read == MAX_CHARACTERS_IN_JSON_FILE) {
		json_error = JSON_ERROR_JSON_FILE_TOO_BIG;
		return false;
	}
	if (err) {
		printf("err: %d\n", err);
		json_error = JSON_ERROR_JSON_FILE_READING_ERROR;
		return false;
	}

	text[bytes_read] = '\0';

	printf("text: '%s'\n", text);

	(void)returned;
	return true;
}
