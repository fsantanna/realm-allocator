#define REALM_C
#include "realm.h"
#include <stdio.h>
#include <assert.h>

static int cleanup_count = 0;
static void* last_cleanup_value = NULL;

static void test_cleanup (int n, const void* key,
                          void* value) {
    (void)n;
    (void)key;
    cleanup_count++;
    last_cleanup_value = value;
}

static void reset_cleanup (void) {
    cleanup_count = 0;
    last_cleanup_value = NULL;
}

static int alloc_count = 0;

static void* test_alloc (int n, const void* key,
                         void* ctx) {
    (void)n;
    (void)key;
    alloc_count++;
    return ctx;
}

static void reset_alloc (void) {
    alloc_count = 0;
}

int main (void) {

    /* Test: open and close */
    {
        printf("Test: open and close... ");
        realm_t* r = realm_open(10);
        assert(r != NULL);
        realm_close(r);
        printf("OK\n");
    }

    /* Test: put and get */
    {
        printf("Test: put and get... ");
        realm_t* r = realm_open(10);
        int value1 = 100;
        int value2 = 200;

        realm_enter(r);

        assert(realm_put(r, '!', 4, "key1",
                         NULL, NULL, &value1) != NULL);
        assert(realm_put(r, '!', 4, "key2",
                         NULL, NULL, &value2) != NULL);

        assert(realm_get(r, 4, "key1") == &value1);
        assert(realm_get(r, 4, "key2") == &value2);
        assert(realm_get(r, 4, "key3") == NULL);

        realm_leave(r);
        realm_close(r);
        printf("OK\n");
    }

    /* Test: put replaces with '~' mode */
    {
        printf("Test: put replaces with '~' mode... ");
        reset_cleanup();
        realm_t* r = realm_open(10);
        int value1 = 100;
        int value2 = 200;

        realm_enter(r);

        assert(realm_put(r, '!', 3, "key",
                         test_cleanup, NULL,
                         &value1) != NULL);
        assert(realm_get(r, 3, "key") == &value1);

        assert(realm_put(r, '~', 3, "key",
                         test_cleanup, NULL,
                         &value2) != NULL);
        assert(realm_get(r, 3, "key") == &value2);
        assert(cleanup_count == 1);
        assert(last_cleanup_value == &value1);

        realm_leave(r);
        realm_close(r);
        printf("OK\n");
    }

    /* Test: close calls cleanup for all entries */
    {
        printf("Test: close calls cleanup... ");
        reset_cleanup();
        realm_t* r = realm_open(10);
        int value1 = 100;
        int value2 = 200;
        int value3 = 300;

        realm_enter(r);

        realm_put(r, '!', 4, "key1",
                  test_cleanup, NULL, &value1);
        realm_put(r, '!', 4, "key2",
                  test_cleanup, NULL, &value2);
        realm_put(r, '!', 4, "key3",
                  test_cleanup, NULL, &value3);

        realm_close(r);
        assert(cleanup_count == 3);
        printf("OK\n");
    }

    /* Test: binary keys */
    {
        printf("Test: binary keys... ");
        realm_t* r = realm_open(10);
        int value1 = 100;
        int value2 = 200;
        char bin1[] = {0x00, 0x01, 0x02};
        char bin2[] = {0x00, 0x01, 0x03};

        realm_enter(r);

        assert(realm_put(r, '!', 3, bin1,
                         NULL, NULL, &value1) != NULL);
        assert(realm_put(r, '!', 3, bin2,
                         NULL, NULL, &value2) != NULL);

        assert(realm_get(r, 3, bin1) == &value1);
        assert(realm_get(r, 3, bin2) == &value2);

        realm_leave(r);
        realm_close(r);
        printf("OK\n");
    }

    /* Test: collision handling (single bucket) */
    {
        printf("Test: collision handling... ");
        realm_t* r = realm_open(1);
        int value1 = 100;
        int value2 = 200;
        int value3 = 300;

        realm_enter(r);

        assert(realm_put(r, '!', 1, "a",
                         NULL, NULL, &value1) != NULL);
        assert(realm_put(r, '!', 1, "b",
                         NULL, NULL, &value2) != NULL);
        assert(realm_put(r, '!', 1, "c",
                         NULL, NULL, &value3) != NULL);

        assert(realm_get(r, 1, "a") == &value1);
        assert(realm_get(r, 1, "b") == &value2);
        assert(realm_get(r, 1, "c") == &value3);

        realm_leave(r);
        realm_close(r);
        printf("OK\n");
    }

    /* Test: NULL cleanup callback */
    {
        printf("Test: NULL cleanup callback... ");
        realm_t* r = realm_open(10);
        int value1 = 100;

        realm_enter(r);
        assert(realm_put(r, '!', 4, "key1",
                         NULL, NULL, &value1) != NULL);

        realm_leave(r);
        assert(realm_get(r, 4, "key1") == NULL);

        realm_close(r);
        printf("OK\n");
    }

    /* Test: enter/leave frees entries */
    {
        printf("Test: enter/leave frees entries... ");
        reset_cleanup();
        realm_t* r = realm_open(10);
        int value1 = 100;
        int value2 = 200;

        realm_enter(r);
        realm_put(r, '!', 4, "key1",
                  test_cleanup, NULL, &value1);
        realm_put(r, '!', 4, "key2",
                  test_cleanup, NULL, &value2);

        realm_leave(r);
        assert(realm_get(r, 4, "key1") == NULL);
        assert(realm_get(r, 4, "key2") == NULL);
        assert(cleanup_count == 2);

        realm_close(r);
        printf("OK\n");
    }

    /* Test: leave frees only current depth */
    {
        printf("Test: leave frees only current depth... ");
        reset_cleanup();
        realm_t* r = realm_open(10);
        int value1 = 100;
        int value2 = 200;

        realm_enter(r);
        realm_put(r, '!', 4, "key1",
                  test_cleanup, NULL, &value1);

        realm_enter(r);
        realm_put(r, '!', 4, "key2",
                  test_cleanup, NULL, &value2);

        realm_leave(r);
        assert(realm_get(r, 4, "key1") == &value1);
        assert(realm_get(r, 4, "key2") == NULL);
        assert(cleanup_count == 1);

        realm_leave(r);
        assert(realm_get(r, 4, "key1") == NULL);
        assert(cleanup_count == 2);

        realm_close(r);
        printf("OK\n");
    }

    /* Test: nested realms (3 levels) */
    {
        printf("Test: nested realms... ");
        reset_cleanup();
        realm_t* r = realm_open(10);
        int v1 = 1, v2 = 2, v3 = 3;

        realm_enter(r);
        realm_put(r, '!', 2, "a1",
                  test_cleanup, NULL, &v1);

        realm_enter(r);
        realm_put(r, '!', 2, "b1",
                  test_cleanup, NULL, &v2);

        realm_enter(r);
        realm_put(r, '!', 2, "c1",
                  test_cleanup, NULL, &v3);

        /* Leave innermost */
        realm_leave(r);
        assert(realm_get(r, 2, "c1") == NULL);
        assert(realm_get(r, 2, "b1") == &v2);
        assert(realm_get(r, 2, "a1") == &v1);
        assert(cleanup_count == 1);

        /* Leave middle */
        realm_leave(r);
        assert(realm_get(r, 2, "b1") == NULL);
        assert(realm_get(r, 2, "a1") == &v1);
        assert(cleanup_count == 2);

        /* Leave outermost */
        realm_leave(r);
        assert(realm_get(r, 2, "a1") == NULL);
        assert(cleanup_count == 3);

        realm_close(r);
        printf("OK\n");
    }

    /* Test: '~' moves entry to current depth */
    {
        printf("Test: '~' moves entry to current depth... ");
        reset_cleanup();
        realm_t* r = realm_open(10);
        int v1 = 100, v2 = 200;

        realm_enter(r);
        realm_put(r, '!', 3, "key",
                  test_cleanup, NULL, &v1);

        realm_enter(r);
        realm_put(r, '~', 3, "key",
                  test_cleanup, NULL, &v2);
        assert(cleanup_count == 1);
        assert(last_cleanup_value == &v1);

        /* Entry moved to depth 2; leaving depth 2 frees it */
        realm_leave(r);
        assert(realm_get(r, 3, "key") == NULL);
        assert(cleanup_count == 2);
        assert(last_cleanup_value == &v2);

        realm_leave(r);
        realm_close(r);
        printf("OK\n");
    }

    /* Test: get is global across depths */
    {
        printf("Test: get is global across depths... ");
        realm_t* r = realm_open(10);
        int v1 = 100, v2 = 200;

        realm_enter(r);
        realm_put(r, '!', 4, "key1",
                  NULL, NULL, &v1);

        realm_enter(r);
        realm_put(r, '!', 4, "key2",
                  NULL, NULL, &v2);

        /* Both visible from depth 2 */
        assert(realm_get(r, 4, "key1") == &v1);
        assert(realm_get(r, 4, "key2") == &v2);

        realm_leave(r);
        realm_leave(r);
        realm_close(r);
        printf("OK\n");
    }

    /* Test: close frees all depths */
    {
        printf("Test: close frees all depths... ");
        reset_cleanup();
        realm_t* r = realm_open(10);
        int v1 = 1, v2 = 2;

        realm_enter(r);
        realm_put(r, '!', 2, "a1",
                  test_cleanup, NULL, &v1);

        realm_enter(r);
        realm_put(r, '!', 2, "b1",
                  test_cleanup, NULL, &v2);

        realm_close(r);
        assert(cleanup_count == 2);
        printf("OK\n");
    }

    /* Test: '=' shared mode — cb creates on miss */
    {
        printf("Test: '=' shared mode... ");
        reset_alloc();
        realm_t* r = realm_open(10);
        int v1 = 100;

        realm_enter(r);

        /* First put: cb called, creates entry */
        void* result = realm_put(r, '=', 3, "key",
                                 NULL, test_alloc, &v1);
        assert(result == &v1);
        assert(alloc_count == 1);

        /* Second put: cb NOT called, returns existing */
        int v2 = 200;
        result = realm_put(r, '=', 3, "key",
                           NULL, test_alloc, &v2);
        assert(result == &v1);
        assert(alloc_count == 1);

        realm_leave(r);
        realm_close(r);
        printf("OK\n");
    }

    /* Test: '!' and '~' with cb (lazy creation) */
    {
        printf("Test: '!' and '~' with cb... ");
        reset_alloc();
        reset_cleanup();
        realm_t* r = realm_open(10);
        int v1 = 100, v2 = 200;

        realm_enter(r);

        /* '!' with cb: creates via callback */
        void* result = realm_put(r, '!', 3, "key",
                                 test_cleanup, test_alloc,
                                 &v1);
        assert(result == &v1);
        assert(alloc_count == 1);

        /* '~' with cb: replaces via callback */
        result = realm_put(r, '~', 3, "key",
                           test_cleanup, test_alloc,
                           &v2);
        assert(result == &v2);
        assert(alloc_count == 2);
        assert(cleanup_count == 1);
        assert(last_cleanup_value == &v1);

        realm_leave(r);
        realm_close(r);
        printf("OK\n");
    }

    printf("\nAll tests passed!\n");
    return 0;
}
