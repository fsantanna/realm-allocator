#ifndef REALM_H
#define REALM_H

#include <stdlib.h>
#include <string.h>

typedef void (*cb_clean_t) (int n, const void* key, void* value);

typedef struct realm_entry {
    int n;
    void* key;
    void* value;
    int ttl;
    struct realm_entry* next;
} realm_entry;

typedef struct {
    int n_buk;
    int n_ttl;
    cb_clean_t clean;
    realm_entry** buckets;
} realm;

realm* realm_open  (int n_buk, int n_ttl, cb_clean_t f);
void      realm_close (realm* ht);
const void* realm_put (realm* ht, int n, const void* key, void* value);
void*     realm_get   (realm* ht, int n, const void* key);
int       realm_rem   (realm* ht, int n, const void* key);
void      realm_tick  (realm* ht);

#endif

#ifdef REALM_C

/* DJB2 hash function */
static unsigned long realm_djb2 (int n, const void* key) {
    unsigned long hash = 5381;
    const unsigned char* p = (const unsigned char*)key;
    for (int i = 0; i < n; i++) {
        hash = ((hash << 5) + hash) + p[i];
    }
    return hash;
}

/* Find entry in bucket chain, return pointer to the link pointing to it */
static realm_entry** realm_find (realm* ht, int n, const void* key) {
    unsigned long hash = realm_djb2(n, key);
    int idx = hash % ht->n_buk;
    realm_entry** pp = &ht->buckets[idx];
    while (*pp != NULL) {
        realm_entry* e = *pp;
        if (e->n == n && memcmp(e->key, key, n) == 0) {
            return pp;
        }
        pp = &e->next;
    }
    return pp;
}

/* Remove entry and call cleanup callback */
static void realm_remove_entry (realm* ht, realm_entry** pp) {
    realm_entry* e = *pp;
    *pp = e->next;
    if (ht->clean != NULL) {
        ht->clean(e->n, e->key, e->value);
    }
    free(e->key);
    free(e);
}

realm* realm_open (int n_buk, int n_ttl, cb_clean_t f) {
    realm* ht = malloc(sizeof(realm));
    if (ht == NULL) {
        return NULL;
    }
    ht->buckets = calloc(n_buk, sizeof(realm_entry*));
    if (ht->buckets == NULL) {
        free(ht);
        return NULL;
    }
    ht->n_buk = n_buk;
    ht->n_ttl = n_ttl;
    ht->clean = f;
    return ht;
}

void realm_close (realm* ht) {
    for (int i = 0; i < ht->n_buk; i++) {
        while (ht->buckets[i] != NULL) {
            realm_remove_entry(ht, &ht->buckets[i]);
        }
    }
    free(ht->buckets);
    free(ht);
}

const void* realm_put (realm* ht, int n, const void* key, void* value) {
    realm_entry** pp = realm_find(ht, n, key);

    /* Key exists: replace value */
    if (*pp != NULL) {
        realm_entry* e = *pp;
        if (ht->clean != NULL) {
            ht->clean(e->n, e->key, e->value);
        }
        e->value = value;
        e->ttl = ht->n_ttl;
        return e->key;
    }

    /* Key does not exist: create new entry */
    realm_entry* e = (realm_entry*)malloc(sizeof(realm_entry));
    if (e == NULL) {
        return NULL;
    }
    e->key = malloc(n);
    if (e->key == NULL) {
        free(e);
        return NULL;
    }
    memcpy(e->key, key, n);
    e->n = n;
    e->value = value;
    e->ttl = ht->n_ttl;
    e->next = NULL;
    *pp = e;
    return e->key;
}

void* realm_get (realm* ht, int n, const void* key) {
    realm_entry** pp = realm_find(ht, n, key);
    if (*pp == NULL) {
        return NULL;
    }
    realm_entry* e = *pp;
    e->ttl = ht->n_ttl;
    return e->value;
}

int realm_rem (realm* ht, int n, const void* key) {
    realm_entry** pp = realm_find(ht, n, key);
    if (*pp == NULL) {
        return -1;
    }
    realm_remove_entry(ht, pp);
    return 0;
}

void realm_tick (realm* ht) {
    for (int i = 0; i < ht->n_buk; i++) {
        realm_entry** pp = &ht->buckets[i];
        while (*pp != NULL) {
            realm_entry* e = *pp;
            e->ttl--;
            if (e->ttl <= 0) {
                realm_remove_entry(ht, pp);
            } else {
                pp = &e->next;
            }
        }
    }
}

#endif
