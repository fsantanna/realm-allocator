#include <stdio.h>

#define REALM_C
#include "realm.h"

int main (void) {
    realm_t* r = realm_open(16);

    realm_enter(r);
    {
        int value1 = 42;
        int value2 = 99;

        realm_put(r, '!', 5, "hello",
                  NULL, NULL, &value1);
        realm_put(r, '!', 5, "world",
                  NULL, NULL, &value2);

        int* v = (int*)realm_get(r, 5, "hello");
        printf("hello = %d\n", *v);
    }
    realm_leave(r);

    if (realm_get(r, 5, "hello") == NULL) {
        printf("hello freed by realm_leave\n");
    } else {
        printf("ERROR: hello still exists\n");
    }

    realm_close(r);
    return 0;
}
