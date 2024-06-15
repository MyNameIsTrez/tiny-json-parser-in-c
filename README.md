# Tiny JSON Parser in C

Parses a subset of [JSON](https://en.wikipedia.org/wiki/JSON) in roughly 420 lines of C, where only arrays, objects and strings are handled.

The [JSON spec](https://www.json.org/json-en.html) specifies that the other value types are `number`, `true`, `false` and `null`, but since they can all be stored as a string, I don't consider them important enough for this tiny JSON parser.

The motivation for this program was that my [grug](https://mynameistrez.github.io/2024/02/29/creating-the-perfect-modding-language.html) programming language needs to be able to parse files that have the form of this `tests_ok/grug.json` test:

```json
[
	{
		"name": "foo",
		"description": "deez",
		"return_type": "i32",
		"arguments": [
			{
				"name": "a",
				"type": "i64"
			},
			{
				"name": "b",
				"type": "i64"
			}
		]
	},
	{
		"name": "bar",
		"description": "nuts",
		"return_type": "f32",
		"arguments": [
			{
				"name": "x",
				"type": "i32"
			}
		]
	}
]
```

## Running the tests

```bash
clear && \
gcc tests.c json.c -Wall -Wextra -Werror -Wpedantic -Wfatal-errors -g && \
./a.out
```

## Limitations

* Having the `\` character in a string does not allow escaping the `"` character.

* Every time `json_parse()` is called its arenas get reset, meaning that calling the function a second time overwrites the previous call's JSON result.
