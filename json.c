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
#define MAX_CHILD_NODES 420
#define MAX_RECURSION_DEPTH 42

#define JSON_ERROR(error) {\
	json_error = error;\
	json_error_line_number = __LINE__;\
	longjmp(error_jmp_buffer, 1);\
}

static jmp_buf error_jmp_buffer;

enum json_error json_error;
int json_error_line_number;
char *json_error_messages[] = {
	[JSON_NO_ERROR] = "No error",
	[JSON_ERROR_FAILED_TO_OPEN_FILE] = "Failed to open file",
	[JSON_ERROR_FAILED_TO_CLOSE_FILE] = "Failed to close file",
	[JSON_ERROR_FILE_EMPTY] = "File is empty",
	[JSON_ERROR_FILE_TOO_BIG] = "File is too big",
	[JSON_ERROR_FILE_READING_ERROR] = "File reading error",
	[JSON_ERROR_UNRECOGNIZED_CHARACTER] = "Unrecognized character",
	[JSON_ERROR_UNCLOSED_STRING] = "Unclosed string",
	[JSON_ERROR_TOO_MANY_TOKENS] = "Too many tokens",
	[JSON_ERROR_TOO_MANY_NODES] = "Too many nodes",
	[JSON_ERROR_TOO_MANY_FIELDS] = "Too many fields",
	[JSON_ERROR_TOO_MANY_STRINGS_CHARACTERS] = "Too many strings[] characters",
	[JSON_ERROR_TOO_MANY_CHILD_NODES] = "Too many child nodes",
	[JSON_ERROR_MAX_RECURSION_DEPTH_EXCEEDED] = "Max recursion depth exceeded",
	[JSON_ERROR_EXPECTED_ARRAY_CLOSE] = "Expected ']'",
	[JSON_ERROR_EXPECTED_OBJECT_CLOSE] = "Expected '}'",
	[JSON_ERROR_EXPECTED_COLON] = "Expected colon",
	[JSON_ERROR_EXPECTED_VALUE] = "Expected value",
	[JSON_ERROR_UNEXPECTED_STRING] = "Unexpected string",
	[JSON_ERROR_UNEXPECTED_ARRAY_OPEN] = "Unexpected '['",
	[JSON_ERROR_UNEXPECTED_ARRAY_CLOSE] = "Unexpected ']'",
	[JSON_ERROR_UNEXPECTED_OBJECT_OPEN] = "Unexpected '{'",
	[JSON_ERROR_UNEXPECTED_OBJECT_CLOSE] = "Unexpected '}'",
	[JSON_ERROR_UNEXPECTED_COMMA] = "Unexpected ','",
	[JSON_ERROR_UNEXPECTED_COLON] = "Unexpected ':'",
	[JSON_ERROR_UNEXPECTED_EXTRA_CHARACTER] = "Unexpected extra character",
};

static size_t recursion_depth;

static char text[MAX_CHARACTERS_IN_JSON_FILE];
static size_t text_size;

enum token_type {
	TOKEN_TYPE_STRING,
	TOKEN_TYPE_ARRAY_OPEN,
	TOKEN_TYPE_ARRAY_CLOSE,
	TOKEN_TYPE_OBJECT_OPEN,
	TOKEN_TYPE_OBJECT_CLOSE,
	TOKEN_TYPE_COMMA,
	TOKEN_TYPE_COLON,
};

struct token {
	enum token_type type;
	char *str;
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
	if (nodes_size >= MAX_NODES) {
		JSON_ERROR(JSON_ERROR_TOO_MANY_NODES);
	}
	nodes[nodes_size++] = node;
}

static void push_field(struct json_field field) {
	if (fields_size >= MAX_FIELDS) {
		JSON_ERROR(JSON_ERROR_TOO_MANY_FIELDS);
	}
	fields[fields_size++] = field;
}

static char *push_string(char *slice_start, size_t length) {
	if (strings_size + length >= MAX_STRINGS_CHARACTERS) {
		JSON_ERROR(JSON_ERROR_TOO_MANY_STRINGS_CHARACTERS);
	}

	char *new_str = strings + strings_size;

	for (size_t i = 0; i < length; i++) {
		strings[strings_size++] = slice_start[i];
	}
	strings[strings_size++] = '\0';

	return new_str;
}

static struct json_node parse_object(size_t *i) {
	struct json_node node;

	node.type = JSON_NODE_OBJECT;
	(*i)++;

	recursion_depth++;
	if (recursion_depth > MAX_RECURSION_DEPTH) {
		JSON_ERROR(JSON_ERROR_MAX_RECURSION_DEPTH_EXCEEDED);
	}

	node.data.object.field_count = 0;

	struct json_field child_fields[MAX_CHILD_NODES];

	bool seen_key = false;
	bool seen_colon = false;
	bool seen_value = false;

	struct json_field field;

	struct json_node string;
	struct json_node array;
	struct json_node object;

