#include "json.h"

#include <ctype.h>
#include <setjmp.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>

#define MAX_CHARACTERS_IN_JSON_FILE 420420
#define MAX_TOKENS 420420
#define MAX_STRINGS_CHARACTERS 420420
#define MAX_NODES 420420
#define MAX_FIELDS 420420
#define MAX_NODES_PER_STACK_FRAME 1337

#define JSON_ERROR(error) {\
	json_error = error;\
	json_error_line_number = __LINE__;\
	longjmp(error_jmp_buffer, 1);\
}

static jmp_buf error_jmp_buffer;

enum json_error json_error;
int json_error_line_number;
char *json_error_messages[] = {
	[JSON_NO_ERROR] = "JSON_NO_ERROR",
	[JSON_ERROR_FAILED_TO_OPEN_JSON_FILE] = "JSON_ERROR_FAILED_TO_OPEN_JSON_FILE",
	[JSON_ERROR_FAILED_TO_CLOSE_JSON_FILE] = "JSON_ERROR_FAILED_TO_CLOSE_JSON_FILE",
	[JSON_ERROR_JSON_FILE_IS_EMPTY] = "JSON_ERROR_JSON_FILE_IS_EMPTY",
	[JSON_ERROR_JSON_FILE_TOO_BIG] = "JSON_ERROR_JSON_FILE_TOO_BIG",
	[JSON_ERROR_JSON_FILE_READING_ERROR] = "JSON_ERROR_JSON_FILE_READING_ERROR",
	[JSON_ERROR_TOO_MANY_TOKENS] = "JSON_ERROR_TOO_MANY_TOKENS",
	[JSON_ERROR_UNRECOGNIZED_CHARACTER] = "JSON_ERROR_UNRECOGNIZED_CHARACTER",
	[JSON_ERROR_TOO_MANY_JSON_NODES] = "JSON_ERROR_TOO_MANY_JSON_NODES",
	[JSON_ERROR_TOO_MANY_STRINGS_CHARACTERS] = "JSON_ERROR_TOO_MANY_STRINGS_CHARACTERS",
	[JSON_ERROR_UNMATCHED_ARRAY_CLOSE] = "JSON_ERROR_UNMATCHED_ARRAY_CLOSE",
	[JSON_ERROR_UNMATCHED_OBJECT_CLOSE] = "JSON_ERROR_UNMATCHED_OBJECT_CLOSE",
};

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

static struct json_node parse_string(size_t *i);
static struct json_node parse_array(size_t *i);

static void push_node(struct json_node node) {
	if (nodes_size + 1 > MAX_NODES) {
		JSON_ERROR(JSON_ERROR_TOO_MANY_JSON_NODES);
	}
	nodes[nodes_size++] = node;
}

static struct json_node parse_object(size_t *i) {
	struct json_node node;

	node.type = JSON_NODE_OBJECT;
	(*i)++;

	node.data.object.fields_offset = fields_size;
	node.data.object.field_count = 0;

	struct json_node child_nodes[MAX_NODES_PER_STACK_FRAME];

	while (*i < tokens_size) {
		struct token *t = tokens + *i;

		switch (t->type) {
		case TOKEN_TYPE_STRING:
			child_nodes[node.data.object.field_count++] = parse_string(i);
			break;
		case TOKEN_TYPE_ARRAY_OPEN:
			child_nodes[node.data.object.field_count++] = parse_array(i);
			break;
		case TOKEN_TYPE_ARRAY_CLOSE:
			JSON_ERROR(JSON_ERROR_UNMATCHED_ARRAY_CLOSE);
		case TOKEN_TYPE_OBJECT_OPEN:
			child_nodes[node.data.object.field_count++] = parse_object(i);
			break;
		case TOKEN_TYPE_OBJECT_CLOSE:
			for (size_t i = 0; i < node.data.object.field_count; i++) {
				push_node(child_nodes[i]);
			}
			return node;
		}

		(*i)++;
	}

	abort();
}

static struct json_node parse_array(size_t *i) {
	struct json_node node;

	node.type = JSON_NODE_ARRAY;
	(*i)++;

	node.data.array.nodes_offset = nodes_size;
	node.data.array.node_count = 0;

	struct json_node child_nodes[MAX_NODES_PER_STACK_FRAME];

