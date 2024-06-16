#include "json.h"

#include <assert.h>
#include <stdio.h>
#include <string.h>

#define OK_PARSE(path, node) {\
	assert(!json_parse(path, node) || fprintf(\
			stderr,\
			"json.c:%d: %s\n",\
			json_error_line_number,\
			json_error_messages[json_error]));\
}

#define ERROR_PARSE(path, error) {\
	assert(json_parse(path, NULL));\
	assert(json_error == error);\
}

static void ok_array_in_array(void) {
	struct json_node node;
	OK_PARSE("./tests_ok/array_in_array.json", &node);
	assert(node.type == JSON_NODE_ARRAY);
	assert(node.data.array.value_count == 1);
	assert(node.data.array.values[0].type == JSON_NODE_ARRAY);
	assert(node.data.array.values[0].data.array.value_count == 0);
}

static void ok_array_within_max_recursion_depth(void) {
	struct json_node node;
	OK_PARSE("./tests_ok/array_within_max_recursion_depth.json", &node);
	for (size_t i = 0; i < 41; i++) {
		assert(node.type == JSON_NODE_ARRAY);
		assert(node.data.array.value_count == 1);
		node = node.data.array.values[0];
	}
	assert(node.type == JSON_NODE_ARRAY);
	assert(node.data.array.value_count == 0);
}

static void ok_array(void) {
	struct json_node node;
	OK_PARSE("./tests_ok/array.json", &node);
	assert(node.type == JSON_NODE_ARRAY);
	assert(node.data.array.value_count == 0);
}

static void ok_grug(void) {
	struct json_node node;
	OK_PARSE("./tests_ok/grug.json", &node);

	struct json_object foo_fn;
	struct json_object bar_fn;
	struct json_array fn_array;
	struct json_array arguments;
	struct json_field *field;

	assert(node.type == JSON_NODE_ARRAY);
	fn_array = node.data.array;
	assert(fn_array.value_count == 2);

	// foo fn

	assert(fn_array.values[0].type == JSON_NODE_OBJECT);
	foo_fn = fn_array.values[0].data.object;
	assert(foo_fn.field_count == 4);

	field = foo_fn.fields;

	assert(strcmp(field->key, "name") == 0);
	assert(field->value->type == JSON_NODE_STRING);
	assert(strcmp(field->value->data.string, "foo") == 0);
	field++;

	assert(strcmp(field->key, "description") == 0);
	assert(field->value->type == JSON_NODE_STRING);
	assert(strcmp(field->value->data.string, "deez") == 0);
	field++;

	assert(strcmp(field->key, "return_type") == 0);
	assert(field->value->type == JSON_NODE_STRING);
	assert(strcmp(field->value->data.string, "i32") == 0);
	field++;

	assert(strcmp(field->key, "arguments") == 0);
	assert(field->value->type == JSON_NODE_ARRAY);
	arguments = field->value->data.array;
	assert(arguments.value_count == 2);

	assert(arguments.values[0].type == JSON_NODE_OBJECT);
	assert(arguments.values[0].data.object.field_count == 2);
	field = arguments.values[0].data.object.fields;

	assert(strcmp(field->key, "name") == 0);
	assert(field->value->type == JSON_NODE_STRING);
	assert(strcmp(field->value->data.string, "a") == 0);
	field++;

	assert(strcmp(field->key, "type") == 0);
	assert(field->value->type == JSON_NODE_STRING);
	assert(strcmp(field->value->data.string, "i64") == 0);
	field++;

	assert(arguments.values[1].type == JSON_NODE_OBJECT);
	assert(arguments.values[1].data.object.field_count == 2);
	field = arguments.values[1].data.object.fields;

	assert(strcmp(field->key, "name") == 0);
	assert(field->value->type == JSON_NODE_STRING);
	assert(strcmp(field->value->data.string, "b") == 0);
	field++;

	assert(strcmp(field->key, "type") == 0);
	assert(field->value->type == JSON_NODE_STRING);
	assert(strcmp(field->value->data.string, "i64") == 0);
	field++;

	// bar fn

	assert(fn_array.values[1].type == JSON_NODE_OBJECT);
	bar_fn = fn_array.values[1].data.object;
	assert(bar_fn.field_count == 4);

	field = bar_fn.fields;

	assert(strcmp(field->key, "name") == 0);
	assert(field->value->type == JSON_NODE_STRING);
	assert(strcmp(field->value->data.string, "bar") == 0);
	field++;

	assert(strcmp(field->key, "description") == 0);
	assert(field->value->type == JSON_NODE_STRING);
	assert(strcmp(field->value->data.string, "nuts") == 0);
	field++;

	assert(strcmp(field->key, "return_type") == 0);
	assert(field->value->type == JSON_NODE_STRING);
	assert(strcmp(field->value->data.string, "f32") == 0);
	field++;

	assert(strcmp(field->key, "arguments") == 0);
	assert(field->value->type == JSON_NODE_ARRAY);
	arguments = field->value->data.array;
	assert(arguments.value_count == 1);

	assert(arguments.values[0].type == JSON_NODE_OBJECT);
	assert(arguments.values[0].data.object.field_count == 2);
	field = arguments.values[0].data.object.fields;

	assert(strcmp(field->key, "name") == 0);
	assert(field->value->type == JSON_NODE_STRING);
	assert(strcmp(field->value->data.string, "x") == 0);
	field++;

	assert(strcmp(field->key, "type") == 0);
	assert(field->value->type == JSON_NODE_STRING);
	assert(strcmp(field->value->data.string, "i32") == 0);
	field++;
}

