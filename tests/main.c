#include "hashtable.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* ------------------------------------------------------------------ */
/* Tiny test harness                                                   */
/* ------------------------------------------------------------------ */

static int tests_run = 0;
static int tests_failed = 0;

#define CHECK(cond, msg)                                                  \
    do {                                                                  \
        tests_run++;                                                      \
        if (cond) {                                                       \
            printf("  [PASS] %s\n", msg);                                 \
        } else {                                                          \
            tests_failed++;                                               \
            printf("  [FAIL] %s  (%s:%d)\n", msg, __FILE__, __LINE__);    \
        }                                                                 \
    } while (0)

static void section(const char *name) {
    printf("\n== %s ==\n", name);
}

/* ------------------------------------------------------------------ */
/* Tests                                                               */
/* ------------------------------------------------------------------ */

static void test_create_destroy(void) {
    section("Create / destroy");

    Hash *h = ht_create(8);
    CHECK(h != NULL, "ht_create(8) returns a table");
    CHECK(ht_size(h) == 0, "new table has size 0");
    ht_destroy(h);
    CHECK(1, "ht_destroy on an empty table");

    CHECK(ht_create(0) == NULL, "ht_create(0) returns NULL");

    ht_destroy(NULL);
    CHECK(1, "ht_destroy(NULL) does not crash");
}

static void test_insert_get(void) {
    section("Insert / get");

    Hash *h = ht_create(8);
    int v = 0;

    CHECK(ht_insert(h, "apple", 5),  "insert apple = 5");
    CHECK(ht_insert(h, "banana", 7), "insert banana = 7");
    CHECK(ht_insert(h, "cherry", 9), "insert cherry = 9");
    CHECK(ht_size(h) == 3, "size is 3");

    CHECK(ht_get(h, "apple", &v) && v == 5,  "get apple -> 5");
    CHECK(ht_get(h, "banana", &v) && v == 7, "get banana -> 7");
    CHECK(ht_get(h, "cherry", &v) && v == 9, "get cherry -> 9");

    ht_destroy(h);
}

static void test_update(void) {
    section("Update existing key");

    Hash *h = ht_create(8);
    int v = 0;

    ht_insert(h, "apple", 5);
    ht_insert(h, "banana", 7);

    CHECK(ht_insert(h, "apple", 42), "re-insert apple = 42");
    CHECK(ht_size(h) == 2, "size is still 2 (update, not a new entry)");
    CHECK(ht_get(h, "apple", &v) && v == 42, "get apple -> 42");
    CHECK(ht_get(h, "banana", &v) && v == 7, "banana untouched -> 7");

    ht_destroy(h);
}

static void test_missing(void) {
    section("Missing keys");

    Hash *h = ht_create(8);
    int v = 12345;

    ht_insert(h, "apple", 5);

    CHECK(!ht_get(h, "mango", &v), "get mango returns false");
    CHECK(v == 12345, "failed get leaves the out-value unchanged");
    CHECK(!ht_delete(h, "mango"), "delete mango returns false");
    CHECK(ht_size(h) == 1, "size unchanged after failed delete");

    ht_destroy(h);
}

static void test_key_is_copied(void) {
    section("Table owns its own copy of each key");

    Hash *h = ht_create(8);
    int v = 0;
    char buf[16];

    strcpy(buf, "temp");
    ht_insert(h, buf, 99);
    strcpy(buf, "XXXX");   /* overwrite the caller's string */

    CHECK(ht_get(h, "temp", &v) && v == 99,
          "key still found after caller's buffer changes");
    CHECK(!ht_get(h, "XXXX", &v),
          "new buffer contents are not a key");

    ht_destroy(h);
}

static void test_delete(void) {
    section("Delete");

    Hash *h = ht_create(8);
    int v = 0;

    ht_insert(h, "apple", 5);
    ht_insert(h, "banana", 7);
    ht_insert(h, "cherry", 9);

    CHECK(ht_delete(h, "banana"), "delete banana returns true");
    CHECK(ht_size(h) == 2, "size drops to 2");
    CHECK(!ht_get(h, "banana", &v), "banana is gone");
    CHECK(!ht_delete(h, "banana"), "deleting banana again returns false");
    CHECK(ht_get(h, "apple", &v) && v == 5,  "apple survives");
    CHECK(ht_get(h, "cherry", &v) && v == 9, "cherry survives");

    CHECK(ht_insert(h, "banana", 70), "re-insert banana after delete");
    CHECK(ht_get(h, "banana", &v) && v == 70, "get banana -> 70");
    CHECK(ht_size(h) == 3, "size back to 3");

    ht_destroy(h);
}

