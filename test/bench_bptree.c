#define BPTREE_IMPLEMENTATION
#include <assert.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#include "bptree.h"

const bool debug_enabled = false;

int compare_ints(const void *a, const void *b, const void *udata) {
    (void)udata;
    int ia = *(const int *)a;
    int ib = *(const int *)b;
    return (ia > ib) - (ia < ib);
}

int compare_ints_qsort(const void *a, const void *b) {
    int *ia = *(int **)a;
    int *ib = *(int **)b;
    return (*ia > *ib) - (*ia < *ib);
}

#define BENCH(label, count, code_block)                                                           \
    do {                                                                                          \
        clock_t start = clock();                                                                  \
        for (int bench_i = 0; bench_i < (count); bench_i++) {                                     \
            code_block;                                                                           \
        }                                                                                         \
        clock_t end = clock();                                                                    \
        double elapsed = (double)(end - start) / CLOCKS_PER_SEC;                                  \
        printf("%s: %d iterations in %f sec (%f sec per iteration)\n", (label), (count), elapsed, \
               elapsed / (count));                                                                \
    } while (0)

void shuffle(void **array, int n) {
    for (int i = n - 1; i > 0; i--) {
        int j = rand() % (i + 1);
        void *temp = array[i];
        array[i] = array[j];
        array[j] = temp;
    }
}

