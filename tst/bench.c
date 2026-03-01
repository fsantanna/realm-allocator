#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <assert.h>
#include <string.h>

#define REALM_C
#include "realm.h"

/* Benchmark: hash table O(1) vs O(n) degenerate case */

#define NUM_TRIALS 10000

/* Generate a key from an integer */
static int int_to_key (int value, char* key) {
    return snprintf(key, 32, "key_%08d", value);
}

/* Measure time for lookups */
static double measure_lookups (realm_t* r,
                               int num_entries) {
    char key[32];
    clock_t start = clock();

    for (int trial = 0; trial < NUM_TRIALS; trial++) {
        int idx = rand() % num_entries;
        int len = int_to_key(idx, key);
        void* result = realm_get(r, len, key);
        assert(result != NULL);
    }

    clock_t end = clock();
    return (double)(end - start) / CLOCKS_PER_SEC;
}

int main (void) {
    srand(42);

    printf("=== Hash Table Complexity Benchmark ===\n\n");
    printf("Testing hash table (n_buckets = n_entries):\n");
    printf("Expected: O(1) average lookup time\n\n");

    realm_t* ht1 = realm_open(100);
    realm_t* ht2 = realm_open(4000);
    char key[32];

    realm_enter(ht1);
    for (int i = 0; i < 100; i++) {
        int len = int_to_key(i, key);
        realm_put(ht1, '!', len, key,
                  NULL, NULL, (void*)(long)(i + 1));
    }

    realm_enter(ht2);
    for (int i = 0; i < 4000; i++) {
        int len = int_to_key(i, key);
        realm_put(ht2, '!', len, key,
                  NULL, NULL, (void*)(long)(i + 1));
    }

    double time_100 = measure_lookups(ht1, 100);
    double time_4000 = measure_lookups(ht2, 4000);

    printf("     100 entries,    100 buckets: "
           "%.6f sec (%d lookups)\n",
           time_100, NUM_TRIALS);
    printf("    4000 entries,   4000 buckets: "
           "%.6f sec (%d lookups)\n",
           time_4000, NUM_TRIALS);

    realm_close(ht1);
    realm_close(ht2);

    double ratio = time_4000 / time_100;
    printf("  Ratio (4000/100): %.2fx\n", ratio);
    assert(ratio < 3.0);
    printf("  O(1) verified\n");

    printf("\nTesting degenerate case (1 bucket):\n");
    printf("Expected: O(n) lookup time\n\n");

    realm_t* ll1 = realm_open(1);
    realm_t* ll2 = realm_open(1);

    realm_enter(ll1);
    for (int i = 0; i < 100; i++) {
        int len = int_to_key(i, key);
        realm_put(ll1, '!', len, key,
                  NULL, NULL, (void*)(long)(i + 1));
    }

    realm_enter(ll2);
    for (int i = 0; i < 4000; i++) {
        int len = int_to_key(i, key);
        realm_put(ll2, '!', len, key,
                  NULL, NULL, (void*)(long)(i + 1));
    }

    double time_ll_100 = measure_lookups(ll1, 100);
    double time_ll_4000 = measure_lookups(ll2, 4000);

    printf("     100 entries,      1 bucket:  "
           "%.6f sec (%d lookups) [DEGENERATE]\n",
           time_ll_100, NUM_TRIALS);
    printf("    4000 entries,      1 bucket:  "
           "%.6f sec (%d lookups) [DEGENERATE]\n",
           time_ll_4000, NUM_TRIALS);

    realm_close(ll1);
    realm_close(ll2);

    double ll_ratio = time_ll_4000 / time_ll_100;
    printf("  Ratio (4000/100): %.2fx\n", ll_ratio);
    assert(ll_ratio > 10.0);
    printf("  O(n) verified\n");

    printf("\n=== Analysis ===\n");
    printf("Hash table: %.2fx for 40x data (constant)\n",
           ratio);
    printf("Linked list: %.2fx for 40x data (linear)\n",
           ll_ratio);
    printf("\nAll assertions passed!\n");

    return 0;
}
