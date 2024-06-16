# Tiny JSON parser in C

Parses a subset of [JSON](https://en.wikipedia.org/wiki/JSON) in roughly 450 lines of C, where only arrays, objects and strings are handled.

The [JSON spec](https://www.json.org/json-en.html) specifies that the other value types are `number`, `true`, `false` and `null`, but they can all be stored as strings. You could easily support these however by adding just a few dozen lines to `json.c`, so feel free to.

The motivation for writing this program was that my tiny programming language called [grug](https://mynameistrez.github.io/2024/02/29/creating-the-perfect-modding-language.html) needs to be able to parse files that have the form of this `tests_ok/grug.json` test:

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

## Running the tests, and generating coverage

Make sure to install [gcovr](https://gcovr.com/en/stable/installation.html) first.

```bash
clear && \
gcc json.c tests.c -Wall -Wextra -Werror -Wpedantic -Wfatal-errors -g --coverage && \
./a.out && \
gcovr --html-details coverage.html
```

You can then view the generated `coverage.html` in your browser. You should see that the program has nearly 100% line and branch coverage.

## Fuzzing

Uses [libFuzzer](https://llvm.org/docs/LibFuzzer.html), which requires [Clang](https://en.wikipedia.org/wiki/Clang) to be installed.

```bash
clear && \
clang json.c fuzz.c -Wall -Wextra -Werror -Wpedantic -Wfatal-errors -Ofast -march=native -g -fsanitize=address,undefined,fuzzer && \
mkdir -p test_corpus && \
cp tests_err/* tests_ok/* test_corpus && \
mkdir -p corpus && \
./a.out -merge=1 corpus test_corpus && \
./a.out corpus
```

## Limitations

* Having the `\` character in a string does not allow escaping the `"` character.

* Every time `json_parse()` is called its arenas get reset, meaning that calling the function a second time overwrites the previous call's JSON result.
