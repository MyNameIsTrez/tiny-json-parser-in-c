#include "json.h"

#include <assert.h>

static void ok_string() {
	struct json_node node;
	assert(json_parse("./tests_ok/string.json", &node));
}

static void ok_array() {
	
}

static void ok_object() {
	
}

static void error_json_file_is_empty() {
	assert(!json_parse("./tests_err/empty.json", NULL) && json_error == JSON_ERROR_JSON_FILE_IS_EMPTY);
}

static void error_failed_to_open_json_file() {
	assert(!json_parse("", NULL) && json_error == JSON_ERROR_FAILED_TO_OPEN_JSON_FILE);
}

static void ok_tests() {
	ok_string();
	ok_array();
	ok_object();
}

static void error_tests() {
	error_failed_to_open_json_file();
	error_json_file_is_empty();
}

int main() {
	error_tests();
	ok_tests();
}
