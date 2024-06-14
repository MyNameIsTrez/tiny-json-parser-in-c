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

union json_node {
	struct json_string s;
	struct json_array a;
	struct json_object o;
};

extern union json_node json_nodes[];

enum json_error {
	JSON_NO_ERROR,
	JSON_ERROR_EMPTY_FILE,
};

// Use to figure out what type of error occurred
extern enum json_error json_error;

// Returns whether parsing succeeded
bool json_parse(char *json_file_path, union json_node *returned);
