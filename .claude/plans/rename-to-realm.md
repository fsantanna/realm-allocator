# Plan: Rename ttl_hash to realm

## Status: COMPLETE

## Decisions
- `cb_clean_t` → keep as-is
- `n_ttl` / `ttl` fields → keep as-is (domain concept)
- README title → rename to `realm`
- GitHub URLs → update to `realm`

## Step 1: Rename file `ttl_hash.h` → `realm.h`

## Step 2: Update `realm.h` (formerly `ttl_hash.h`)
| Line | Old | New |
|------|-----|-----|
| 1 | `TTL_HASH_H` | `REALM_H` |
| 2 | `TTL_HASH_H` | `REALM_H` |
| 9 | `struct ttl_hash_entry` | `struct realm_entry` |
| 15 | `} ttl_hash_entry` | `} realm_entry` |
| 17-22 | `ttl_hash` struct (and `ttl_hash_entry**`) | `realm` / `realm_entry**` |
| 24-29 | `ttl_hash_open/close/put/get/rem/tick` | `realm_open/close/put/get/rem/tick` |
| 33 | `TTL_HASH_C` | `REALM_C` |
| 36 | `ttl_hash_djb2` | `realm_djb2` |
| 46 | `ttl_hash_find` | `realm_find` |
| 61 | `ttl_hash_remove_entry` | `realm_remove_entry` |
| All | `ttl_hash*` (type refs) | `realm*` |
| All | `ttl_hash_entry*` (type refs) | `realm_entry*` |

## Step 3: Update `hello.c`
| Line | Old | New |
|------|-----|-----|
| 3 | `#define TTL_HASH_C` | `#define REALM_C` |
| 4 | `#include "ttl_hash.h"` | `#include "realm.h"` |
| 7+ | `ttl_hash*`, `ttl_hash_open/put/get/tick/close` | `realm*`, `realm_open/put/get/tick/close` |

## Step 4: Update `tst/unit.c`
| Line | Old | New |
|------|-----|-----|
| 1 | `#define TTL_HASH_C` | `#define REALM_C` |
| 2 | `#include "ttl_hash.h"` | `#include "realm.h"` |
| All | `ttl_hash*`, `ttl_hash_open/close/put/get/rem/tick` | `realm*`, `realm_open/close/put/get/rem/tick` |

## Step 5: Update `tst/app.c`
| Line | Old | New |
|------|-----|-----|
| 1 | `#define TTL_HASH_C` | `#define REALM_C` |
| 2 | `#include "ttl_hash.h"` | `#include "realm.h"` |
| 33 | `TTL-Hash Session` | `Realm Session` |
| 35+ | `ttl_hash*`, `ttl_hash_open/close/put/get/rem/tick` | `realm*`, `realm_open/close/put/get/rem/tick` |

## Step 6: Update `tst/bench.c`
| Line | Old | New |
|------|-----|-----|
| 7 | `#define TTL_HASH_C` | `#define REALM_C` |
| 8 | `#include "ttl_hash.h"` | `#include "realm.h"` |
| All | `ttl_hash*`, `ttl_hash_open/close/put/get` | `realm*`, `realm_open/close/put/get` |

## Step 7: Update `Makefile`
| Line | Old | New |
|------|-----|-----|
| 9 | `hello.c ttl_hash.h` | `hello.c realm.h` |
| 13 | `tst/... ttl_hash.h` | `tst/... realm.h` |

## Step 8: Update `README.md`
- Title: `ttl-hash` → `realm`
- Badge/URLs: `fsantanna/ttl-hash` → `fsantanna/realm`
- All `TTL-Hash` → `Realm`
- All `TTL_HASH_C` → `REALM_C`
- All `ttl_hash.h` → `realm.h`
- All `ttl_hash*` function refs → `realm_*`
- All `ttl_hash` type refs → `realm`

## Step 9: Update `CLAUDE.md`
- `TTL-Hash` → `Realm`
- `ttl_hash.h` → `realm.h`
- `TTL_HASH_C` → `REALM_C`

## Progress
- [x] Step 1 — renamed `ttl_hash.h` → `realm.h`
- [x] Step 2 — updated all 13 identifiers in `realm.h`
- [x] Step 3 — updated `hello.c`
- [x] Step 4 — updated `tst/unit.c`
- [x] Step 5 — updated `tst/app.c` (incl. session string)
- [x] Step 6 — updated `tst/bench.c`
- [x] Step 7 — updated `Makefile` deps
- [x] Step 8 — updated `README.md` (title, badges, API docs)
- [x] Step 9 — updated `CLAUDE.md`

## Verification
- `grep -r "ttl_hash"` — zero hits in source (only plan files)
- `grep -r "TTL_HASH"` — zero hits in source (only plan files)
- `grep -r "TTL-Hash"` — zero hits in source (only plan files)
