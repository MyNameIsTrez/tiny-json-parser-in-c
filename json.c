#include "json.h"

#include <stdio.h>
#include <sys/types.h>

#define MAX_JSON_NODES 420420
#define MAX_CHARACTERS_IN_JSON_FILE 420420
#define MAX_TOKENS 420420
#define MAX_STRINGS_CHARACTERS 420420

struct json_node json_nodes[MAX_JSON_NODES];

enum json_error json_error;

static char text[MAX_CHARACTERS_IN_JSON_FILE];
static size_t text_size;

enum token_type {
	TOKEN_TYPE_STRING,
	TOKEN_TYPE_ARRAY_OPEN,
	TOKEN_TYPE_ARRAY_CLOSE,
	TOKEN_TYPE_OBJECT_OPEN,
	TOKEN_TYPE_OBJECT_CLOSE,
};

struct token {
	enum token_type type;
	size_t offset;
	size_t length;
};
static struct token tokens[MAX_TOKENS];
static size_t tokens_size;

static char strings[MAX_STRINGS_CHARACTERS];
static size_t strings_size;

static bool push_string(size_t offset, size_t length) {
	if (strings_size + length >= MAX_STRINGS_CHARACTERS) {
		json_error = JSON_ERROR_TOO_MANY_STRINGS_CHARACTERS;
		return true;
	}
	for (size_t i = 0; i < length; i++) {
		strings[strings_size++] = text[offset + i];
	}
	strings[strings_size++] = '\0';
	return false;
}

static bool parse(struct json_node *node) {
	node->type = JSON_NODE_STRING;

	node->data.string.str = strings + strings_size;
	if (push_string(0, 2)) {
		return true;
	}

	return false;
}

static void print_tokens(void) {
	printf("tokens:\n");
	for (size_t i = 0; i < tokens_size; i++) {
		struct token t = tokens[i];
		printf("'%.*s'\n", (int)t.length, text + t.offset);
	}
}

static bool push_token(enum token_type type, size_t offset, size_t length) {
	if (tokens_size + 1 > MAX_TOKENS) {
		json_error = JSON_ERROR_TOO_MANY_TOKENS;
		return true;
	}
	tokens[tokens_size++] = (struct token){
		.type = type,
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
				if (push_token(TOKEN_TYPE_STRING, string_start_index, i - string_start_index + 1)) {
					return true;
				}
			} else {
				string_start_index = i;
			}
			in_string = !in_string;
		} else if (text[i] == '[') {
			if (push_token(TOKEN_TYPE_ARRAY_OPEN, i, 1)) {
				return true;
			}
		} else if (text[i] == ']') {
			if (push_token(TOKEN_TYPE_ARRAY_CLOSE, i, 1)) {
				return true;
			}
		} else if (text[i] == '{') {
			if (push_token(TOKEN_TYPE_OBJECT_OPEN, i, 1)) {
				return true;
			}
		} else if (text[i] == '}') {
			if (push_token(TOKEN_TYPE_OBJECT_CLOSE, i, 1)) {
				return true;
			}
		}
		i++;
	}

	print_tokens();

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

static void reset(void) {
	json_error = JSON_NO_ERROR;
	text_size = 0;
	tokens_size = 0;
	strings_size = 0;
}

bool json_parse(char *json_file_path, struct json_node *returned) {
	reset();

	if (read_text(json_file_path)) {
		return true;
	}

	if (tokenize()) {
		return true;
	}

	if (parse(returned)) {
		return true;
	}

	(void)returned;
	return false;
}
