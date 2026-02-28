#define REALM_C
#include "realm.h"
#include <stdio.h>
#include <assert.h>

static int cleanup_count = 0;
static void* last_cleanup_value = NULL;

static void test_cleanup(int n, const void* key, void* value) {
    (void)n;
    (void)key;
    cleanup_count++;
    last_cleanup_value = value;
}

static void reset_cleanup(void) {
    cleanup_count = 0;
    last_cleanup_value = NULL;
}

int main(void) {

    /* Test: open and close */
    {
        printf("Test: open and close... ");
        realm* ht = realm_open(10, 5, NULL);
        assert(ht != NULL);
        realm_close(ht);
        printf("OK\n");
    }

    /* Test: put and get */
    {
        printf("Test: put and get... ");
        realm* ht = realm_open(10, 5, NULL);
        int value1 = 100;
        int value2 = 200;

        assert(realm_put(ht, 4, "key1", &value1) != NULL);
        assert(realm_put(ht, 4, "key2", &value2) != NULL);

        assert(realm_get(ht, 4, "key1") == &value1);
        assert(realm_get(ht, 4, "key2") == &value2);
        assert(realm_get(ht, 4, "key3") == NULL);

        realm_close(ht);
        printf("OK\n");
    }

    /* Test: put replaces existing key */
    {
        printf("Test: put replaces existing key... ");
        reset_cleanup();
        realm* ht = realm_open(10, 5, test_cleanup);
        int value1 = 100;
        int value2 = 200;

        assert(realm_put(ht, 3, "key", &value1) != NULL);
        assert(realm_get(ht, 3, "key") == &value1);

        assert(realm_put(ht, 3, "key", &value2) != NULL);
        assert(realm_get(ht, 3, "key") == &value2);
        assert(cleanup_count == 1);
        assert(last_cleanup_value == &value1);

        realm_close(ht);
        printf("OK\n");
    }

    /* Test: rem */
    {
        printf("Test: rem... ");
        reset_cleanup();
        realm* ht = realm_open(10, 5, test_cleanup);
        int value1 = 100;

        assert(realm_put(ht, 4, "key1", &value1) != NULL);
        assert(realm_get(ht, 4, "key1") == &value1);

        assert(realm_rem(ht, 4, "key1") == 0);
        assert(realm_get(ht, 4, "key1") == NULL);
        assert(cleanup_count == 1);
        assert(last_cleanup_value == &value1);

        assert(realm_rem(ht, 4, "key1") != 0);

        realm_close(ht);
        printf("OK\n");
    }

    /* Test: tick expires entries */
    {
        printf("Test: tick expires entries... ");
        reset_cleanup();
        realm* ht = realm_open(10, 3, test_cleanup);
        int value1 = 100;

        assert(realm_put(ht, 4, "key1", &value1) != NULL);

        realm_tick(ht);
        assert(realm_get(ht, 4, "key1") == &value1);

        realm_tick(ht);
        assert(realm_get(ht, 4, "key1") == &value1);

        realm_tick(ht);
        assert(realm_get(ht, 4, "key1") == &value1);

        realm_tick(ht);
        realm_tick(ht);
        realm_tick(ht);
        assert(realm_get(ht, 4, "key1") == NULL);
        assert(cleanup_count == 1);

        realm_close(ht);
        printf("OK\n");
    }

    /* Test: get resets ttl */
    {
        printf("Test: get resets ttl... ");
        reset_cleanup();
        realm* ht = realm_open(10, 2, test_cleanup);
        int value1 = 100;

        assert(realm_put(ht, 4, "key1", &value1) != NULL);

        realm_tick(ht);
        assert(realm_get(ht, 4, "key1") == &value1);

        realm_tick(ht);
        assert(realm_get(ht, 4, "key1") == &value1);

        realm_tick(ht);
        assert(realm_get(ht, 4, "key1") == &value1);

        realm_tick(ht);
        realm_tick(ht);
        assert(realm_get(ht, 4, "key1") == NULL);
        assert(cleanup_count == 1);

        realm_close(ht);
        printf("OK\n");
    }

    /* Test: close calls cleanup for all entries */
    {
        printf("Test: close calls cleanup for all entries... ");
        reset_cleanup();
        realm* ht = realm_open(10, 5, test_cleanup);
        int value1 = 100;
        int value2 = 200;
        int value3 = 300;

        assert(realm_put(ht, 4, "key1", &value1) != NULL);
        assert(realm_put(ht, 4, "key2", &value2) != NULL);
        assert(realm_put(ht, 4, "key3", &value3) != NULL);

        realm_close(ht);
        assert(cleanup_count == 3);
        printf("OK\n");
    }

    /* Test: binary keys */
    {
        printf("Test: binary keys... ");
        realm* ht = realm_open(10, 5, NULL);
        int value1 = 100;
        int value2 = 200;
        char bin1[] = {0x00, 0x01, 0x02};
        char bin2[] = {0x00, 0x01, 0x03};

        assert(realm_put(ht, 3, bin1, &value1) != NULL);
        assert(realm_put(ht, 3, bin2, &value2) != NULL);

        assert(realm_get(ht, 3, bin1) == &value1);
        assert(realm_get(ht, 3, bin2) == &value2);

        realm_close(ht);
        printf("OK\n");
    }

    /* Test: collision handling (same bucket) */
    {
        printf("Test: collision handling... ");
        realm* ht = realm_open(1, 5, NULL);
        int value1 = 100;
        int value2 = 200;
        int value3 = 300;

        assert(realm_put(ht, 1, "a", &value1) != NULL);
        assert(realm_put(ht, 1, "b", &value2) != NULL);
        assert(realm_put(ht, 1, "c", &value3) != NULL);

        assert(realm_get(ht, 1, "a") == &value1);
        assert(realm_get(ht, 1, "b") == &value2);
        assert(realm_get(ht, 1, "c") == &value3);

        assert(realm_rem(ht, 1, "b") == 0);
        assert(realm_get(ht, 1, "a") == &value1);
        assert(realm_get(ht, 1, "b") == NULL);
        assert(realm_get(ht, 1, "c") == &value3);

        realm_close(ht);
        printf("OK\n");
    }

    /* Test: NULL cleanup callback */
    {
        printf("Test: NULL cleanup callback... ");
        realm* ht = realm_open(10, 2, NULL);
        int value1 = 100;

        assert(realm_put(ht, 4, "key1", &value1) != NULL);
        realm_tick(ht);
        realm_tick(ht);
        assert(realm_get(ht, 4, "key1") == NULL);

        realm_close(ht);
        printf("OK\n");
    }

    printf("\nAll tests passed!\n");
    return 0;
}
