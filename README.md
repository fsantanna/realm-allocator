# realm

[![Tests][badge]][test]

[badge]: https://github.com/fsantanna/realm-allocator/actions/workflows/test.yml/badge.svg
[test]:  https://github.com/fsantanna/realm-allocator/actions/workflows/test.yml

`Realm` is a stack-based resource allocator that groups keyed resources by
depth, enabling bulk-free when a scope ends.

- A key is a plain memory buffer (owned by the library).
- A value is an opaque pointer to a payload (owned by the application).

Two keys match when they have the exact same memory bytes.

Resources are managed through a depth stack:
`realm_enter` pushes a new scope, `realm_leave` pops it and frees all
entries created at that depth.
Three put modes control collision behavior: exclusive (`'!'`), shared
(`'='`), and replaceable (`'~'`).

Notes about `Realm`:

- implemented as a single-header library
- uses the simple [DJB2][djb2] hashing algorithm
- it is not thread safe

[djb2]: https://en.wikipedia.org/wiki/List_of_hash_functions

# Guide

## General Structure

```c
#define REALM_C
#include "realm.h"

int main (void) {
    realm_t* r = realm_open(16);
    realm_enter(r);
    {
        realm_put(r, '!', 3, "key", NULL, NULL, val);
        void* v = realm_get(r, 3, "key");
    }
    realm_leave(r);
    realm_close(r);
}
```

## Example

See [hello.c](hello.c) for a minimal example.

```bash
gcc -o hello.exe hello.c && ./hello.exe
# or
make hello
```

## Tests

```bash
make tests  # runs tests with valgrind
```

# API

## Types

- `typedef void (*realm_free_t) (int n, const void* key, void* value)`
    - Cleanup callback, called whenever an entry is evicted.
    - Parameters:
        - `n: int` | key length
        - `key: void*` | key buffer
        - `value: void*` | value to free

- `typedef void* (*realm_alloc_t) (int n, const void* key, void* ctx)`
    - Factory callback for lazy value creation.
    - Parameters:
        - `n: int` | key length
        - `key: void*` | key buffer
        - `ctx: void*` | caller-provided context
    - Return:
        - `void*` | newly created value

## Collision Modes

```c
#define REALM_EXCLUSIVE   '!'   // assert if key exists
#define REALM_SHARED      '='   // return existing on hit
#define REALM_REPLACEABLE '~'   // free old + store new
```

### Put Behavior

| Sym | Mode        | key exists            | key not exists    |
|-----|-------------|-----------------------|-------------------|
| `!` | Exclusive   | assert (bug)          | create            |
| `=` | Shared      | return existing       | create via alloc  |
| `~` | Replaceable | free old + create new | create            |

When `alloc!=NULL`, `ctx` serves as context and `alloc(n, key, ctx)` creates
the resource. When `alloc==NULL`, `ctx` is the resource directly.
Mode `'='` requires `alloc!=NULL`.

The `free` callback is stored per-entry and called on eviction (`realm_leave`,
`realm_close`, or `'~'` replacement).
`free` may be `NULL` (no cleanup needed).

## Functions

- `realm_t* realm_open (int n)`
    - Creates a realm allocator with `n` hash entries.
    - Parameters:
        - `n: int` | number of hash entries
    - Return:
        - `realm_t*` | pointer to the new allocator

- `void realm_close (realm_t* r)`
    - Destroys the allocator, freeing all entries at all depths.
        Calls the per-entry free callback for each entry.
    - Parameters:
        - `r: realm_t*` | allocator to close

- `void realm_enter (realm_t* r)`
    - Pushes a new scope onto the depth stack.
    - Parameters:
        - `r: realm_t*` | allocator

- `void realm_leave (realm_t* r)`
    - Pops the current scope. Frees all entries created at the
        current depth, calling the per-entry free callback for each.
    - Parameters:
        - `r: realm_t*` | allocator

- `void* realm_put (realm_t* r, int mode, int n, const void* key, realm_free_t free, realm_alloc_t alloc, void* ctx)`
    - Stores a key-value pair. Behavior depends on mode:
        - `'!'`: assert if key exists, create if not
        - `'='`: return existing if key exists, call `alloc(n, key, ctx)`
          to create if not. Requires `alloc != NULL`.
        - `'~'`: free old and store new if key exists, create if not
    - When `alloc != NULL`, it is called to create the value (for all
      modes). The `ctx` parameter serves as context for the callback.
    - When `alloc == NULL`, `ctx` is used directly as the value.
    - The `free` callback is stored on the entry for later cleanup.
    - Parameters:
        - `r: realm_t*` | allocator
        - `mode: int` | one of `'!'`, `'='`, `'~'`
        - `n: int` | key length
        - `key: void*` | key buffer
        - `free: realm_free_t` | per-entry cleanup (NULL = no cleanup)
        - `alloc: realm_alloc_t` | factory callback (NULL = direct value)
        - `ctx: void*` | value or context for alloc
    - Return:
        - `void*` | the stored or existing value
    - Notes:
        - The library owns the key (allocates, copies, frees).
        - Requires `realm_enter` before first use (asserts depth > 0).
        - Complexity: O(n entries)

- `void* realm_get (realm_t* r, int n, const void* key)`
    - Retrieves the value for the given key. Global lookup across
        all depths.
    - Parameters:
        - `r: realm_t*` | allocator
        - `n: int` | key length
        - `key: void*` | key buffer
    - Return:
        - `void*` | pointer to value (`NULL` if not found)
    - Notes:
        - Complexity: O(n entries)
