#pragma once

#include <stdbool.h>
#include <stddef.h>

struct json_string {
	char *str;
};

struct json_array {
	struct json_node *values;
	size_t value_count;
};

struct json_object {
	struct json_field *fields;
	size_t field_count;
};

struct json_field {
	char *key;
	struct json_node *value;
};

struct json_node {
	enum {
		JSON_NODE_STRING,
		JSON_NODE_ARRAY,
		JSON_NODE_OBJECT,
	} type;
	union {
		struct json_string string;
		struct json_array array;
		struct json_object object;
	} data;
};

enum json_error {
	JSON_NO_ERROR,
	JSON_ERROR_FAILED_TO_OPEN_FILE,
	JSON_ERROR_FAILED_TO_CLOSE_FILE,
	JSON_ERROR_FILE_IS_EMPTY,
	JSON_ERROR_FILE_TOO_BIG,
	JSON_ERROR_FILE_READING_ERROR,
	JSON_ERROR_TOO_MANY_TOKENS,
	JSON_ERROR_UNRECOGNIZED_CHARACTER,
	JSON_ERROR_TOO_MANY_NODES,
	JSON_ERROR_TOO_MANY_FIELDS,
	JSON_ERROR_TOO_MANY_STRINGS_CHARACTERS,
	JSON_ERROR_UNMATCHED_ARRAY_CLOSE,
	JSON_ERROR_UNMATCHED_OBJECT_CLOSE,
	JSON_ERROR_MAX_RECURSION_DEPTH,
	JSON_ERROR_OBJECT_FIELD,
	JSON_ERROR_NOT_EXPECTING_VALUE,
	JSON_ERROR_UNEXPECTED_COMMA,
	JSON_ERROR_UNEXPECTED_COLON,
};

// Use to figure out what type of error occurred
extern enum json_error json_error;
extern int json_error_line_number;
extern char *json_error_messages[];

// Returns whether there was an error
bool json_parse(char *json_file_path, struct json_node *returned);