static void test_chains(void) {
    section("Delete from front / middle / end of chains (small table)");

    /* A tiny table forces lots of keys to share drawers. */
    Hash *h = ht_create(1);
    const char *keys[] = { "a", "b", "c", "d", "e" };
    int v = 0;

    for (int i = 0; i < 5; i++) {
        ht_insert(h, keys[i], i);
    }
    CHECK(ht_size(h) == 5, "5 keys inserted");

    CHECK(ht_delete(h, "a"), "delete first-inserted key (a)");
    CHECK(ht_delete(h, "e"), "delete last-inserted key (e)");
    CHECK(ht_delete(h, "c"), "delete middle key (c)");
    CHECK(ht_size(h) == 2, "size is 2");

    CHECK(ht_get(h, "b", &v) && v == 1, "b still -> 1");
    CHECK(ht_get(h, "d", &v) && v == 3, "d still -> 3");
    CHECK(!ht_get(h, "a", &v) && !ht_get(h, "c", &v) && !ht_get(h, "e", &v),
          "a, c, e all gone");

    ht_destroy(h);
}

static void test_resize(void) {
    section("Resize stress (1000 keys into a capacity-2 table)");

    Hash *h = ht_create(2);
    char buf[32];
    int v = 0;
    int insert_fail = 0, missing = 0, wrong = 0;

    for (int i = 0; i < 1000; i++) {
        snprintf(buf, sizeof buf, "key%d", i);
        if (!ht_insert(h, buf, i)) insert_fail++;
    }
    CHECK(insert_fail == 0, "all 1000 inserts succeed");
    CHECK(ht_size(h) == 1000, "size is 1000");

    for (int i = 0; i < 1000; i++) {
        snprintf(buf, sizeof buf, "key%d", i);
        if (!ht_get(h, buf, &v)) missing++;
        else if (v != i) wrong++;
    }
    CHECK(missing == 0, "every key is found after resizing");
    CHECK(wrong == 0, "every key has the right value");

    /* Delete the even keys, keep the odd ones. */
    for (int i = 0; i < 1000; i += 2) {
        snprintf(buf, sizeof buf, "key%d", i);
        ht_delete(h, buf);
    }
    CHECK(ht_size(h) == 500, "size is 500 after deleting even keys");

    missing = 0;
    int still_there = 0;
    for (int i = 0; i < 1000; i++) {
        snprintf(buf, sizeof buf, "key%d", i);
        int found = ht_get(h, buf, &v);
        if (i % 2 == 0 && found) still_there++;
        if (i % 2 == 1 && !found) missing++;
    }
    CHECK(still_there == 0, "no even keys remain");
    CHECK(missing == 0, "all odd keys remain");

    ht_destroy(h);
}

static void test_null_args(void) {
    section("NULL arguments are rejected safely");

    Hash *h = ht_create(8);
    int v = 0;

    CHECK(!ht_insert(NULL, "x", 1), "insert with NULL table -> false");
    CHECK(!ht_insert(h, NULL, 1),   "insert with NULL key -> false");
    CHECK(!ht_get(NULL, "x", &v),   "get with NULL table -> false");
    CHECK(!ht_get(h, NULL, &v),     "get with NULL key -> false");
    CHECK(!ht_get(h, "x", NULL),    "get with NULL out-value -> false");
    CHECK(!ht_delete(NULL, "x"),    "delete with NULL table -> false");
    CHECK(!ht_delete(h, NULL),      "delete with NULL key -> false");
    CHECK(ht_size(NULL) == 0,       "size of NULL table -> 0");

    ht_destroy(h);
}

/* ------------------------------------------------------------------ */
/* Real collision tests                                                */
/*                                                                     */
/* The tests above use ht_create(1) and hope keys share a drawer, but  */
/* a resize can spread them out. These tests instead FIND keys that    */
/* provably land in the same drawer, and keep the table small enough   */
/* that it never resizes.                                              */
/*                                                                     */
/* >>> MIRROR YOUR HASH: test_hash() below must compute the same      */
/* >>> value as the hash function in hashtable.c (djb2 shown here).   */
/* >>> If yours differs, edit test_hash() and nothing else.           */
/* ------------------------------------------------------------------ */

#define COLLISION_BUCKETS 64   /* capacity passed to ht_create          */
#define COLLISION_KEYS    4    /* size of the colliding group we build  */
#define SEARCH_LIMIT      100000

