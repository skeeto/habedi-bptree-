#define BPTREE_IMPLEMENTATION

#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "bptree.h"

int str_compare(const void *a, const void *b, const void *udata) {
    (void)udata;
    return strcmp((const char *)a, (const char *)b);
}

const bool debug_enabled = false;

void test_insertion_and_search() {
    printf("Test insertion and search...\n");
    bptree *tree = bptree_new(5, str_compare, NULL, NULL, NULL, debug_enabled);
    assert(tree != NULL);

    char *s1 = "apple";
    char *s2 = "banana";
    char *s3 = "cherry";

    assert(bptree_insert(tree, s1) == BPTREE_OK);
    assert(bptree_insert(tree, s2) == BPTREE_OK);
    assert(bptree_insert(tree, s3) == BPTREE_OK);

    char *res = bptree_search(tree, "banana");
    assert(res && strcmp(res, "banana") == 0);

    res = bptree_search(tree, "durian");
    assert(res == NULL);

    bptree_free(tree);
    printf("Insertion and search passed.\n");
}

void test_deletion() {
    printf("Test deletion...\n");
    bptree *tree = bptree_new(5, str_compare, NULL, NULL, NULL, debug_enabled);
    assert(tree != NULL);

    char *arr[] = {"alpha", "beta", "gamma", "delta", "epsilon"};
    for (size_t i = 0; i < 5; i++) {
        assert(bptree_insert(tree, arr[i]) == BPTREE_OK);
    }

    assert(bptree_delete(tree, "gamma") == BPTREE_OK);
    assert(bptree_search(tree, "gamma") == NULL);
    assert(bptree_delete(tree, "zeta") == BPTREE_NOT_FOUND);

    bptree_free(tree);
    printf("Deletion passed.\n");
}

void test_empty_tree() {
    printf("Test operations on an empty tree...\n");
    bptree *tree = bptree_new(5, str_compare, NULL, NULL, NULL, debug_enabled);
    assert(tree != NULL);

    assert(bptree_search(tree, "anything") == NULL);
    assert(bptree_delete(tree, "anything") == BPTREE_NOT_FOUND);

    bptree_free(tree);
    printf("Empty tree operations passed.\n");
}

void test_duplicate_insertion() {
    printf("Test duplicate insertion...\n");
    bptree *tree = bptree_new(5, str_compare, NULL, NULL, NULL, debug_enabled);
    assert(tree != NULL);

    char *dup = "duplicate";
    assert(bptree_insert(tree, dup) == BPTREE_OK);
    assert(bptree_insert(tree, dup) == BPTREE_DUPLICATE);

    char *res = bptree_search(tree, dup);
    assert(res && strcmp(res, dup) == 0);

    bptree_free(tree);
    printf("Duplicate insertion passed.\n");
}

void test_single_element() {
    printf("Test single element tree...\n");
    bptree *tree = bptree_new(5, str_compare, NULL, NULL, NULL, debug_enabled);
    assert(tree != NULL);

    char *solo = "solo";
    assert(bptree_insert(tree, solo) == BPTREE_OK);
    char *res = bptree_search(tree, solo);
    assert(res && strcmp(res, solo) == 0);

    assert(bptree_delete(tree, solo) == BPTREE_OK);
    assert(bptree_search(tree, solo) == NULL);

    bptree_free(tree);
    printf("Single element tree passed.\n");
}

void test_long_string_keys() {
    printf("Test long string keys...\n");
    bptree *tree = bptree_new(5, str_compare, NULL, NULL, NULL, debug_enabled);
    assert(tree != NULL);

    char long1[1024], long2[1024];
    memset(long1, 'a', sizeof(long1) - 1);
    long1[sizeof(long1) - 1] = '\0';
    memset(long2, 'b', sizeof(long2) - 1);
    long2[sizeof(long2) - 1] = '\0';

    assert(bptree_insert(tree, long1) == BPTREE_OK);
    assert(bptree_insert(tree, long2) == BPTREE_OK);

    char *res = bptree_search(tree, long1);
    assert(res && strcmp(res, long1) == 0);
    res = bptree_search(tree, long2);
    assert(res && strcmp(res, long2) == 0);

    assert(bptree_delete(tree, long1) == BPTREE_OK);
    assert(bptree_search(tree, long1) == NULL);

    bptree_free(tree);
    printf("Long string keys passed.\n");
}

