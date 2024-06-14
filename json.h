#pragma once

#include <stdbool.h>
#include <stddef.h>

struct json_string {
	size_t i;
};

struct json_array {
	size_t i;
};

struct json_object {
	size_t i;
};

struct json_node {
	union {
		struct json_string s;
		struct json_array a;
		struct json_object o;
	} data;
	enum {
		JSON_NODE_STRING,
		JSON_NODE_ARRAY,
		JSON_NODE_OBJECT,
	} type;
};

extern struct json_node json_nodes[];

enum json_error {
	JSON_NO_ERROR,
	JSON_ERROR_FAILED_TO_OPEN_JSON_FILE,
	JSON_ERROR_FAILED_TO_CLOSE_JSON_FILE,
	JSON_ERROR_JSON_FILE_IS_EMPTY,
	JSON_ERROR_JSON_FILE_TOO_BIG,
	JSON_ERROR_JSON_FILE_READING_ERROR,
	JSON_ERROR_TOO_MANY_TOKENS,
};

// Use to figure out what type of error occurred
extern enum json_error json_error;

// Returns whether there was an error
bool json_parse(char *json_file_path, struct json_node *returned);