static unsigned long test_hash(const char *s) {
    unsigned long h = 5381;
    int c;
    while ((c = (unsigned char)*s++)) {
        h = ((h << 5) + h) + c;   /* h * 33 + c */
    }
    return h;
}

static size_t test_bucket(const char *s) {
    return (size_t)(test_hash(s) % COLLISION_BUCKETS);
}

/* Fills out[][32] with `want` distinct keys that share one bucket.
 * Pigeonhole guarantees this succeeds within
 * COLLISION_BUCKETS * (want - 1) + 1 candidates. Returns count found. */
static int find_colliding_keys(char out[][32], int want) {
    static int counts[COLLISION_BUCKETS];
    static char first_seen[COLLISION_BUCKETS][COLLISION_KEYS][32];
    memset(counts, 0, sizeof counts);

    char buf[32];
    for (int i = 0; i < SEARCH_LIMIT; i++) {
        snprintf(buf, sizeof buf, "col%d", i);
        size_t b = test_bucket(buf);
        if (counts[b] < want) {
            strcpy(first_seen[b][counts[b]], buf);
            counts[b]++;
        }
        if (counts[b] == want) {
            for (int k = 0; k < want; k++) {
                strcpy(out[k], first_seen[b][k]);
            }
            return want;
        }
    }
    return 0;
}

static void test_real_collisions(void) {
    section("Real collisions (keys proven to share a bucket)");

    char keys[COLLISION_KEYS][32];
    int found = find_colliding_keys(keys, COLLISION_KEYS);
    CHECK(found == COLLISION_KEYS, "found a group of keys that share a bucket");
    if (found != COLLISION_KEYS) return;

    /* Prove they collide under the mirrored hash. */
    int all_same = 1;
    for (int i = 1; i < COLLISION_KEYS; i++) {
        if (test_bucket(keys[i]) != test_bucket(keys[0])) all_same = 0;
    }
    CHECK(all_same, "all chosen keys map to the same bucket");

    int distinct = 1;
    for (int i = 0; i < COLLISION_KEYS; i++)
        for (int j = i + 1; j < COLLISION_KEYS; j++)
            if (strcmp(keys[i], keys[j]) == 0) distinct = 0;
    CHECK(distinct, "colliding keys are different strings");

    printf("  (colliding keys: %s, %s, %s, %s -> bucket %zu)\n",
           keys[0], keys[1], keys[2], keys[3], test_bucket(keys[0]));

    Hash *h = ht_create(COLLISION_BUCKETS);
    int v = -1;

    /* Insert all colliding keys with distinct values. */
    int inserts_ok = 1;
    for (int i = 0; i < COLLISION_KEYS; i++) {
        if (!ht_insert(h, keys[i], 100 + i)) inserts_ok = 0;
    }
    CHECK(inserts_ok, "all colliding keys insert successfully");
    CHECK(ht_size(h) == COLLISION_KEYS, "size counts every colliding key");

    /* Every one must still be retrievable with its OWN value. */
    int right = 1;
    for (int i = 0; i < COLLISION_KEYS; i++) {
        v = -1;
        if (!ht_get(h, keys[i], &v) || v != 100 + i) right = 0;
    }
    CHECK(right, "each colliding key returns its own value");

    /* A key in the same bucket that was never inserted must not be found. */
    char stranger[32];
    int have_stranger = 0;
    for (int i = SEARCH_LIMIT; i < SEARCH_LIMIT * 2 && !have_stranger; i++) {
        snprintf(stranger, sizeof stranger, "col%d", i);
        if (test_bucket(stranger) == test_bucket(keys[0])) have_stranger = 1;
    }
    if (have_stranger) {
        v = 777;
        CHECK(!ht_get(h, stranger, &v),
              "same-bucket key that was never inserted is not found");
        CHECK(v == 777, "failed same-bucket get leaves out-value unchanged");
        CHECK(!ht_delete(h, stranger),
              "delete of same-bucket absent key returns false");
        CHECK(ht_size(h) == COLLISION_KEYS, "size unchanged by that delete");
    }

    /* Update one key in the chain: others and size must be untouched. */
    CHECK(ht_insert(h, keys[1], 555), "update a key in the middle of a chain");
    CHECK(ht_size(h) == COLLISION_KEYS, "update does not grow the size");
    CHECK(ht_get(h, keys[1], &v) && v == 555, "updated colliding key has new value");
    CHECK(ht_get(h, keys[0], &v) && v == 100, "neighbor before update is untouched");
    CHECK(ht_get(h, keys[2], &v) && v == 102, "neighbor after update is untouched");

    /* Delete from the middle, then the front, then the end. */
    CHECK(ht_delete(h, keys[1]), "delete middle colliding key");
    CHECK(!ht_get(h, keys[1], &v), "deleted middle key is gone");
    CHECK(ht_get(h, keys[0], &v) && v == 100, "front survives middle delete");
    CHECK(ht_get(h, keys[2], &v) && v == 102, "later keys survive middle delete");
    CHECK(ht_get(h, keys[3], &v) && v == 103, "tail survives middle delete");

    CHECK(ht_delete(h, keys[0]), "delete front colliding key");
    CHECK(!ht_get(h, keys[0], &v), "deleted front key is gone");
    CHECK(ht_get(h, keys[2], &v) && v == 102, "remaining key 2 still found");
    CHECK(ht_get(h, keys[3], &v) && v == 103, "remaining key 3 still found");

    CHECK(ht_delete(h, keys[3]), "delete tail colliding key");
    CHECK(!ht_get(h, keys[3], &v), "deleted tail key is gone");
    CHECK(ht_get(h, keys[2], &v) && v == 102, "last remaining key still found");
    CHECK(ht_size(h) == 1, "size is 1 after three deletes");

    /* Re-insert into a chain that has been carved up. */
    CHECK(ht_insert(h, keys[0], 900), "re-insert into the same bucket");
    CHECK(ht_get(h, keys[0], &v) && v == 900, "re-inserted key has new value");
    CHECK(ht_get(h, keys[2], &v) && v == 102, "existing key unaffected by re-insert");
    CHECK(ht_size(h) == 2, "size is 2");

    /* Empty the bucket completely, then use it again. */
    CHECK(ht_delete(h, keys[0]) && ht_delete(h, keys[2]), "delete the rest");
    CHECK(ht_size(h) == 0, "table empty after removing the whole chain");
    CHECK(ht_insert(h, keys[3], 1), "insert into a fully emptied bucket");
    CHECK(ht_get(h, keys[3], &v) && v == 1, "and read it back");

    ht_destroy(h);
}

