# Tiny C JSON Parser

Only arrays, objects and strings are parseable, since those are the only types that my [grug](https://mynameistrez.github.io/2024/02/29/creating-the-perfect-modding-language.html) programming language uses.

Every time `json_parse()` is called, its arena gets reset, meaning that calling the function a second time overwrites the previous call's JSON result.

Run the tests with  this:
```bash
clear && \
gcc tests.c json.c -Wall -Wextra -Werror -Wpedantic -Wfatal-errors -g && \
./a.out
```
