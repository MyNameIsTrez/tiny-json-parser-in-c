#include "json.h"

#include <assert.h>
#include <stdio.h>
#include <string.h>

static void ok_array(void) {
	struct json_node node;
	assert(!json_parse("./tests_ok/array.json", &node));
	assert(node.type == JSON_NODE_ARRAY);
	assert(node.data.array.node_count == 0);
}

static void ok_object(void) {
	struct json_node node;
	assert(!json_parse("./tests_ok/object.json", &node));
	assert(node.type == JSON_NODE_OBJECT);
	assert(node.data.object.field_count == 0);
}

static void ok_string_foo(void) {
	struct json_node node;
	assert(!json_parse("./tests_ok/string_foo.json", &node) || fprintf(stderr, "%d\n", json_error_line_number));
	assert(node.type == JSON_NODE_STRING);
	assert(strcmp(node.data.string.str, "\"foo\"") == 0);
}

static void ok_string(void) {
	struct json_node node;
	assert(!json_parse("./tests_ok/string.json", &node));
	assert(node.type == JSON_NODE_STRING);
	assert(strcmp(node.data.string.str, "\"\"") == 0);
}

static void error_json_file_is_empty(void) {
	assert(json_parse("./tests_err/empty.json", NULL) && json_error == JSON_ERROR_JSON_FILE_IS_EMPTY);
}

static void error_failed_to_open_json_file(void) {
	assert(json_parse("", NULL) && json_error == JSON_ERROR_FAILED_TO_OPEN_JSON_FILE);
}

static void ok_tests(void) {
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
