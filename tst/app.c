#define REALM_C
#include "realm.h"
#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <time.h>

/* Simulates a cache for session data with TTL expiration */

typedef struct {
    int session_id;
    char username[32];
} session_t;

static int expired_count = 0;

static void session_cleanup(int n, const void* key, void* value) {
    session_t* session = (session_t*)value;
    printf("  [cleanup] session expired: id=%d key=%.*s user=%s\n",
           session->session_id, n, (const char*)key, session->username);
    free(session);
    expired_count++;
}

static session_t* create_session(int id, const char* username) {
    session_t* s = (session_t*)malloc(sizeof(session_t));
    s->session_id = id;
    snprintf(s->username, sizeof(s->username), "%s", username);
    return s;
}

int main(void) {
    printf("=== Realm Session Cache Simulation ===\n\n");

    realm* cache = realm_open(16, 3, session_cleanup);
    int session_id = 1000;

    /* Simulation loop: 10 ticks */
    for (int tick = 1; tick <= 10; tick++) {
        printf("--- Tick %d ---\n", tick);

        /* Add new sessions on certain ticks */
        if (tick == 1) {
            session_t* s1 = create_session(session_id++, "alice");
            assert(realm_put(cache, 6, "sess_a", s1) != NULL);
            printf("  [put] added session for alice\n");

            session_t* s2 = create_session(session_id++, "bob");
            assert(realm_put(cache, 6, "sess_b", s2) != NULL);
            printf("  [put] added session for bob\n");

            assert(realm_get(cache, 6, "sess_a") == s1);
            assert(realm_get(cache, 6, "sess_b") == s2);
        }

        if (tick == 3) {
            session_t* s3 = create_session(session_id++, "carol");
            assert(realm_put(cache, 6, "sess_c", s3) != NULL);
            printf("  [put] added session for carol\n");
            assert(realm_get(cache, 6, "sess_c") == s3);
        }

        if (tick == 5) {
            session_t* s4 = create_session(session_id++, "dave");
            assert(realm_put(cache, 6, "sess_d", s4) != NULL);
            printf("  [put] added session for dave\n");
            assert(realm_get(cache, 6, "sess_d") == s4);
        }

        /* Simulate accessing alice's session to keep it alive */
        if (tick == 2 || tick == 4 || tick == 6 || tick == 8) {
            session_t* s = (session_t*)realm_get(cache, 6, "sess_a");
            assert(s != NULL);
            printf("  [get] accessed alice's session (ttl reset)\n");
        }

        /* Simulate explicit logout for bob */
        if (tick == 3) {
            assert(realm_rem(cache, 6, "sess_b") == 0);
            printf("  [rem] bob logged out explicitly\n");
            assert(realm_get(cache, 6, "sess_b") == NULL);
        }

        /* Check who is still active */
        printf("  [status] active sessions: ");
        int active = 0;
        void* alice = realm_get(cache, 6, "sess_a");
        void* bob = realm_get(cache, 6, "sess_b");
        void* carol = realm_get(cache, 6, "sess_c");
        void* dave = realm_get(cache, 6, "sess_d");

        if (alice != NULL) { printf("alice "); active++; }
        if (bob != NULL) { printf("bob "); active++; }
        if (carol != NULL) { printf("carol "); active++; }
        if (dave != NULL) { printf("dave "); active++; }
        if (active == 0) printf("(none)");
        printf("\n");

        /* Verify expected states at specific ticks */
        if (tick == 1) {
            assert(alice != NULL && bob != NULL);
            assert(carol == NULL && dave == NULL);
        }
        if (tick == 3) {
            assert(alice != NULL && carol != NULL);
            assert(bob == NULL);
        }
        if (tick == 5) {
            assert(alice != NULL && carol != NULL && dave != NULL);
            assert(bob == NULL);
        }
        if (tick >= 8) {
            assert(alice != NULL);
        }

        /* Advance time */
        realm_tick(cache);
        printf("\n");
    }

    printf("--- Closing cache ---\n");
    realm_close(cache);

    printf("\n=== Summary ===\n");
    printf("Total sessions expired/cleaned: %d\n", expired_count);
    assert(expired_count == 4);

    printf("\nAll assertions passed!\n");
    return 0;
}
