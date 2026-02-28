#include <stdio.h>

#define REALM_C
#include "realm.h"

int main(void) {
    realm* ht = realm_open(16, 3, NULL);

    int value1 = 42;
    int value2 = 99;

    realm_put(ht, 5, "hello", &value1);
    realm_put(ht, 5, "world", &value2);

    int* v = (int*)realm_get(ht, 5, "hello");
    printf("hello = %d\n", *v);

    realm_tick(ht);
    realm_tick(ht);
    realm_tick(ht);

    if (realm_get(ht, 5, "hello") == NULL) {
        printf("hello expired\n");
    }

    realm_close(ht);
    return 0;
}
