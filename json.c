#include "json.h"

#include <ctype.h>
#include <setjmp.h>
#include <stdio.h>
#include <sys/types.h>

#define MAX_CHARACTERS_IN_JSON_FILE 420420
#define MAX_TOKENS 420420
#define MAX_STRINGS_CHARACTERS 420420
#define MAX_NODES 420420
#define MAX_FIELDS 420420
#define MAX_NODES_PER_STACK_FRAME 1337

#define JSON_ERROR(error) { json_error = error; json_error_line_number = __LINE__; longjmp(error_jmp_buffer, 1); } while(0)

static jmp_buf error_jmp_buffer;

enum json_error json_error;
int json_error_line_number;

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

struct json_node nodes[MAX_NODES];
static size_t nodes_size;

static char strings[MAX_STRINGS_CHARACTERS];
static size_t strings_size;

struct json_field fields[MAX_FIELDS];
static size_t fields_size;

static struct json_string parse_string(size_t *i);
static struct json_array parse_array(size_t *i);

static void push_node(struct json_node node) {
	if (nodes_size + 1 > MAX_NODES) {
		JSON_ERROR(JSON_ERROR_TOO_MANY_JSON_NODES);
	}
	nodes[nodes_size++] = node;
}

static struct json_node parse_object(size_t *i) {
	struct json_node node;

	node->type = JSON_NODE_OBJECT;

	(void)i;
}

static struct json_node parse_array(size_t *i) {
	struct json_node node;

	node->data.array.nodes_offset = nodes_size;

	node->type = JSON_NODE_ARRAY;

	// struct json_node child_nodes[MAX_NODES_PER_STACK_FRAME];
	// size_t child_nodes_size = 0;

	while (*i < tokens_size) {
		struct token *t = tokens + *i;

		switch (t->type) {
		case TOKEN_TYPE_STRING:
			// size_t child_node_index = nodes_size;
			parse_string(i);
			// child_nodes[child_nodes_size++] = nodes[child_node_index];
			break;
		// case TOKEN_TYPE_ARRAY_OPEN:
		// 	if (parse_array(node, i)) {
		// 		return true;
		// 	}
		// 	break;
		case TOKEN_TYPE_ARRAY_CLOSE:
			return node;
		// case TOKEN_TYPE_OBJECT_OPEN:
		// 	node->type = JSON_NODE_OBJECT;
		// 	break;
		case TOKEN_TYPE_OBJECT_CLOSE:
			JSON_ERROR(JSON_ERROR_UNMATCHED_OBJECT_CLOSE);
		}

		(*i)++;
	}

	abort();
}

static void push_string(size_t offset, size_t length) {
	if (strings_size + length >= MAX_STRINGS_CHARACTERS) {
		JSON_ERROR(JSON_ERROR_TOO_MANY_STRINGS_CHARACTERS);
	}
	for (size_t i = 0; i < length; i++) {
		strings[strings_size++] = text[offset + i];
	}
	strings[strings_size++] = '\0';
}

static struct json_node parse_string(size_t *i) {
	struct json_node node;

	node->type = JSON_NODE_STRING;

	node->data.string.str = strings + strings_size;

	struct token *t = tokens + *i;
	push_string(t->offset, t->length));
}

static struct json_node parse(size_t *i) {
	while (*i < tokens_size) {
		struct token *t = tokens + *i;

		switch (t->type) {
		case TOKEN_TYPE_STRING:
			parse_string(i);
			break;
		case TOKEN_TYPE_ARRAY_OPEN:
			parse_array(i);
			break;
		case TOKEN_TYPE_ARRAY_CLOSE:
			JSON_ERROR(JSON_ERROR_UNMATCHED_ARRAY_CLOSE);
		case TOKEN_TYPE_OBJECT_OPEN:
			parse_object(i);
			break;
		case TOKEN_TYPE_OBJECT_CLOSE:
			JSON_ERROR(JSON_ERROR_UNMATCHED_OBJECT_CLOSE);
		}

		(*i)++;
	}

	abort();
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
		} else if (!isspace(text[i])) {
			json_error = JSON_ERROR_UNRECOGNIZED_CHARACTER;
			return true;
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
	nodes_size = 0;
	strings_size = 0;
	fields_size = 0;
}

bool json_parse(char *json_file_path, struct json_node *returned) {
	if (setjmp(error_jmp_buffer)) {
		return true;
	}

	reset();

	if (read_text(json_file_path)) {
		return true;
	}

	if (tokenize()) {
		return true;
	}

	size_t token_index = 0;
	if (parse(&token_index)) {
		return true;
	}

	if (nodes_size > 0) {
		*returned = nodes[0];
	}

	return false;
}
