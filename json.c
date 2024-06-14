#include "json.h"

#include <stdio.h>
#include <sys/types.h>

#define MAX_JSON_NODES 420420
#define MAX_CHARACTERS_IN_JSON_FILE 420420
#define MAX_TOKENS 420420

struct json_node json_nodes[MAX_JSON_NODES];

enum json_error json_error;

static char text[MAX_CHARACTERS_IN_JSON_FILE];
static size_t text_size;

struct token {
	size_t offset;
	size_t length;
};
static struct token tokens[MAX_TOKENS];
static size_t tokens_size;

static void print_tokens() {
	printf("tokens:\n");
	for (size_t i = 0; i < tokens_size; i++) {
		printf("'%.*s'\n", (int)tokens[i].length, text + tokens[i].offset);
	}
}

static bool push_token(size_t offset, size_t length) {
	if (tokens_size + 1 > MAX_TOKENS) {
		json_error = JSON_ERROR_TOO_MANY_TOKENS;
		return true;
	}
	tokens[tokens_size++] = (struct token){
		.offset = offset,
		.length = length,
	};
	return false;
}

static bool tokenize(void) {
	size_t i = 0;
	bool in_string = false;
	size_t string_start_index;

	while (i < text_size) {
		if (text[i] == '"') {
			if (in_string) {
				if (push_token(string_start_index, i - string_start_index + 1)) {
					return true;
				}
			} else {
				string_start_index = i;
			}
			in_string = !in_string;
		}
		i++;
	}

	print_tokens();

	(void)tokens;
	return false;
}

static bool read_text(char *json_file_path) {
	FILE *f = fopen(json_file_path, "r");
	if (!f) {
		json_error = JSON_ERROR_FAILED_TO_OPEN_JSON_FILE;
		return true;
	}

	text_size = fread(text, sizeof(char), MAX_CHARACTERS_IN_JSON_FILE, f);

	int is_eof = feof(f);
	int err = ferror(f);

    if (fclose(f)) {
		json_error = JSON_ERROR_FAILED_TO_CLOSE_JSON_FILE;
		return true;
    }

	if (text_size == 0) {
		json_error = JSON_ERROR_JSON_FILE_IS_EMPTY;
		return true;
	}
	if (!is_eof || text_size == MAX_CHARACTERS_IN_JSON_FILE) {
		json_error = JSON_ERROR_JSON_FILE_TOO_BIG;
		return true;
	}
	if (err) {
		printf("err: %d\n", err);
		json_error = JSON_ERROR_JSON_FILE_READING_ERROR;
		return true;
	}

	text[text_size] = '\0';

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