	while (*i < tokens_size) {
		struct token *token = tokens + *i;

		switch (token->type) {
		case TOKEN_TYPE_STRING:
			if (!seen_key) {
				seen_key = true;
				field.key = token->str;
				(*i)++;
			} else if (seen_colon && !seen_value) {
				seen_value = true;
				string = parse_string(i);
				field.value = nodes + nodes_size;
				push_node(string);
				if (node.data.object.field_count >= MAX_CHILD_NODES) {
					JSON_ERROR(JSON_ERROR_TOO_MANY_CHILD_NODES);
				}
				child_fields[node.data.object.field_count++] = field;
			} else {
				JSON_ERROR(JSON_ERROR_UNEXPECTED_STRING);
			}
			break;
		case TOKEN_TYPE_ARRAY_OPEN:
			if (seen_colon && !seen_value) {
				seen_value = true;
				array = parse_array(i);
				field.value = nodes + nodes_size;
				push_node(array);
				if (node.data.object.field_count >= MAX_CHILD_NODES) {
					JSON_ERROR(JSON_ERROR_TOO_MANY_CHILD_NODES);
				}
				child_fields[node.data.object.field_count++] = field;
			} else {
				JSON_ERROR(JSON_ERROR_UNEXPECTED_ARRAY_OPEN);
			}
			break;
		case TOKEN_TYPE_ARRAY_CLOSE:
			JSON_ERROR(JSON_ERROR_UNEXPECTED_ARRAY_CLOSE);
		case TOKEN_TYPE_OBJECT_OPEN:
			if (seen_colon && !seen_value) {
				seen_value = true;
				object = parse_object(i);
				field.value = nodes + nodes_size;
				push_node(object);
				if (node.data.object.field_count >= MAX_CHILD_NODES) {
					JSON_ERROR(JSON_ERROR_TOO_MANY_CHILD_NODES);
				}
				child_fields[node.data.object.field_count++] = field;
			} else {
				JSON_ERROR(JSON_ERROR_UNEXPECTED_OBJECT_OPEN);
			}
			break;
		case TOKEN_TYPE_OBJECT_CLOSE:
			if (seen_key && !seen_colon) {
				JSON_ERROR(JSON_ERROR_EXPECTED_COLON);
			} else if (seen_colon && !seen_value) {
				JSON_ERROR(JSON_ERROR_EXPECTED_VALUE);
			}
			node.data.object.fields = fields + fields_size;
			for (size_t i = 0; i < node.data.object.field_count; i++) {
				push_field(child_fields[i]);
			}
			(*i)++;
			recursion_depth--;
			return node;
		case TOKEN_TYPE_COMMA:
			if (!seen_value) {
				JSON_ERROR(JSON_ERROR_UNEXPECTED_COMMA);
			}
			seen_key = false;
			seen_colon = false;
			seen_value = false;
			(*i)++;
			break;
		case TOKEN_TYPE_COLON:
			if (!seen_key) {
				JSON_ERROR(JSON_ERROR_UNEXPECTED_COLON);
			}
			seen_colon = true;
			(*i)++;
			break;
		}
	}

	JSON_ERROR(JSON_ERROR_EXPECTED_OBJECT_CLOSE);
}

static struct json_node parse_array(size_t *i) {
	struct json_node node;

	node.type = JSON_NODE_ARRAY;
	(*i)++;

	recursion_depth++;
	if (recursion_depth > MAX_RECURSION_DEPTH) {
		JSON_ERROR(JSON_ERROR_MAX_RECURSION_DEPTH_EXCEEDED);
	}

	node.data.array.value_count = 0;

	struct json_node child_nodes[MAX_CHILD_NODES];

	bool expecting_value = true;

	while (*i < tokens_size) {
		struct token *token = tokens + *i;

		switch (token->type) {
		case TOKEN_TYPE_STRING:
			if (!expecting_value) {
				JSON_ERROR(JSON_ERROR_UNEXPECTED_STRING);
			}
			expecting_value = false;
			if (node.data.array.value_count >= MAX_CHILD_NODES) {
				JSON_ERROR(JSON_ERROR_TOO_MANY_CHILD_NODES);
			}
			child_nodes[node.data.array.value_count++] = parse_string(i);
			break;
		case TOKEN_TYPE_ARRAY_OPEN:
			if (!expecting_value) {
				JSON_ERROR(JSON_ERROR_UNEXPECTED_ARRAY_OPEN);
			}
			expecting_value = false;
			if (node.data.array.value_count >= MAX_CHILD_NODES) {
				JSON_ERROR(JSON_ERROR_TOO_MANY_CHILD_NODES);
			}
			child_nodes[node.data.array.value_count++] = parse_array(i);
			break;
		case TOKEN_TYPE_ARRAY_CLOSE:
			node.data.array.values = nodes + nodes_size;
			for (size_t i = 0; i < node.data.array.value_count; i++) {
				push_node(child_nodes[i]);
			}
			(*i)++;
			recursion_depth--;
			return node;
		case TOKEN_TYPE_OBJECT_OPEN:
			if (!expecting_value) {
				JSON_ERROR(JSON_ERROR_UNEXPECTED_OBJECT_OPEN);
			}
			expecting_value = false;
			if (node.data.array.value_count >= MAX_CHILD_NODES) {
				JSON_ERROR(JSON_ERROR_TOO_MANY_CHILD_NODES);
			}
			child_nodes[node.data.array.value_count++] = parse_object(i);
			break;
		case TOKEN_TYPE_OBJECT_CLOSE:
			JSON_ERROR(JSON_ERROR_UNEXPECTED_OBJECT_CLOSE);
		case TOKEN_TYPE_COMMA:
			if (expecting_value) {
				JSON_ERROR(JSON_ERROR_UNEXPECTED_COMMA);
			}
			expecting_value = true;
			(*i)++;
			break;
		case TOKEN_TYPE_COLON:
			JSON_ERROR(JSON_ERROR_UNEXPECTED_COLON);
		}
	}

