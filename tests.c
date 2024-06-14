#include "json.h"

#include <assert.h>

static void ok_string() {
	
}

static void ok_array() {
	
}

static void ok_object() {
	
}

static void error_empty() {
	assert(!json_parse("", NULL) && json_error == JSON_ERROR_EMPTY_FILE);
}

static void ok_tests() {
	ok_string();
	ok_array();
	ok_object();
}

static void error_tests() {
	error_empty();
}

int main() {
	error_tests();
	ok_tests();
}