void test_mixed_operations() {
    printf("Test mixed operations...\n");
    bptree *tree = bptree_new(5, str_compare, NULL, NULL, NULL, debug_enabled);
    assert(tree != NULL);

    char *keys1[] = {"one", "two", "three", "four", "five"};
    size_t count1 = sizeof(keys1) / sizeof(keys1[0]);
    for (size_t i = 0; i < count1; i++) {
        assert(bptree_insert(tree, keys1[i]) == BPTREE_OK);
    }

    assert(bptree_delete(tree, "three") == BPTREE_OK);
    assert(bptree_delete(tree, "five") == BPTREE_OK);
    assert(bptree_search(tree, "three") == NULL);
    assert(bptree_search(tree, "five") == NULL);

    char *keys2[] = {"six", "seven"};
    size_t count2 = sizeof(keys2) / sizeof(keys2[0]);
    for (size_t i = 0; i < count2; i++) {
        assert(bptree_insert(tree, keys2[i]) == BPTREE_OK);
    }
    assert(bptree_insert(tree, "three") == BPTREE_OK);

    char *res = bptree_search(tree, "two");
    assert(res && strcmp(res, "two") == 0);
    res = bptree_search(tree, "seven");
    assert(res && strcmp(res, "seven") == 0);
    res = bptree_search(tree, "three");
    assert(res && strcmp(res, "three") == 0);

    bptree_free(tree);
    printf("Mixed operations passed.\n");
}

void test_repeated_nonexistent_deletion() {
    printf("Test repeated deletion of non-existent keys...\n");
    bptree *tree = bptree_new(5, str_compare, NULL, NULL, NULL, debug_enabled);
    assert(tree != NULL);

    assert(bptree_insert(tree, "alpha") == BPTREE_OK);
    assert(bptree_insert(tree, "beta") == BPTREE_OK);

    assert(bptree_delete(tree, "gamma") == BPTREE_NOT_FOUND);
    assert(bptree_delete(tree, "delta") == BPTREE_NOT_FOUND);

    bptree_free(tree);
    printf("Repeated non-existent deletion passed.\n");
}

void test_empty_string_key() {
    printf("Test empty string key...\n");
    bptree *tree = bptree_new(5, str_compare, NULL, NULL, NULL, debug_enabled);
    assert(tree != NULL);

    char *empty = "";
    assert(bptree_insert(tree, empty) == BPTREE_OK);
    char *res = bptree_search(tree, empty);
    assert(res && strcmp(res, empty) == 0);
    assert(bptree_delete(tree, empty) == BPTREE_OK);
    assert(bptree_search(tree, empty) == NULL);

    bptree_free(tree);
    printf("Empty string key passed.\n");
}

void test_reinsertion_after_deletion() {
    printf("Test reinsertion after deletion...\n");
    bptree *tree = bptree_new(5, str_compare, NULL, NULL, NULL, debug_enabled);
    assert(tree != NULL);

    char *key = "reinsertion";
    assert(bptree_insert(tree, key) == BPTREE_OK);
    assert(bptree_delete(tree, key) == BPTREE_OK);
    assert(bptree_insert(tree, key) == BPTREE_OK);
    char *res = bptree_search(tree, key);
    assert(res && strcmp(res, key) == 0);

    bptree_free(tree);
    printf("Reinsertion after deletion passed.\n");
}

