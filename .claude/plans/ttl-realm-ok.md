# TTL Hash → Stack-Based Realm Allocator

## Context

The current `realm.h` is a TTL-based hash table (renamed from `ttl_hash`
symbols). TTL is fragile — it requires tick tuning and assumes "not accessed
recently" means "not needed". The caller always knows when resources should
die. This plan replaces TTL with:

1. **Stack-based realm lifetimes**: `realm_enter` pushes a scope,
   `realm_leave` pops it and frees all entries in that scope.
2. **3-mode put semantics**: controls collision behavior per resource type.

---

## Design Summary

### Removals
- `int ttl` field, `int n_ttl` config
- `realm_tick()`, `realm_rem()`
- TTL reset in `realm_get`
- `realm_dup` helper

### Additions
- `int depth` field on entries (stack level at creation)
- `int depth` counter on realm struct
- `realm_enter(r)`, `realm_leave(r)`
- 3-mode put: `'!'` exclusive, `'='` shared, `'~'` replaceable
- Factory callback for lazy creation (all modes can use it)

### Naming
- Lowercase types: `realm`, `realm_entry`
- Callbacks: `realm_clean_cb_t`, `realm_cb_t`
- Mode defines: `REALM_EXCLUSIVE '!'`, `REALM_SHARED '='`,
  `REALM_REPLACEABLE '~'`

### Key properties
- Depth: counter with safety cap (`REALM_MAX_DEPTH 8`),
  assert on overflow/underflow
- Key scope: global (single hash table, `realm_get` finds any entry)
- `realm_put` returns `void*` (value pointer)
- Empty stack (depth==0) on `realm_put` → assert
- Matching in `realm_leave`: integer comparison (`e->depth == r->depth`)

---

## 3-Mode Put Matrix

| Mode | Char | If key exists | If key not exists |
|------|------|---------------|-------------------|
| Exclusive | `'!'` | assert (bug) | create |
| Shared | `'='` | return existing | create (via cb) |
| Replaceable | `'~'` | clean old + create new | create |

### Value vs callback (unified across modes)

```
cb == NULL  →  value IS the resource directly
cb != NULL  →  value IS ctx, call cb(n, key, ctx) to create
```

| Mode | cb == NULL | cb != NULL |
|------|-----------|------------|
| `'!'` | direct value | lazy create, assert if exists |
| `'='` | **invalid** (assert) | get-or-create |
| `'~'` | direct value | lazy create/replace |

For `'~'` + key exists + cb != NULL: cb is called to create the
replacement value, then old value is cleaned.

### Return value

- `'!'`: the new value (or NULL on malloc failure)
- `'='`: existing value if key existed, new value from cb if created
- `'~'`: the new value (or NULL on malloc failure)

---

## Step 1: Rewrite `realm.h` — Header Section (lines 1–31)

File: `realm.h`

### Includes
- Add `#include <assert.h>`

### Types

```c
typedef void  (*realm_clean_cb_t) (int n, const void* key,
                                   void* value);
typedef void* (*realm_cb_t) (int n, const void* key,
                             void* ctx);
```

### Mode defines

```c
#define REALM_EXCLUSIVE   '!'
#define REALM_SHARED      '='
#define REALM_REPLACEABLE '~'

#define REALM_MAX_DEPTH 8
```

### `realm_entry` struct
```
REMOVE:  int ttl;
ADD:     int depth;
```

### `realm` struct
```
REMOVE:  int n_ttl;
ADD:     int depth;
```
Depth is just a counter.

### Declarations

```c
realm*  realm_open  (int n_buk, realm_clean_cb_t f);
void    realm_close (realm* r);
void    realm_enter (realm* r);
void    realm_leave (realm* r);
void*   realm_put   (realm* r, int n, const void* key,
                     void* value, realm_cb_t cb,
                     int mode);
void*   realm_get   (realm* r, int n, const void* key);
```

Removed: `realm_rem`, `realm_tick`.

---

## Step 2: Rewrite `realm.h` — Implementation (lines 33–164)

File: `realm.h`

### `realm_djb2` — NO CHANGE

### `realm_find` — NO CHANGE (rename `ht`→`r`)

### `realm_remove_entry` — NO CHANGE

### `realm_open`
- Remove `n_ttl` parameter
- Add `r->depth = 0;`

### `realm_close` — NO CHANGE

### `realm_enter` — ADD
```c
void realm_enter (realm* r) {
    assert(r->depth < REALM_MAX_DEPTH);
    r->depth++;
}
```

### `realm_leave` — ADD
```c
void realm_leave (realm* r) {
    assert(r->depth > 0);
    r->depth--;
    for (int i = 0; i < r->n_buk; i++) {
        realm_entry** pp = &r->buckets[i];
        while (*pp != NULL) {
            if ((*pp)->depth == r->depth) {
                realm_remove_entry(r, pp);
            } else {
                pp = &(*pp)->next;
            }
        }
    }
}
```

### `realm_put` — REWRITE