int main(void) {
    int seed = getenv("SEED") ? atoi(getenv("SEED")) : (int)time(NULL);
    int max_keys = getenv("MAX_ITEMS") ? atoi(getenv("MAX_ITEMS")) : 32;
    int N = getenv("N") ? atoi(getenv("N")) : 1000000;
    if (N <= 0) {
        fprintf(stderr, "Invalid N value (%d); defaulting to 1000000\n", N);
        N = 1000000;
    }
    printf("SEED=%d, MAX_ITEMS=%d, N=%d\n", seed, max_keys, N);
    srand(seed);

    int *vals = malloc(N * sizeof(int));
    void **pointers = malloc(N * sizeof(void *));
    if (!vals || !pointers) {
        fprintf(stderr, "Allocation failed\n");
        exit(1);
    }
    for (int i = 0; i < N; i++) {
        vals[i] = i;
        pointers[i] = &vals[i];
    }

    /* --- Insertion Benchmarks --- */
    // Random Insertion Benchmark
    shuffle(pointers, N);
    {
        bptree *tree = bptree_new(max_keys, compare_ints, NULL, NULL, NULL, debug_enabled);
        if (!tree) {
            fprintf(stderr, "Failed to create tree\n");
            exit(1);
        }
        BENCH("Insertion (rand)", N, {
            bptree_status stat = bptree_insert(tree, pointers[bench_i]);
            assert(stat == BPTREE_OK);
        });
        bptree_free(tree);
    }
    // Sequential Insertion Benchmark
    qsort(pointers, N, sizeof(void *), (int (*)(const void *, const void *))compare_ints_qsort);
    {
        bptree *tree = bptree_new(max_keys, compare_ints, NULL, NULL, NULL, debug_enabled);
        if (!tree) {
            fprintf(stderr, "Failed to create tree\n");
            exit(1);
        }
        BENCH("Insertion (seq)", N, {
            bptree_status stat = bptree_insert(tree, pointers[bench_i]);
            assert(stat == BPTREE_OK);
        });
        bptree_free(tree);
    }

    /* --- Search Benchmarks --- */
    // Random Search Benchmark
    shuffle(pointers, N);
    {
        bptree *tree = bptree_new(max_keys, compare_ints, NULL, NULL, NULL, debug_enabled);
        if (!tree) {
            fprintf(stderr, "Failed to create tree\n");
            exit(1);
        }
        for (int i = 0; i < N; i++) {
            bptree_status stat = bptree_insert(tree, pointers[i]);
            assert(stat == BPTREE_OK);
        }
        BENCH("Search (rand)", N, {
            void *res = bptree_search(tree, pointers[bench_i]);
            if (!res) {
                printf("Search failed at index %d, pointer=%p, value=%d\n", bench_i,
                       pointers[bench_i], *(int *)pointers[bench_i]);
            }
            assert(res != NULL);
        });
        bptree_free(tree);
    }
    // Sequential Search Benchmark
    qsort(pointers, N, sizeof(void *), (int (*)(const void *, const void *))compare_ints_qsort);
    {
        bptree *tree = bptree_new(max_keys, compare_ints, NULL, NULL, NULL, debug_enabled);
        if (!tree) {
            fprintf(stderr, "Failed to create tree\n");
            exit(1);
        }
        for (int i = 0; i < N; i++) {
            bptree_status stat = bptree_insert(tree, pointers[i]);
            assert(stat == BPTREE_OK);
        }
        BENCH("Search (seq)", N, {
            void *res = bptree_search(tree, pointers[bench_i]);
            if (!res) {
                printf("Sequential search failed at index %d, pointer=%p, value=%d\n", bench_i,
                       pointers[bench_i], *(int *)pointers[bench_i]);
            }
            assert(res != NULL);
        });
        bptree_free(tree);
    }

    /* --- Deletion Benchmarks --- */
    // Random Deletion Benchmark
    {
        shuffle(pointers, N);
        bptree *tree = bptree_new(max_keys, compare_ints, NULL, NULL, NULL, debug_enabled);
        if (!tree) {
            fprintf(stderr, "Failed to create tree\n");
            exit(1);
        }
        for (int i = 0; i < N; i++) {
            bptree_status stat = bptree_insert(tree, pointers[i]);
            assert(stat == BPTREE_OK);
        }
        shuffle(pointers, N);
        BENCH("Deletion (rand)", N, {
            bptree_status stat = bptree_delete(tree, pointers[bench_i]);
            if (stat != BPTREE_OK) {
                printf("Random deletion failed at index %d, pointer=%p, value=%d\n", bench_i,
                       pointers[bench_i], *(int *)pointers[bench_i]);
            }
            assert(stat == BPTREE_OK);
        });
        bptree_free(tree);
    }
    // Sequential Deletion Benchmark
    {
        qsort(pointers, N, sizeof(void *), (int (*)(const void *, const void *))compare_ints_qsort);
        bptree *tree = bptree_new(max_keys, compare_ints, NULL, NULL, NULL, debug_enabled);
        if (!tree) {
            fprintf(stderr, "Failed to create tree\n");
            exit(1);
        }
        for (int i = 0; i < N; i++) {
            bptree_status stat = bptree_insert(tree, pointers[i]);
            assert(stat == BPTREE_OK);
        }
        BENCH("Deletion (seq)", N, {
            bptree_status stat = bptree_delete(tree, pointers[bench_i]);
            if (stat != BPTREE_OK) {
                printf("Sequential deletion failed at index %d, pointer=%p, value=%d\n", bench_i,
                       pointers[bench_i], *(int *)pointers[bench_i]);
            }
            assert(stat == BPTREE_OK);
        });
        bptree_free(tree);
    }

    /* --- Range Search Benchmarks --- */
    // Sequential Range Search Benchmark
    {
        qsort(pointers, N, sizeof(void *), (int (*)(const void *, const void *))compare_ints_qsort);
        bptree *tree = bptree_new(max_keys, compare_ints, NULL, NULL, NULL, debug_enabled);
        if (!tree) {
            fprintf(stderr, "Failed to create tree\n");
            exit(1);
        }
        for (int i = 0; i < N; i++) {
            bptree_status stat = bptree_insert(tree, pointers[i]);
            assert(stat == BPTREE_OK);
        }
        BENCH("Range Search (seq)", N, {
            int delta = 100;
            int idx = bench_i;
            int end_idx = idx + delta;
            if (end_idx >= N) {
                end_idx = N - 1;
            }
            int count = 0;
            void **res = bptree_range_search(tree, pointers[idx], pointers[end_idx], &count);
            assert(count >= 0);
            tree->free_fn(res);
        });
        bptree_free(tree);
    }

    free(vals);
    free(pointers);
    return 0;
}