static void test_collisions_survive_resize(void) {
    section("Collisions survive a resize");

    /* Build a big colliding group in a table that WILL resize, then check
     * nothing is lost or mixed up when the chain is rehashed. */
    enum { N = 6 };
    char keys[N][32];
    Hash *h = ht_create(COLLISION_BUCKETS);
    int v = -1;

    /* find_colliding_keys uses COLLISION_KEYS-sized storage, so gather
     * colliding keys by hand here with a simple scan. */
    int n = 0;
    size_t target = test_bucket("col0");
    strcpy(keys[n++], "col0");
    char buf[32];
    for (int i = 1; i < SEARCH_LIMIT * 10 && n < N; i++) {
        snprintf(buf, sizeof buf, "col%d", i);
        if (test_bucket(buf) == target) strcpy(keys[n++], buf);
    }
    CHECK(n == N, "found six keys in one bucket");
    if (n != N) { ht_destroy(h); return; }

    for (int i = 0; i < N; i++) ht_insert(h, keys[i], 10 * i);

    /* Push the table well past any load-factor limit with filler keys. */
    int filler_fail = 0;
    for (int i = 0; i < 2000; i++) {
        snprintf(buf, sizeof buf, "filler%d", i);
        if (!ht_insert(h, buf, -i)) filler_fail++;
    }
    CHECK(filler_fail == 0, "filler inserts (forcing resize) all succeed");
    CHECK(ht_size(h) == N + 2000, "size counts colliding keys plus filler");

    int right = 1;
    for (int i = 0; i < N; i++) {
        v = -12345;
        if (!ht_get(h, keys[i], &v) || v != 10 * i) right = 0;
    }
    CHECK(right, "previously colliding keys keep their own values after resize");

    ht_destroy(h);
}

/* ------------------------------------------------------------------ */

int main(void) {
    printf("Hash table test suite\n");

    test_create_destroy();
    test_insert_get();
    test_update();
    test_missing();
    test_key_is_copied();
    test_delete();
    test_chains();
    test_resize();
    test_null_args();
    test_real_collisions();
    test_collisions_survive_resize();

    printf("\n======================================\n");
    printf("  %d / %d checks passed\n", tests_run - tests_failed, tests_run);
    if (tests_failed == 0) {
        printf("  ALL TESTS PASSED\n");
    } else {
        printf("  %d FAILED\n", tests_failed);
    }
    printf("======================================\n");

    return tests_failed == 0 ? 0 : 1;
}