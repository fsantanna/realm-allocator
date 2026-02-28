# realm

[![Tests][badge]][test]

[badge]: https://github.com/fsantanna/realm/actions/workflows/test.yml/badge.svg
[test]:  https://github.com/fsantanna/realm/actions/workflows/test.yml

`Realm` is a simple hash table for caches with time-to-live key-value pairs.

- A key is a plain memory buffer (owned by the library).
- A value is an opaque pointer to a payload (owned by the application).

Two keys match when they have the exact same memory bytes.

Each key-value pair starts with a time-to-live counter (`ttl`), which is reset
after each retrieval.
An exposed tick function decrements all `ttls` at once.
When a `ttl` reaches `0`, its associated key-value pair is removed from the
table, and a registered callback may cleanup the value.

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

// uses open, get, put, tick, close

int main (void) {
    realm* ht = realm_open(16, 3, cleanup_callback);

    while (running) {
        void* val = realm_get(ht, len, key);
        if (val == NULL) {
            realm_put(ht, key_len, key, value);
        }
        realm_tick(ht);
    }

    realm_close(ht);
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

- `typedef void (*cb_clean_t) (int n, const void* key, void* value)`
    - `cb_clean_t` is a function pointer type to cleanup callbacks.
        The callback is called whenever a key-value is removed.
    - Parameters:
        - `n: int` | key length
        - `key: void*` | key buffer
        - `value: void*` | value to clean

- `realm* realm_open (int n_buk, int n_ttl, cb_clean_t f)`
    - Creates a hash table with `n` buckets to hold entries.
    - Parameters:
        - `n_buk: int` | number of buckets
        - `n_ttl: int` | starting ttl for key-value pairs
        - `f: cb_clean_t` | cleanup callback for expired ttls (optional)
    - Return:
        - `realm*` | a pointer to the new hash table

- `void realm_close (realm* ht)`
    - Destroys a hash table, deallocating all existing entries.
        Calls the cleanup callback for each entry.
    - Parameters:
        - `ht: realm` | hash table to close
    - Return:
        - `void` | nothing

- `int realm_put (realm* ht, int n, const void* key, void* value)`
    - Stores a key-value pair into the given hash table.
    - Parameters:
        - `ht: realm` | hash table to store
        - `n: int` | key length
        - `key: void*` | key buffer
        - `value: void*` | non-NULL value pointer
    - Return:
        - `int` | `0` on success
    - Notes:
        - The hash table owns the key, but not the value.
            It allocates, copies, and releases all key buffer bytes properly.
            It ignores the value pointer, only passing it to the cleanup
            callback eventually.
        - If the key already exists, the new value substitutes the old, which
          is passed to the cleanup callback.
        - Complexity: O(n buckets)

- `void* realm_get (realm* ht, int n, const void* key)`
    - Retrieves the value associated with the given hash table and key.
    - Parameters:
        - `ht: realm` | hash table to query
        - `n: int` | key length
        - `key: void*` | key buffer
    - Return:
        - `void*` | pointer to associated value (`NULL` if non existent)
    - Notes
        - Complexity: O(n buckets)

- `int realm_rem (realm* ht, int n, const void* key)`
    - Removes the key-value pair associated with the given hash table and key.
    - Parameters:
        - `ht: realm` | hash table to remove
        - `n: int` | key length
        - `key: void*` | key buffer
    - Return:
        - `int` | `0` on success (key exists)
    - Notes
        - The cleanup callback is called for the key-value pair.
        - Complexity: O(n buckets).

- `void realm_tick (realm* ht)`
    - Decrements all key-value ttls at once, removing from the table those
      that reach `0`, and calling the cleanup callback for the value.
    - Parameters:
        - `ht: realm*`: hash table to tick
    - Return:
        - `void` | nothing
    - Notes
        - Complexity: O(n buckets)