```c
void* realm_put (realm* r, int n, const void* key,
                 void* value, realm_cb_t cb, int mode) {
    assert(r->depth > 0);
    if (mode == '=') {
        assert(cb != NULL);
    }
    realm_entry** pp = realm_find(r, n, key);

    /* Key exists */
    if (*pp != NULL) {
        realm_entry* e = *pp;
        if (mode == '!') {
            assert(0 && "realm: exclusive key exists");
            return NULL;
        } else if (mode == '=') {
            return e->value;
        } else {
            void* nv = (cb != NULL)
                ? cb(n, key, value)
                : value;
            if (r->clean != NULL) {
                r->clean(e->n, e->key, e->value);
            }
            e->depth = r->depth - 1;
            e->value = nv;
            return nv;
        }
    }

    /* Key does not exist: create */
    void* nv = (cb != NULL)
        ? cb(n, key, value)
        : value;
    realm_entry* e = malloc(sizeof(realm_entry));
    if (e == NULL) return NULL;
    e->key = malloc(n);
    if (e->key == NULL) { free(e); return NULL; }
    memcpy(e->key, key, n);
    e->n = n;
    e->value = nv;
    e->depth = r->depth - 1;
    e->next = NULL;
    *pp = e;
    return nv;
}
```

### `realm_get` — SIMPLIFY
- Remove TTL reset, just find and return value

### DELETE `realm_rem`, `realm_tick`

---

## Step 3: Rewrite `hello.c`

Demonstrate enter/leave:
- `realm_open(16, NULL)`
- `realm_enter(r)`
- `realm_put` with `'!'` mode, `NULL` cb
- `realm_get` to retrieve + print
- `realm_leave(r)` — entries freed
- Verify get returns NULL
- `realm_close(r)`

---

## Step 4: Rewrite `tst/unit.c`

### Tests to UPDATE (adapt to new API):
| Test | Change |
|------|--------|
| open/close | `realm_open(10, NULL)` |
| put/get | Add `realm_enter`; `'!'` mode + `NULL` cb |
| put replaces | Use `'~'` mode for replacement |
| close cleanup | Add `realm_enter`; `'!'` mode |
| binary keys | Add `realm_enter`; `'!'` mode |
| collision | Add `realm_enter`; remove `realm_rem` |
| NULL callback | Replace tick expiry with `realm_leave` |

### Tests to DELETE:
- "rem" — `realm_rem` removed
- "tick expires entries" — `realm_tick` removed
- "get resets ttl" — TTL removed

### Tests to ADD:
1. **enter/leave** — enter, put, leave, verify freed + cleanup
2. **leave frees only current depth** — depth 1 survives
   when depth 2 is left
3. **nested realms** — 3 levels, leave innermost
4. **'~' moves entry to current depth** — enter, put key at
   depth 1, enter, put same key '~' at depth 2, leave depth 2,
   verify key gone (moved to depth 2)
5. **get is global** — get key from a different depth
6. **close frees all** — verify cleanup count
7. **'=' shared mode** — cb creates on miss, returns existing
   on hit
8. **'=' cb not called on hit** — verify cb invocation count
9. **'!' and '~' with cb** — verify lazy creation works

---

## Step 5: Rewrite `tst/app.c`

Scene-based realm demo:
- Depth 1: global resources
- Depth 2: scene-specific resources
- `realm_enter` + put resources + `realm_leave` = freed
- Global resources survive scene transitions
- `realm_close` frees globals

---

## Step 6: Update `tst/bench.c`

Minimal:
- `realm_open(N, 100, NULL)` → `realm_open(N, NULL)` (4 sites)
- Add `realm_enter(ht)` before insert loops
- `realm_put` gains `NULL, '!'` params
- `realm_get` and `realm_close` unchanged

---

## Step 7: Update `README.md`

Rewrite to reflect:
- New types: `realm_clean_cb_t`, `realm_cb_t`
- Mode defines and 3-mode matrix
- New API: `realm_enter`, `realm_leave`
- Changed: `realm_open` (no `n_ttl`), `realm_put` (new signature)
- Removed: `realm_rem`, `realm_tick`

---

## Step 8: Update `CLAUDE.md`

Change description from "hash table with time-to-live" to
"stack-based realm allocator for grouped resource lifetimes".

---

## Files Changed

| File | Change |
|------|--------|
| `realm.h` | Core rewrite: remove TTL, add depth + 3-mode put |
| `hello.c` | Rewrite for enter/leave demo |
| `tst/unit.c` | Update + add realm and mode tests |
| `tst/app.c` | Rewrite as scene-based demo |
| `tst/bench.c` | Fix signatures, add realm_enter |
| `README.md` | Rewrite API docs |
| `CLAUDE.md` | Update description |
| `Makefile` | No change |

---

## Verification

```bash
make tests    # builds + runs all tests under valgrind
make hello    # builds + runs hello example
```

Valgrind catches: memory leaks, use-after-free, invalid reads/writes.
