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