static void ok_object_foo(void) {
	struct json_node node;
	OK_PARSE("./tests_ok/object_foo.json", &node);
	assert(node.type == JSON_NODE_OBJECT);
	assert(node.data.object.field_count == 1);
}

static void ok_object_within_max_recursion_depth(void) {
	struct json_node node;
	OK_PARSE("./tests_ok/object_within_max_recursion_depth.json", &node);
	for (size_t i = 0; i < 41; i++) {
		assert(node.type == JSON_NODE_OBJECT);
		assert(node.data.object.field_count == 1);
		node = *node.data.object.fields[0].value;
	}
	assert(node.type == JSON_NODE_OBJECT);
	assert(node.data.object.field_count == 0);
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
	assert(strcmp(node.data.string, "foo") == 0);
}

static void ok_string(void) {
	struct json_node node;
	OK_PARSE("./tests_ok/string.json", &node);
	assert(node.type == JSON_NODE_STRING);
	assert(strcmp(node.data.string, "") == 0);
}

static void error_failed_to_open_file(void) {
	ERROR_PARSE("", JSON_ERROR_FAILED_TO_OPEN_FILE);
}

static void error_array_max_recursion_depth(void) {
	ERROR_PARSE("./tests_err/array_max_recursion_depth.json", JSON_ERROR_MAX_RECURSION_DEPTH);
}

static void error_array_unmatched_object_close(void) {
	ERROR_PARSE("./tests_err/array_unmatched_object_close.json", JSON_ERROR_UNMATCHED_OBJECT_CLOSE);
}

static void error_file_empty(void) {
	ERROR_PARSE("./tests_err/file_empty.json", JSON_ERROR_FILE_EMPTY);
}

static void error_object_max_recursion_depth(void) {
	ERROR_PARSE("./tests_err/object_max_recursion_depth.json", JSON_ERROR_MAX_RECURSION_DEPTH);
}

static void error_object_unmatched_array_close(void) {
	ERROR_PARSE("./tests_err/object_unmatched_array_close.json", JSON_ERROR_UNMATCHED_ARRAY_CLOSE);
}

static void error_root_unmatched_array_close(void) {
	ERROR_PARSE("./tests_err/root_unmatched_array_close.json", JSON_ERROR_UNMATCHED_ARRAY_CLOSE);
}

static void error_root_unmatched_object_close(void) {
	ERROR_PARSE("./tests_err/root_unmatched_object_close.json", JSON_ERROR_UNMATCHED_OBJECT_CLOSE);
}

static void error_unexpected_array_open_1(void) {
	ERROR_PARSE("./tests_err/unexpected_array_open_1.json", JSON_ERROR_UNEXPECTED_ARRAY_OPEN);
}

static void error_unexpected_array_open_2(void) {
	ERROR_PARSE("./tests_err/unexpected_array_open_2.json", JSON_ERROR_UNEXPECTED_ARRAY_OPEN);
}

static void error_unexpected_comma_array_1(void) {
	ERROR_PARSE("./tests_err/unexpected_comma_array_1.json", JSON_ERROR_UNEXPECTED_COMMA);
}

static void error_unexpected_comma_array_2(void) {
	ERROR_PARSE("./tests_err/unexpected_comma_array_2.json", JSON_ERROR_UNEXPECTED_COMMA);
}

static void error_unexpected_comma_object_1(void) {
	ERROR_PARSE("./tests_err/unexpected_comma_object_1.json", JSON_ERROR_UNEXPECTED_COMMA);
}

static void error_unexpected_comma_object_2(void) {
	ERROR_PARSE("./tests_err/unexpected_comma_object_2.json", JSON_ERROR_UNEXPECTED_COMMA);
}

static void error_unexpected_string_1(void) {
	ERROR_PARSE("./tests_err/unexpected_string_1.json", JSON_ERROR_UNEXPECTED_STRING);
}

static void error_unexpected_string_2(void) {
	ERROR_PARSE("./tests_err/unexpected_string_2.json", JSON_ERROR_UNEXPECTED_STRING);
}

static void error_unrecognized_character(void) {
	ERROR_PARSE("./tests_err/unrecognized_character.json", JSON_ERROR_UNRECOGNIZED_CHARACTER);
}

static void ok_tests(void) {
	ok_array_in_array();
	ok_array_within_max_recursion_depth();
	ok_array();
	ok_grug();
	ok_object_foo();
	ok_object_within_max_recursion_depth();
	ok_object();
	ok_string_foo();
	ok_string();
}

static void error_tests(void) {
	error_failed_to_open_file();

	error_array_max_recursion_depth();
	error_array_unmatched_object_close();
	error_file_empty();
	error_object_max_recursion_depth();
	error_object_unmatched_array_close();
	error_root_unmatched_array_close();
	error_root_unmatched_object_close();
	error_unexpected_array_open_1();
	error_unexpected_array_open_2();
	error_unexpected_comma_array_1();
	error_unexpected_comma_array_2();
	error_unexpected_comma_object_1();
	error_unexpected_comma_object_2();
	error_unexpected_string_1();
	error_unexpected_string_2();
	error_unrecognized_character();
}

int main(void) {
	error_tests();
	ok_tests();
}
