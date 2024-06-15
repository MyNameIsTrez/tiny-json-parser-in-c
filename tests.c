#include "json.h"

#include <assert.h>
#include <stdio.h>
#include <string.h>

#define OK_PARSE(path, node) {\
	assert(!json_parse(path, node) || fprintf(\
			stderr,\
			"At line %d in json.c, the error '%s' was thrown\n",\
			json_error_line_number,\
			json_error_messages[json_error]));\
}

#define ERROR_PARSE(path, error) {\
	assert(json_parse(path, NULL) && json_error == error);\
}

static void ok_array_in_array(void) {
	struct json_node node;
	OK_PARSE("./tests_ok/array_in_array.json", &node);
	assert(node.type == JSON_NODE_ARRAY);
	assert(node.data.array.value_count == 1);
	assert(node.data.array.values[0].type == JSON_NODE_ARRAY);
	assert(node.data.array.values[0].data.array.value_count == 0);
}

static void ok_array(void) {
	struct json_node node;
	OK_PARSE("./tests_ok/array.json", &node);
	assert(node.type == JSON_NODE_ARRAY);
	assert(node.data.array.value_count == 0);
}

static void ok_object(void) {
	struct json_node node;
	OK_PARSE("./tests_ok/object.json", &node);
	assert(node.type == JSON_NODE_OBJECT);
	assert(node.data.object.field_count == 0);
}

static void ok_string_foo(void) {
	struct json_node node;
	OK_PARSE("./tests_ok/string_foo.json", &node);
	assert(node.type == JSON_NODE_STRING);
	assert(strcmp(node.data.string.str, "\"foo\"") == 0);
}

static void ok_string(void) {
	struct json_node node;
	OK_PARSE("./tests_ok/string.json", &node);
	assert(node.type == JSON_NODE_STRING);
	assert(strcmp(node.data.string.str, "\"\"") == 0);
}

static void error_json_file_is_empty(void) {
	ERROR_PARSE("./tests_err/empty.json", JSON_ERROR_JSON_FILE_IS_EMPTY);
}

static void error_failed_to_open_json_file(void) {
	ERROR_PARSE("", JSON_ERROR_FAILED_TO_OPEN_JSON_FILE);
}

static void ok_tests(void) {
	ok_array_in_array();
	ok_array();
	ok_object();
	ok_string_foo();
	ok_string();
}

static void error_tests(void) {
	error_failed_to_open_json_file();
	error_json_file_is_empty();
}

int main(void) {
	error_tests();
	ok_tests();
}