	while (*i < tokens_size) {
		struct token *t = tokens + *i;

		switch (t->type) {
		case TOKEN_TYPE_STRING:
			child_nodes[node.data.array.node_count++] = parse_string(i);
			break;
		case TOKEN_TYPE_ARRAY_OPEN:
			child_nodes[node.data.array.node_count++] = parse_array(i);
			break;
		case TOKEN_TYPE_ARRAY_CLOSE:
			for (size_t i = 0; i < node.data.array.node_count; i++) {
				push_node(child_nodes[i]);
			}
			return node;
		case TOKEN_TYPE_OBJECT_OPEN:
			child_nodes[node.data.array.node_count++] = parse_object(i);
			break;
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

	node.type = JSON_NODE_STRING;

	node.data.string.str = strings + strings_size;

	struct token *t = tokens + *i;
	push_string(t->offset, t->length);

	return node;
}

static struct json_node parse(size_t *i) {
	struct token *t = tokens + *i;

	switch (t->type) {
	case TOKEN_TYPE_STRING:
		return parse_string(i);
	case TOKEN_TYPE_ARRAY_OPEN:
		return parse_array(i);
	case TOKEN_TYPE_ARRAY_CLOSE:
		JSON_ERROR(JSON_ERROR_UNMATCHED_ARRAY_CLOSE);
	case TOKEN_TYPE_OBJECT_OPEN:
		return parse_object(i);
	case TOKEN_TYPE_OBJECT_CLOSE:
		JSON_ERROR(JSON_ERROR_UNMATCHED_OBJECT_CLOSE);
	}

	abort();
}

// static void print_tokens(void) {
// 	printf("tokens:\n");
// 	for (size_t i = 0; i < tokens_size; i++) {
// 		struct token t = tokens[i];
// 		printf("'%.*s'\n", (int)t.length, text + t.offset);
// 	}
// }

static void push_token(enum token_type type, size_t offset, size_t length) {
	if (tokens_size + 1 > MAX_TOKENS) {
		JSON_ERROR(JSON_ERROR_TOO_MANY_TOKENS);
	}
	tokens[tokens_size++] = (struct token){
		.type = type,
		.offset = offset,
		.length = length,
	};
}

static void tokenize(void) {
	size_t i = 0;
	bool in_string = false;
	size_t string_start_index;

	while (i < text_size) {
		if (text[i] == '"') {
			if (in_string) {
				push_token(TOKEN_TYPE_STRING, string_start_index, i - string_start_index + 1);
			} else {
				string_start_index = i;
			}
			in_string = !in_string;
		} else if (text[i] == '[') {
			push_token(TOKEN_TYPE_ARRAY_OPEN, i, 1);
		} else if (text[i] == ']') {
			push_token(TOKEN_TYPE_ARRAY_CLOSE, i, 1);
		} else if (text[i] == '{') {
			push_token(TOKEN_TYPE_OBJECT_OPEN, i, 1);
		} else if (text[i] == '}') {
			push_token(TOKEN_TYPE_OBJECT_CLOSE, i, 1);
		} else if (!isspace(text[i]) && !in_string) {
			JSON_ERROR(JSON_ERROR_UNRECOGNIZED_CHARACTER);
		}
		i++;
	}

	// print_tokens();
}

static void read_text(char *json_file_path) {
	FILE *f = fopen(json_file_path, "r");
	if (!f) {
		JSON_ERROR(JSON_ERROR_FAILED_TO_OPEN_JSON_FILE);
	}

	text_size = fread(text, sizeof(char), MAX_CHARACTERS_IN_JSON_FILE, f);

	int is_eof = feof(f);
	int err = ferror(f);

    if (fclose(f)) {
		JSON_ERROR(JSON_ERROR_FAILED_TO_CLOSE_JSON_FILE);
    }

	if (text_size == 0) {
		JSON_ERROR(JSON_ERROR_JSON_FILE_IS_EMPTY);
	}
	if (!is_eof || text_size == MAX_CHARACTERS_IN_JSON_FILE) {
		JSON_ERROR(JSON_ERROR_JSON_FILE_TOO_BIG);
	}
	if (err) {
		JSON_ERROR(JSON_ERROR_JSON_FILE_READING_ERROR);
	}

	text[text_size] = '\0';

	// printf("text: '%s'\n", text);
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

	read_text(json_file_path);

	tokenize();

	size_t token_index = 0;
	*returned = parse(&token_index);

	return false;
}
