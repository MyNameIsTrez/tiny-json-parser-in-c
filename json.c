#include "json.h"

#include <stdio.h>

#define MAX_JSON_NODES 420420

union json_node json_nodes[MAX_JSON_NODES];

enum json_error json_error;

bool json_parse(char *json_file_path, union json_node *returned) {
	json_error = JSON_NO_ERROR;

	FILE *f = fopen(json_file_path, "r");
	if (!f) {
		json_error = JSON_ERROR_EMPTY_FILE;
		return false;
	}

	(void)json_file_path;
	(void)returned;
	return true;
}