	JSON_ERROR(JSON_ERROR_EXPECTED_ARRAY_CLOSE);
}

static struct json_node parse_string(size_t *i) {
	struct json_node node;

	node.type = JSON_NODE_STRING;

	struct token *token = tokens + *i;
	node.data.string = token->str;

	(*i)++;

	return node;
}

static struct json_node parse(size_t *i) {
	struct token *t = tokens + *i;
	struct json_node node;

	switch (t->type) {
	case TOKEN_TYPE_STRING:
		node = parse_string(i);
		break;
	case TOKEN_TYPE_ARRAY_OPEN:
		node = parse_array(i);
		break;
	case TOKEN_TYPE_ARRAY_CLOSE:
		JSON_ERROR(JSON_ERROR_UNEXPECTED_ARRAY_CLOSE);
	case TOKEN_TYPE_OBJECT_OPEN:
		node = parse_object(i);
		break;
	case TOKEN_TYPE_OBJECT_CLOSE:
		JSON_ERROR(JSON_ERROR_UNEXPECTED_OBJECT_CLOSE);
	case TOKEN_TYPE_COMMA:
		JSON_ERROR(JSON_ERROR_UNEXPECTED_COMMA);
	case TOKEN_TYPE_COLON:
		JSON_ERROR(JSON_ERROR_UNEXPECTED_COLON);
	}

	if (*i < tokens_size) {
		JSON_ERROR(JSON_ERROR_UNEXPECTED_EXTRA_CHARACTER);
	}

	return node;
}

static void push_token(enum token_type type, size_t offset, size_t length) {
	if (tokens_size >= MAX_TOKENS) {
		JSON_ERROR(JSON_ERROR_TOO_MANY_TOKENS);
	}
	tokens[tokens_size++] = (struct token){
		.type = type,
		.str = push_string(text + offset, length),
	};
}

static void tokenize(void) {
	size_t i = 0;
	bool in_string = false;
	size_t string_start_index;

	while (i < text_size) {
		if (text[i] == '"') {
			if (in_string) {
				push_token(
					TOKEN_TYPE_STRING,
					string_start_index + 1,
					i - string_start_index - 1
				);
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
		} else if (text[i] == ',') {
			push_token(TOKEN_TYPE_COMMA, i, 1);
		} else if (text[i] == ':') {
			push_token(TOKEN_TYPE_COLON, i, 1);
		} else if (!isspace(text[i]) && !in_string) {
			JSON_ERROR(JSON_ERROR_UNRECOGNIZED_CHARACTER);
		}
		i++;
	}

	if (in_string) {
		JSON_ERROR(JSON_ERROR_UNCLOSED_STRING);
	}
}

static void read_text(char *json_file_path) {
	FILE *f = fopen(json_file_path, "r");
	if (!f) {
		JSON_ERROR(JSON_ERROR_FAILED_TO_OPEN_FILE);
	}

	text_size = fread(
		text,
		sizeof(char),
		MAX_CHARACTERS_IN_JSON_FILE,
		f
	);

	int is_eof = feof(f);
	int err = ferror(f);

    if (fclose(f)) {
		JSON_ERROR(JSON_ERROR_FAILED_TO_CLOSE_FILE);
    }

	if (text_size == 0) {
		JSON_ERROR(JSON_ERROR_FILE_EMPTY);
	}
	if (!is_eof || text_size == MAX_CHARACTERS_IN_JSON_FILE) {
		JSON_ERROR(JSON_ERROR_FILE_TOO_BIG);
	}
	if (err) {
		JSON_ERROR(JSON_ERROR_FILE_READING_ERROR);
	}

	text[text_size] = '\0';
}

static void reset(void) {
	json_error = JSON_NO_ERROR;
	recursion_depth = 0;
	text_size = 0;
	tokens_size = 0;
	nodes_size = 0;
	strings_size = 0;
	fields_size = 0;
}

bool json(char *json_file_path, struct json_node *returned) {
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
