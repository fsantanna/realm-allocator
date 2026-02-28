#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <assert.h>
#include <string.h>

#define REALM_C
#include "realm.h"

/* Benchmark to demonstrate hash table O(1) average case vs O(n) degenerate case */

#define NUM_TRIALS 10000

/* Generate a key from an integer */
static int int_to_key(int value, char* key) {
    return snprintf(key, 32, "key_%08d", value);
}

/* Measure time for lookups */
static double measure_lookups(realm* ht, int num_entries) {
    char key[32];
    clock_t start = clock();

    for (int trial = 0; trial < NUM_TRIALS; trial++) {
        int idx = rand() % num_entries;
        int len = int_to_key(idx, key);
        void* result = realm_get(ht, len, key);
        assert(result != NULL);
    }

    clock_t end = clock();
    return (double)(end - start) / CLOCKS_PER_SEC;
}

int main(void) {
    srand(42);

    printf("=== Hash Table Complexity Benchmark ===\n\n");
    printf("Testing hash table (n_buckets = n_entries):\n");
    printf("Expected: O(1) average lookup time (constant regardless of size)\n\n");

    realm* ht1 = realm_open(100, 100, NULL);
    realm* ht2 = realm_open(4000, 100, NULL);
    char key[32];

    /* Insert entries */
    for (int i = 0; i < 100; i++) {
        int len = int_to_key(i, key);
        realm_put(ht1, len, key, (void*)(long)(i + 1));
    }
    for (int i = 0; i < 4000; i++) {
        int len = int_to_key(i, key);
        realm_put(ht2, len, key, (void*)(long)(i + 1));
    }

    double time_100 = measure_lookups(ht1, 100);
    double time_4000 = measure_lookups(ht2, 4000);

    printf("     100 entries,    100 buckets: %.6f sec (%d lookups)\n",
           time_100, NUM_TRIALS);
    printf("    4000 entries,   4000 buckets: %.6f sec (%d lookups)\n",
           time_4000, NUM_TRIALS);

    realm_close(ht1);
    realm_close(ht2);

    /* Assert O(1): 40x more entries should not increase time by more than 3x */
    double ratio = time_4000 / time_100;
    printf("  Ratio (4000/100): %.2fx\n", ratio);
    assert(ratio < 3.0);
    printf("  ✓ O(1) verified: time stayed relatively constant\n");

    printf("\nTesting degenerate case (1 bucket = linked list):\n");
    printf("Expected: O(n) lookup time (grows linearly with size)\n\n");

    realm* ll1 = realm_open(1, 100, NULL);
    realm* ll2 = realm_open(1, 100, NULL);

    /* Insert entries */
    for (int i = 0; i < 100; i++) {
        int len = int_to_key(i, key);
        realm_put(ll1, len, key, (void*)(long)(i + 1));
    }
    for (int i = 0; i < 4000; i++) {
        int len = int_to_key(i, key);
        realm_put(ll2, len, key, (void*)(long)(i + 1));
    }

    double time_ll_100 = measure_lookups(ll1, 100);
    double time_ll_4000 = measure_lookups(ll2, 4000);

    printf("     100 entries,      1 buckets: %.6f sec (%d lookups) [DEGENERATE]\n",
           time_ll_100, NUM_TRIALS);
    printf("    4000 entries,      1 buckets: %.6f sec (%d lookups) [DEGENERATE]\n",
           time_ll_4000, NUM_TRIALS);

    realm_close(ll1);
    realm_close(ll2);

    /* Assert O(n): 40x more entries should increase time by at least 10x */
    double ll_ratio = time_ll_4000 / time_ll_100;
    printf("  Ratio (4000/100): %.2fx\n", ll_ratio);
    assert(ll_ratio > 10.0);
    printf("  ✓ O(n) verified: time grew linearly with size\n");

    printf("\n=== Analysis ===\n");
    printf("Hash table (proper buckets): %.2fx slowdown for 40x data (constant time)\n", ratio);
    printf("Linked list (1 bucket):      %.2fx slowdown for 40x data (linear time)\n", ll_ratio);
    printf("\nThis proves the implementation uses hashing for O(1) average case,\n");
    printf("not a simple linked list with O(n) lookup.\n");
    printf("\nAll assertions passed!\n");

    return 0;
}
