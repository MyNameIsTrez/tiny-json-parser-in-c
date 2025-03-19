#include "json.h"

#include <ctype.h>
#include <setjmp.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>

#define MAX_CHILD_NODES 420
#define MAX_RECURSION_DEPTH 42

#define json_error(error) {\
	json_error_line_number = __LINE__;\
	longjmp(error_jmp_buffer, error);\
}

#define json_assert(condition, error) {\
	if (!(condition)) {\
		json_error(error);\
	}\
}

static jmp_buf error_jmp_buffer;

int json_error_line_number;

char *json_error_messages[] = {
	[JSON_OK] = "No error",
	[JSON_ERROR_FAILED_TO_OPEN_FILE] = "Failed to open file",
	[JSON_ERROR_FAILED_TO_CLOSE_FILE] = "Failed to close file",
	[JSON_ERROR_FILE_EMPTY] = "File is empty",
	[JSON_ERROR_FILE_TOO_BIG] = "File is too big",
	[JSON_ERROR_FILE_READING_ERROR] = "File reading error",
	[JSON_ERROR_UNRECOGNIZED_CHARACTER] = "Unrecognized character",
	[JSON_ERROR_UNCLOSED_STRING] = "Unclosed string",
	[JSON_ERROR_DUPLICATE_KEY] = "Duplicate key",
	[JSON_ERROR_TOO_MANY_CHILD_NODES] = "Too many child nodes",
	[JSON_ERROR_MAX_RECURSION_DEPTH_EXCEEDED] = "Max recursion depth exceeded",
	[JSON_ERROR_TRAILING_COMMA] = "Trailing comma",
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

struct {
	bool initialized;

	char *text;
	size_t text_size;
	size_t text_used;

	struct token *tokens;
	size_t tokens_size;
	size_t tokens_used;

	struct json_node *nodes;
	size_t nodes_size;
	size_t nodes_used;

	char *strings;
	size_t strings_size;
	size_t strings_used;

	struct json_field *fields;
	uint32_t *fields_buckets;
	uint32_t *fields_chains;
	size_t fields_size;
	size_t fields_used;
} *g;

static size_t recursion_depth;

static struct json_node parse_string(size_t *i);
static struct json_node parse_array(size_t *i);

static void push_node(struct json_node node) {
	if (g->nodes_used + 1 > g->nodes_size) {
		g->nodes_size *= 2;
		json_error(JSON_ERROR_OUT_OF_MEMORY);
	}

	g->nodes[g->nodes_used++] = node;
}

static void push_field(struct json_field field) {
	if (g->fields_used + 1 > g->fields_size) {
		g->fields_size *= 2;
		json_error(JSON_ERROR_OUT_OF_MEMORY);
	}

	g->fields[g->fields_used++] = field;
}

static char *push_string(char *slice_start, size_t length) {
	if (g->strings_used + 1 > g->strings_size) {
		g->strings_size *= 2;
		json_error(JSON_ERROR_OUT_OF_MEMORY);
	}

	char *new_str = g->strings + g->strings_used;

	for (size_t i = 0; i < length; i++) {
		g->strings[g->strings_used++] = slice_start[i];
	}
	g->strings[g->strings_used++] = '\0';

	return new_str;
}

// From https://sourceware.org/git/?p=binutils-gdb.git;a=blob;f=bfd/elf.c#l193
static uint32_t elf_hash(const char *namearg) {
	uint32_t h = 0;

	for (const unsigned char *name = (const unsigned char *) namearg; *name; name++) {
		h = (h << 4) + *name;
		h ^= (h >> 24) & 0xf0;
	}

	return h & 0x0fffffff;
}

static bool is_duplicate_key(struct json_field *child_fields, size_t field_count, char *key) {
	uint32_t i = g->fields_buckets[elf_hash(key) % field_count];

	while (1) {
		if (i == UINT32_MAX) {
			return false;
		}

		if (strcmp(key, child_fields[i].key) == 0) {
			break;
		}

		i = g->fields_chains[i];
	}

	return true;
}

static void check_duplicate_keys(struct json_field *child_fields, size_t field_count) {
	memset(g->fields_buckets, 0xff, field_count * sizeof(uint32_t));

	size_t chains_used = 0;

	for (size_t i = 0; i < field_count; i++) {
		char *key = child_fields[i].key;

		json_assert(!is_duplicate_key(child_fields, field_count, key), JSON_ERROR_DUPLICATE_KEY);

		uint32_t bucket_index = elf_hash(key) % field_count;

		g->fields_chains[chains_used++] = g->fields_buckets[bucket_index];

		g->fields_buckets[bucket_index] = i;
	}
}

static struct json_node parse_object(size_t *i) {
	struct json_node node;

	node.type = JSON_NODE_OBJECT;
	(*i)++;

	recursion_depth++;
	json_assert(recursion_depth <= MAX_RECURSION_DEPTH, JSON_ERROR_MAX_RECURSION_DEPTH_EXCEEDED);

	node.object.field_count = 0;

	struct json_field child_fields[MAX_CHILD_NODES];

	bool seen_key = false;
	bool seen_colon = false;
	bool seen_value = false;
	bool seen_comma = false;

	struct json_field field;

	struct json_node string;
	struct json_node array;
	struct json_node object;

	while (*i < g->tokens_used) {
		struct token *token = g->tokens + *i;

		switch (token->type) {
		case TOKEN_TYPE_STRING:
			if (!seen_key) {
				seen_key = true;
				field.key = token->str;
				(*i)++;
			} else if (seen_colon && !seen_value) {
				seen_value = true;
				seen_comma = false;
				string = parse_string(i);
				field.value = g->nodes + g->nodes_used;
				push_node(string);
				json_assert(node.object.field_count < MAX_CHILD_NODES, JSON_ERROR_TOO_MANY_CHILD_NODES);
				child_fields[node.object.field_count++] = field;
			} else {
				json_error(JSON_ERROR_UNEXPECTED_STRING);
			}
			break;
		case TOKEN_TYPE_ARRAY_OPEN:
			if (seen_colon && !seen_value) {
				seen_value = true;
				seen_comma = false;
				array = parse_array(i);
				field.value = g->nodes + g->nodes_used;
				push_node(array);
				json_assert(node.object.field_count < MAX_CHILD_NODES, JSON_ERROR_TOO_MANY_CHILD_NODES);
				child_fields[node.object.field_count++] = field;
			} else {
				json_error(JSON_ERROR_UNEXPECTED_ARRAY_OPEN);
			}
			break;
		case TOKEN_TYPE_ARRAY_CLOSE:
			json_error(JSON_ERROR_UNEXPECTED_ARRAY_CLOSE);
		case TOKEN_TYPE_OBJECT_OPEN:
			if (seen_colon && !seen_value) {
				seen_value = true;
				seen_comma = false;
				object = parse_object(i);
				field.value = g->nodes + g->nodes_used;
				push_node(object);
				json_assert(node.object.field_count < MAX_CHILD_NODES, JSON_ERROR_TOO_MANY_CHILD_NODES);
				child_fields[node.object.field_count++] = field;
			} else {
				json_error(JSON_ERROR_UNEXPECTED_OBJECT_OPEN);
			}
			break;
		case TOKEN_TYPE_OBJECT_CLOSE:
			if (seen_key && !seen_colon) {
				json_error(JSON_ERROR_EXPECTED_COLON);
			} else if (seen_colon && !seen_value) {
				json_error(JSON_ERROR_EXPECTED_VALUE);
			} else if (seen_comma) {
				json_error(JSON_ERROR_TRAILING_COMMA);
			}
			check_duplicate_keys(child_fields, node.object.field_count);
			node.object.fields = g->fields + g->fields_used;
			for (size_t field_index = 0; field_index < node.object.field_count; field_index++) {
				push_field(child_fields[field_index]);
			}
			(*i)++;
			recursion_depth--;
			return node;
		case TOKEN_TYPE_COMMA:
			json_assert(seen_value, JSON_ERROR_UNEXPECTED_COMMA);
			seen_key = false;
			seen_colon = false;
			seen_value = false;
			seen_comma = true;
			(*i)++;
			break;
		case TOKEN_TYPE_COLON:
			json_assert(seen_key, JSON_ERROR_UNEXPECTED_COLON);
			seen_colon = true;
			(*i)++;
			break;
		}
	}

	json_error(JSON_ERROR_EXPECTED_OBJECT_CLOSE);
}

static struct json_node parse_array(size_t *i) {
	struct json_node node;

	node.type = JSON_NODE_ARRAY;
	(*i)++;

	recursion_depth++;
	json_assert(recursion_depth <= MAX_RECURSION_DEPTH, JSON_ERROR_MAX_RECURSION_DEPTH_EXCEEDED);

	node.array.value_count = 0;

	struct json_node child_nodes[MAX_CHILD_NODES];

	bool seen_value = false;
	bool seen_comma = false;

	while (*i < g->tokens_used) {
		struct token *token = g->tokens + *i;

		switch (token->type) {
		case TOKEN_TYPE_STRING:
			json_assert(!seen_value, JSON_ERROR_UNEXPECTED_STRING);
			seen_value = true;
			seen_comma = false;
			json_assert(node.array.value_count < MAX_CHILD_NODES, JSON_ERROR_TOO_MANY_CHILD_NODES);
			child_nodes[node.array.value_count++] = parse_string(i);
			break;
		case TOKEN_TYPE_ARRAY_OPEN:
			json_assert(!seen_value, JSON_ERROR_UNEXPECTED_ARRAY_OPEN);
			seen_value = true;
			seen_comma = false;
			json_assert(node.array.value_count < MAX_CHILD_NODES, JSON_ERROR_TOO_MANY_CHILD_NODES);
			child_nodes[node.array.value_count++] = parse_array(i);
			break;
		case TOKEN_TYPE_ARRAY_CLOSE:
			json_assert(!seen_comma, JSON_ERROR_TRAILING_COMMA);
			node.array.values = g->nodes + g->nodes_used;
			for (size_t value_index = 0; value_index < node.array.value_count; value_index++) {
				push_node(child_nodes[value_index]);
			}
			(*i)++;
			recursion_depth--;
			return node;
		case TOKEN_TYPE_OBJECT_OPEN:
			json_assert(!seen_value, JSON_ERROR_UNEXPECTED_OBJECT_OPEN);
			seen_value = true;
			seen_comma = false;
			json_assert(node.array.value_count < MAX_CHILD_NODES, JSON_ERROR_TOO_MANY_CHILD_NODES);
			child_nodes[node.array.value_count++] = parse_object(i);
			break;
		case TOKEN_TYPE_OBJECT_CLOSE:
			json_error(JSON_ERROR_UNEXPECTED_OBJECT_CLOSE);
		case TOKEN_TYPE_COMMA:
			json_assert(seen_value, JSON_ERROR_UNEXPECTED_COMMA);
			seen_value = false;
			seen_comma = true;
			(*i)++;
			break;
		case TOKEN_TYPE_COLON:
			json_error(JSON_ERROR_UNEXPECTED_COLON);
		}
	}

	json_error(JSON_ERROR_EXPECTED_ARRAY_CLOSE);
}

static struct json_node parse_string(size_t *i) {
	struct json_node node;

	node.type = JSON_NODE_STRING;

	struct token *token = g->tokens + *i;
	node.string = token->str;

	(*i)++;

	return node;
}

static struct json_node parse(size_t *i) {
	struct token *t = g->tokens + *i;
	struct json_node node;

	switch (t->type) {
	case TOKEN_TYPE_STRING:
		node = parse_string(i);
		break;
	case TOKEN_TYPE_ARRAY_OPEN:
		node = parse_array(i);
		break;
	case TOKEN_TYPE_ARRAY_CLOSE:
		json_error(JSON_ERROR_UNEXPECTED_ARRAY_CLOSE);
	case TOKEN_TYPE_OBJECT_OPEN:
		node = parse_object(i);
		break;
	case TOKEN_TYPE_OBJECT_CLOSE:
		json_error(JSON_ERROR_UNEXPECTED_OBJECT_CLOSE);
	case TOKEN_TYPE_COMMA:
		json_error(JSON_ERROR_UNEXPECTED_COMMA);
	case TOKEN_TYPE_COLON:
		json_error(JSON_ERROR_UNEXPECTED_COLON);
	}

	json_assert(*i >= g->tokens_used, JSON_ERROR_UNEXPECTED_EXTRA_CHARACTER);

	return node;
}

static void push_token(enum token_type type, size_t offset, size_t length) {
	if (g->tokens_used + 1 > g->tokens_size) {
		g->tokens_size *= 2;
		json_error(JSON_ERROR_OUT_OF_MEMORY);
	}

	g->tokens[g->tokens_used++] = (struct token){
		.type = type,
		.str = push_string(g->text + offset, length),
	};
}

static void tokenize(void) {
	size_t i = 0;

	while (i < g->text_used) {
		if (g->text[i] == '"') {
			size_t string_start_index = i;

			while (++i < g->text_used && g->text[i] != '"') {}

			json_assert(g->text[i] == '"', JSON_ERROR_UNCLOSED_STRING);

			push_token(
				TOKEN_TYPE_STRING,
				string_start_index + 1,
				i - string_start_index - 1
			);
		} else if (g->text[i] == '[') {
			push_token(TOKEN_TYPE_ARRAY_OPEN, i, 1);
		} else if (g->text[i] == ']') {
			push_token(TOKEN_TYPE_ARRAY_CLOSE, i, 1);
		} else if (g->text[i] == '{') {
			push_token(TOKEN_TYPE_OBJECT_OPEN, i, 1);
		} else if (g->text[i] == '}') {
			push_token(TOKEN_TYPE_OBJECT_CLOSE, i, 1);
		} else if (g->text[i] == ',') {
			push_token(TOKEN_TYPE_COMMA, i, 1);
		} else if (g->text[i] == ':') {
			push_token(TOKEN_TYPE_COLON, i, 1);
		} else if (!isspace(g->text[i])) {
			json_error(JSON_ERROR_UNRECOGNIZED_CHARACTER);
		}
		i++;
	}
}

static void read_text(char *json_file_path) {
	FILE *f = fopen(json_file_path, "r");
	json_assert(f, JSON_ERROR_FAILED_TO_OPEN_FILE);

	g->text_used = fread(
		g->text,
		sizeof(char),
		g->text_size, // TODO: I may need to subtract 1, to prevent an off-by-one error? Add a test for this
		f
	);

	int is_eof = feof(f);
	int err = ferror(f);

	json_assert(fclose(f) == 0, JSON_ERROR_FAILED_TO_CLOSE_FILE);

	json_assert(g->text_used != 0, JSON_ERROR_FILE_EMPTY);

	if (!is_eof || g->text_used == g->text_size) {
		g->text_size *= 2;
		json_error(JSON_ERROR_OUT_OF_MEMORY);
	}

	json_assert(err == 0, JSON_ERROR_FILE_READING_ERROR);

	g->text[g->text_used] = '\0'; // TODO: I may need to subtract 1, to prevent an off-by-one error? Add a test for this
}

static void check_buffer_used(size_t buffer_used, size_t buffer_size) {
    if (buffer_used > buffer_size) {
        json_error(JSON_ERROR_OUT_OF_MEMORY);
    }
}

static void *get_next_aligned_area(void **buffer, size_t *buffer_used) {
    // Align buffer_used to 16 bytes
    *buffer_used += (16 - (*buffer_used % 16)) % 16;

    *buffer = (char *)*buffer + *buffer_used;
    return *buffer;
}

static void allocate_arrays(void *buffer, size_t size) {
	g->text_used = 0;
	g->tokens_used = 0;
	g->nodes_used = 0;
	g->strings_used = 0;
	g->fields_used = 0;

	size_t used = 0;

	// Reserve space for the struct itself in the buffer
	used += sizeof(*g);
    check_buffer_used(used, size);

	g->text = get_next_aligned_area(&buffer, &used);
	used += g->text_size * sizeof(*g->text);
    check_buffer_used(used, size);

	g->tokens = get_next_aligned_area(&buffer, &used);
	used += g->tokens_size * sizeof(*g->tokens);
    check_buffer_used(used, size);

	g->nodes = get_next_aligned_area(&buffer, &used);
	used += g->nodes_size * sizeof(*g->nodes);
    check_buffer_used(used, size);

	g->strings = get_next_aligned_area(&buffer, &used);
	used += g->strings_size * sizeof(*g->strings);
    check_buffer_used(used, size);

	g->fields = get_next_aligned_area(&buffer, &used);
	used += g->fields_size * sizeof(*g->fields);
    check_buffer_used(used, size);

	g->fields_buckets = get_next_aligned_area(&buffer, &used);
	used += g->fields_size * sizeof(*g->fields_buckets);
    check_buffer_used(used, size);

	g->fields_chains = get_next_aligned_area(&buffer, &used);
	used += g->fields_size * sizeof(*g->fields_chains);
    check_buffer_used(used, size);
}

enum json_status json(char *json_file_path, struct json_node *returned, void *buffer, size_t buffer_size) {
	enum json_status status = setjmp(error_jmp_buffer);
	if (status) {
		return status;
	}

	g = buffer;

	if (!g->initialized) {
		g->initialized = true;

		g->text_size = 1;
		g->tokens_size = 1;
		g->nodes_size = 1;
		g->strings_size = 1;
		g->fields_size = 1;
	}

	allocate_arrays(buffer, buffer_size);

	read_text(json_file_path);

	tokenize();

	recursion_depth = 0;

	size_t token_index = 0;
	*returned = parse(&token_index);

	return JSON_OK;
}
