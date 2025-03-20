# Tiny allocationless JSON parser in C

Parses a subset of [JSON](https://en.wikipedia.org/wiki/JSON) in roughly 600 lines of C, where only arrays, objects and strings are handled.

I wrote this JSON parser for my tiny programming language called [grug](https://mynameistrez.github.io/2024/02/29/creating-the-perfect-modding-language.html).

I was inspired by null program's [Minimalist C Libraries](https://nullprogram.com/blog/2018/06/10/) blog post, describing how C libraries never really need to allocate any memory themselves. The trick is to expect the user to pass `void *buffer` and `size_t buffer_capacity`:

```c
int main() {
    char buffer[420]

	// If json_init() fails, just increase the starting size
	assert(!json_init(buffer, sizeof(buffer)));

	struct json_node node;

	enum json_status status = json("foo.json", &node, buffer, sizeof(buffer));
	if (status) {
		// Handle error here
		exit(EXIT_FAILURE);
	}

	// You can now recursively walk the JSON data in the node variable here
}
```

Instead of using a fixed size buffer, you can use `realloc()` to keep retrying the call with a bigger buffer:

```c
int main() {
    size_t size = 420;
    void *buffer = malloc(size);

	// If json_init() fails, just increase the starting size
	assert(!json_init(buffer, size));

	struct json_node node;

	enum json_status status;
    do {
		status = json("foo.json", &node, buffer, size);
        if (status == JSON_OUT_OF_MEMORY) {
            size *= 2;
            buffer = realloc(buffer, size);
        }
    } while (status == JSON_OUT_OF_MEMORY);

	// You can now recursively walk the JSON data in the node variable here
}
```

It makes use of the array-based hash table I described in [this](https://mynameistrez.github.io/2024/06/19/array-based-hash-table-in-c.html) blog post, to detect duplicate object keys.

The [JSON spec](https://www.json.org/json-en.html) specifies that the other value types are `number`, `true`, `false` and `null`, but they can all be stored as strings. You could easily support these however by adding just a few dozen lines to `json.c`, so feel free to.

## Smaller version

Originally this was roughly 500 lines
* Every time `json_parse()` is called its arenas get reset, meaning that calling the function a second time overwrites the previous call's JSON result.

## Running the tests

```bash
gcc json.c tests.c && \
./a.out
```

and if you want to let the compiler do more checks:

```bash
gcc json.c tests.c -Wall -Wextra -Werror -Wpedantic -Wfatal-errors -g -fsanitize=address,undefined && \
./a.out
```

## Generating coverage

Make sure to install [gcovr](https://gcovr.com/en/stable/installation.html) first.

```bash
gcc json.c tests.c -Wall -Wextra -Werror -Wpedantic -Wfatal-errors -g -fsanitize=address,undefined --coverage && \
./a.out && \
gcovr --html-details coverage.html
```

You can then view the generated `coverage.html` in your browser. You should see that the program has nearly 100% line and branch coverage.

## Fuzzing

This uses [libFuzzer](https://llvm.org/docs/LibFuzzer.html), which requires [Clang](https://en.wikipedia.org/wiki/Clang) to be installed.

```bash
clang json.c fuzz.c -Wall -Wextra -Werror -Wpedantic -Wfatal-errors -Ofast -march=native -g -fsanitize=address,undefined,fuzzer && \
mkdir -p test_corpus && \
cp tests_err/* tests_ok/* test_corpus && \
mkdir -p corpus && \
./a.out -merge=1 corpus test_corpus && \
./a.out corpus
```

## Limitations

* Having the `\` character in a string does not allow escaping the `"` character.
