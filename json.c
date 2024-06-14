#include "json.h"

#include <stdio.h>
#include <sys/types.h>

#define MAX_JSON_NODES 420420
#define MAX_CHARACTERS_IN_JSON_FILE 420420
#define MAX_TOKENS 420420

struct json_node json_nodes[MAX_JSON_NODES];

enum json_error json_error;

static char text[MAX_CHARACTERS_IN_JSON_FILE];

struct token {
	size_t offset;
	size_t length;
};
static struct token tokens[MAX_TOKENS];

static bool tokenize(void) {
	(void)tokens;
	return false;
}

static bool read_text(char *json_file_path) {
	FILE *f = fopen(json_file_path, "r");
	if (!f) {
		json_error = JSON_ERROR_FAILED_TO_OPEN_JSON_FILE;
		return true;
	}

	size_t bytes_read = fread(text, sizeof(char), MAX_CHARACTERS_IN_JSON_FILE, f);

	int is_eof = feof(f);
	int err = ferror(f);

    if (fclose(f)) {
		json_error = JSON_ERROR_FAILED_TO_CLOSE_JSON_FILE;
		return true;
    }

	if (bytes_read == 0) {
		json_error = JSON_ERROR_JSON_FILE_IS_EMPTY;
		return true;
	}
	if (!is_eof || bytes_read == MAX_CHARACTERS_IN_JSON_FILE) {
		json_error = JSON_ERROR_JSON_FILE_TOO_BIG;
		return true;
	}
	if (err) {
		printf("err: %d\n", err);
		json_error = JSON_ERROR_JSON_FILE_READING_ERROR;
		return true;
	}

	text[bytes_read] = '\0';

	printf("text: '%s'\n", text);

	return false;
}

bool json_parse(char *json_file_path, struct json_node *returned) {
	json_error = JSON_NO_ERROR;

	if (read_text(json_file_path)) {
		return true;
	}

	if (tokenize()) {
		return true;
	}

	(void)returned;
	return false;
}