/* ----- Range Search Tests ----- */
void test_range_search_basic() {
    printf("Test range search (basic)...\n");
    bptree *tree = bptree_new(5, str_compare, NULL, NULL, NULL, debug_enabled);
    assert(tree != NULL);

    char *keys[] = {"apple", "banana", "cherry", "date", "fig", "grape"};
    size_t n = sizeof(keys) / sizeof(keys[0]);
    for (size_t i = 0; i < n; i++) {
        assert(bptree_insert(tree, keys[i]) == BPTREE_OK);
    }

    int count = 0;
    void **range = bptree_range_search(tree, "banana", "fig", &count);
    // Expected keys: banana, cherry, date, fig => count == 4
    assert(count == 4);
    // Verify the range values.
    assert(strcmp((const char *)range[0], "banana") == 0);
    assert(strcmp((const char *)range[1], "cherry") == 0);
    assert(strcmp((const char *)range[2], "date") == 0);
    assert(strcmp((const char *)range[3], "fig") == 0);
    tree->free_fn(range);
    bptree_free(tree);
    printf("Basic range search passed.\n");
}

void test_range_search_empty() {
    printf("Test range search (empty range)...\n");
    bptree *tree = bptree_new(5, str_compare, NULL, NULL, NULL, debug_enabled);
    assert(tree != NULL);

    char *keys[] = {"apple", "banana", "cherry"};
    size_t n = sizeof(keys) / sizeof(keys[0]);
    for (size_t i = 0; i < n; i++) {
        assert(bptree_insert(tree, keys[i]) == BPTREE_OK);
    }

    int count = 0;
    void **range = bptree_range_search(tree, "date", "fig", &count);
    // No keys between "date" and "fig"
    assert(count == 0);
    tree->free_fn(range);
    bptree_free(tree);
    printf("Empty range search passed.\n");
}

void test_range_search_full() {
    printf("Test range search (full range)...\n");
    bptree *tree = bptree_new(5, str_compare, NULL, NULL, NULL, debug_enabled);
    assert(tree != NULL);

    char *keys[] = {"apple", "banana", "cherry", "date", "fig", "grape"};
    size_t n = sizeof(keys) / sizeof(keys[0]);
    for (size_t i = 0; i < n; i++) {
        assert(bptree_insert(tree, keys[i]) == BPTREE_OK);
    }

    int count = 0;
    // Range covers all keys.
    void **range = bptree_range_search(tree, "apple", "grape", &count);
    assert(count == 6);
    tree->free_fn(range);
    bptree_free(tree);
    printf("Full range search passed.\n");
}

void test_range_search_boundaries() {
    printf("Test range search (boundary conditions)...\n");
    bptree *tree = bptree_new(5, str_compare, NULL, NULL, NULL, debug_enabled);
    assert(tree != NULL);

    char *keys[] = {"apple", "banana", "cherry", "date", "fig", "grape"};
    size_t n = sizeof(keys) / sizeof(keys[0]);
    for (size_t i = 0; i < n; i++) {
        assert(bptree_insert(tree, keys[i]) == BPTREE_OK);
    }

    int count = 0;
    // Query exactly one key: "cherry"
    void **range = bptree_range_search(tree, "cherry", "cherry", &count);
    assert(count == 1);
    assert(strcmp((const char *)range[0], "cherry") == 0);
    tree->free_fn(range);

    // Query range that starts before the first key and ends between keys.
    count = 0;
    range = bptree_range_search(tree, "aardvark", "blueberry", &count);
    // Expected: "apple", "banana"
    assert(count == 2);
    assert(strcmp((const char *)range[0], "apple") == 0);
    assert(strcmp((const char *)range[1], "banana") == 0);
    tree->free_fn(range);

    bptree_free(tree);
    printf("Boundary range search passed.\n");
}

int main(void) {
    // Run the tests
    test_insertion_and_search();
    test_deletion();
    test_empty_tree();
    test_duplicate_insertion();
    test_single_element();
    test_long_string_keys();
    test_mixed_operations();
    test_repeated_nonexistent_deletion();
    test_empty_string_key();
    test_reinsertion_after_deletion();
    test_range_search_basic();
    test_range_search_empty();
    test_range_search_full();
    test_range_search_boundaries();

    printf("All tests passed.\n");
    return 0;
}
