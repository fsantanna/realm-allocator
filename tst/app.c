#define REALM_C
#include "realm.h"
#include <stdio.h>
#include <stdlib.h>
#include <assert.h>

/* Simulates a game with scene-based resource management */

typedef struct {
    int id;
    char name[32];
} resource_t;

static int freed_count = 0;

static void resource_free (int n, const void* key,
                           void* value) {
    resource_t* res = (resource_t*)value;
    printf("  [free] id=%d key=%.*s name=%s\n",
           res->id, n, (const char*)key, res->name);
    free(res);
    freed_count++;
}

static resource_t* create_resource (int id,
                                    const char* name) {
    resource_t* r = (resource_t*)malloc(sizeof(resource_t));
    r->id = id;
    snprintf(r->name, sizeof(r->name), "%s", name);
    return r;
}

int main (void) {
    printf("=== Realm Scene Demo ===\n\n");

    realm_t* r = realm_open(16);
    int next_id = 1;

    /* Depth 1: global resources */
    printf("--- Enter global realm ---\n");
    realm_enter(r);

    resource_t* font = create_resource(next_id++, "font");
    realm_put(r, '!', 4, "font",
              resource_free, NULL, font);
    printf("  [put] font (global)\n");

    resource_t* conf = create_resource(next_id++, "config");
    realm_put(r, '!', 6, "config",
              resource_free, NULL, conf);
    printf("  [put] config (global)\n");

    /* Depth 2: scene 1 */
    printf("\n--- Enter scene 1 ---\n");
    realm_enter(r);

    resource_t* tex_a = create_resource(next_id++, "sky");
    realm_put(r, '!', 3, "sky",
              resource_free, NULL, tex_a);
    printf("  [put] sky texture\n");

    resource_t* snd_a = create_resource(next_id++, "wind");
    realm_put(r, '!', 4, "wind",
              resource_free, NULL, snd_a);
    printf("  [put] wind sound\n");

    /* Globals still accessible */
    assert(realm_get(r, 4, "font") == font);
    assert(realm_get(r, 6, "config") == conf);
    printf("  [get] globals accessible from scene 1\n");

    /* Depth 3: dialog within scene 1 */
    printf("\n--- Enter dialog ---\n");
    realm_enter(r);

    resource_t* btn = create_resource(next_id++, "button");
    realm_put(r, '!', 6, "button",
              resource_free, NULL, btn);
    printf("  [put] button texture\n");

    /* Leave dialog */
    printf("\n--- Leave dialog ---\n");
    realm_leave(r);
    assert(realm_get(r, 6, "button") == NULL);
    assert(realm_get(r, 3, "sky") == tex_a);
    printf("  dialog resources freed, scene 1 intact\n");

    /* Leave scene 1 */
    printf("\n--- Leave scene 1 ---\n");
    realm_leave(r);
    assert(realm_get(r, 3, "sky") == NULL);
    assert(realm_get(r, 4, "wind") == NULL);
    assert(realm_get(r, 4, "font") == font);
    printf("  scene 1 resources freed, globals intact\n");

    /* Depth 2: scene 2 */
    printf("\n--- Enter scene 2 ---\n");
    realm_enter(r);

    resource_t* tex_b = create_resource(next_id++, "lava");
    realm_put(r, '!', 4, "lava",
              resource_free, NULL, tex_b);
    printf("  [put] lava texture\n");

    /* Globals still accessible */
    assert(realm_get(r, 4, "font") == font);
    printf("  [get] globals still accessible\n");

    /* Leave scene 2 */
    printf("\n--- Leave scene 2 ---\n");
    realm_leave(r);
    assert(realm_get(r, 4, "lava") == NULL);
    assert(realm_get(r, 4, "font") == font);
    printf("  scene 2 resources freed, globals intact\n");

    /* Close: frees globals */
    printf("\n--- Close ---\n");
    realm_close(r);

    printf("\n=== Summary ===\n");
    printf("Total resources freed: %d\n", freed_count);
    assert(freed_count == 6);

    printf("\nAll assertions passed!\n");
    return 0;
}
