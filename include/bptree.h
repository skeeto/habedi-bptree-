#ifndef BPTREE_H
#define BPTREE_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stdarg.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdio.h>
#include <time.h>

static inline const char *logger_timestamp(void) {
    static char buf[32];
    time_t now = time(NULL);
    struct tm *tm_info = localtime(&now);
    strftime(buf, sizeof(buf), "%Y-%m-%d %H:%M:%S", tm_info);
    return buf;
}

static inline void bptree_logger(const char *fmt, ...) {
    va_list args;
    va_start(args, fmt);
    printf("[%s] [DBG] ", logger_timestamp());
    vprintf(fmt, args);
    printf("\n");
    va_end(args);
}

typedef void *(*bptree_malloc_t)(size_t size);
typedef void (*bptree_free_t)(void *ptr);

typedef enum {
    BPTREE_OK,
    BPTREE_DUPLICATE,
    BPTREE_ALLOCATION_ERROR,
    BPTREE_NOT_FOUND,
    BPTREE_ERROR
} bptree_status;

typedef struct bptree bptree;

bptree *bptree_new(int max_keys, int (*compare)(const void *a, const void *b, const void *udata),
                   void *udata, bptree_malloc_t malloc_fn, bptree_free_t free_fn,
                   bool debug_enabled);
void bptree_free(bptree *tree);
bptree_status bptree_insert(bptree *tree, void *item);
bptree_status bptree_delete(bptree *tree, const void *key);
void *bptree_search(const bptree *tree, const void *key);
/* Range search: returns an array of items whose keys are between start_key and end_key (inclusive).
   The number of results is stored in *count. The returned array must be freed by the caller using
   tree->free_fn.
*/
void **bptree_range_search(const bptree *tree, const void *start_key, const void *end_key,
                           int *count);

#ifdef __cplusplus
}
#endif

#ifdef BPTREE_IMPLEMENTATION

#include <assert.h>
#include <stdlib.h>
#include <string.h>

#define BPTREE_LOG_DEBUG(tree, ...)                            \
    do {                                                       \
        if ((tree)->debug_enabled) bptree_logger(__VA_ARGS__); \
    } while (0)

static void *default_malloc(size_t size) { return malloc(size); }
static void default_free(void *ptr) { free(ptr); }

/* Node structure.
   Both internal and leaf nodes have a keys array.
   Leaf nodes also store items and a next pointer.
*/
typedef struct bptree_node {
    int is_leaf;
    int num_keys;
    void **keys;
    union {
        struct {
            void **items;
            struct bptree_node *next;
        } leaf;
        struct {
            struct bptree_node **children;
        } internal;
    } ptr;
} bptree_node;

struct bptree {
    int max_keys;
    int min_keys;
    int height;
    int count;
    int (*compare)(const void *a, const void *b, const void *udata);
    void *udata;
    bptree_node *root;
    bptree_malloc_t malloc_fn;
    bptree_free_t free_fn;
    bool debug_enabled;
};

/* Unified binary search helper */
static inline int binary_search(const bptree *tree, void *const *array, int count,
                                const void *key) {
    int low = 0, high = count - 1;
    while (low <= high) {
        int mid = (low + high) / 2;
        int cmp = tree->compare(key, array[mid], tree->udata);
        if (cmp == 0) return mid;
        if (cmp < 0)
            high = mid - 1;
        else
            low = mid + 1;
    }
    return low;
}

/* For leaf nodes, use binary search directly */
static inline int leaf_node_search(const bptree *tree, void *const *keys, int count,
                                   const void *key) {
    return binary_search(tree, keys, count, key);
}

/* For internal nodes, adjust the result so that the index is in [0, num_keys] */
static inline int internal_node_search(const bptree *tree, void *const *keys, int count,
                                       const void *key) {
    int low = 0, high = count;
    while (low < high) {
        int mid = (low + high) / 2;
        if (tree->compare(key, keys[mid], tree->udata) < 0)
            high = mid;
        else
            low = mid + 1;
    }
    return low;
}

static bptree_node *create_leaf(bptree *tree) {
    bptree_node *node = tree->malloc_fn(sizeof(bptree_node));
    if (!node) {
        BPTREE_LOG_DEBUG(tree, "Allocation failure (leaf)");
        return NULL;
    }
    node->is_leaf = 1;
    node->num_keys = 0;
    node->keys = tree->malloc_fn(tree->max_keys * sizeof(void *));
    if (!node->keys) {
        tree->free_fn(node);
        BPTREE_LOG_DEBUG(tree, "Allocation failure (leaf keys)");
        return NULL;
    }
    node->ptr.leaf.items = tree->malloc_fn(tree->max_keys * sizeof(void *));
    if (!node->ptr.leaf.items) {
        tree->free_fn(node->keys);
        tree->free_fn(node);
        BPTREE_LOG_DEBUG(tree, "Allocation failure (leaf items)");
        return NULL;
    }
    node->ptr.leaf.next = NULL;
    return node;
}

static bptree_node *create_internal(bptree *tree) {
    bptree_node *node = tree->malloc_fn(sizeof(bptree_node));
    if (!node) {
        BPTREE_LOG_DEBUG(tree, "Allocation failure (internal)");
        return NULL;
    }
    node->is_leaf = 0;
    node->num_keys = 0;
    node->keys = tree->malloc_fn(tree->max_keys * sizeof(void *));
    if (!node->keys) {
        tree->free_fn(node);
        BPTREE_LOG_DEBUG(tree, "Allocation failure (internal keys)");
        return NULL;
    }
    node->ptr.internal.children = tree->malloc_fn((tree->max_keys + 1) * sizeof(bptree_node *));
    if (!node->ptr.internal.children) {
        tree->free_fn(node->keys);
        tree->free_fn(node);
        BPTREE_LOG_DEBUG(tree, "Allocation failure (internal children)");
        return NULL;
    }
    return node;
}

static void free_node(bptree *tree, bptree_node *node) {
    if (!node) return;
    if (node->is_leaf) {
        tree->free_fn(node->keys);
        tree->free_fn(node->ptr.leaf.items);
    } else {
        for (int i = 0; i <= node->num_keys; i++) free_node(tree, node->ptr.internal.children[i]);
        tree->free_fn(node->keys);
        tree->free_fn(node->ptr.internal.children);
    }
    tree->free_fn(node);
}

typedef struct {
    void *promoted_key;
    bptree_node *new_child;
    bptree_status status;
} insert_result;

static insert_result split_internal(bptree *tree, bptree_node *node, void *new_key,
                                    bptree_node *new_child, int pos) {
    insert_result res = {NULL, NULL, BPTREE_ERROR};
    int total = node->num_keys + 1;
    int split = total / 2;
    void **all_keys = tree->malloc_fn(total * sizeof(void *));
    bptree_node **all_children = tree->malloc_fn((total + 1) * sizeof(bptree_node *));
    if (!all_keys || !all_children) {
        if (all_keys) tree->free_fn(all_keys);
        if (all_children) tree->free_fn(all_children);
        return res;
    }
    memcpy(all_keys, node->keys, node->num_keys * sizeof(void *));
    memcpy(all_children, node->ptr.internal.children, (node->num_keys + 1) * sizeof(bptree_node *));
    memmove(&all_keys[pos + 1], &all_keys[pos], (node->num_keys - pos) * sizeof(void *));
    all_keys[pos] = new_key;
    memmove(&all_children[pos + 2], &all_children[pos + 1],
            (node->num_keys - pos) * sizeof(bptree_node *));
    all_children[pos + 1] = new_child;
    node->num_keys = split;
    memcpy(node->keys, all_keys, split * sizeof(void *));
    memcpy(node->ptr.internal.children, all_children, (split + 1) * sizeof(bptree_node *));
    bptree_node *new_internal = create_internal(tree);
    if (!new_internal) {
        tree->free_fn(all_keys);
        tree->free_fn(all_children);
        return res;
    }
    new_internal->num_keys = total - split - 1;
    memcpy(new_internal->keys, &all_keys[split + 1], (total - split - 1) * sizeof(void *));
    memcpy(new_internal->ptr.internal.children, &all_children[split + 1],
           (total - split) * sizeof(bptree_node *));
    res.promoted_key = all_keys[split];
    assert(res.promoted_key != NULL);
    res.new_child = new_internal;
    res.status = BPTREE_OK;
    tree->free_fn(all_keys);
    tree->free_fn(all_children);
    return res;
}

static insert_result insert_recursive(bptree *tree, bptree_node *node, void *item) {
    insert_result result = {NULL, NULL, BPTREE_ERROR};
    if (node->is_leaf) {
        int pos = leaf_node_search(tree, node->keys, node->num_keys, item);
        if (pos < node->num_keys && tree->compare(item, node->keys[pos], tree->udata) == 0) {
            result.status = BPTREE_DUPLICATE;
            return result;
        }
        if (node->num_keys < tree->max_keys) {
            memmove(&node->keys[pos + 1], &node->keys[pos],
                    (node->num_keys - pos) * sizeof(void *));
            memmove(&node->ptr.leaf.items[pos + 1], &node->ptr.leaf.items[pos],
                    (node->num_keys - pos) * sizeof(void *));
            node->keys[pos] = item;
            node->ptr.leaf.items[pos] = item;
            node->num_keys++;
            result.status = BPTREE_OK;
            return result;
        } else {
            int total = node->num_keys + 1;
            int split = total / 2;
            void **temp_keys = tree->malloc_fn(total * sizeof(void *));
            void **temp_items = tree->malloc_fn(total * sizeof(void *));
            if (!temp_keys || !temp_items) {
                if (temp_keys) tree->free_fn(temp_keys);
                if (temp_items) tree->free_fn(temp_items);
                return result;
            }
            memcpy(temp_keys, node->keys, pos * sizeof(void *));
            memcpy(temp_items, node->ptr.leaf.items, pos * sizeof(void *));
            temp_keys[pos] = item;
            temp_items[pos] = item;
            memcpy(&temp_keys[pos + 1], &node->keys[pos], (node->num_keys - pos) * sizeof(void *));
            memcpy(&temp_items[pos + 1], &node->ptr.leaf.items[pos],
                   (node->num_keys - pos) * sizeof(void *));
            node->num_keys = split;
            memcpy(node->keys, temp_keys, split * sizeof(void *));
            memcpy(node->ptr.leaf.items, temp_items, split * sizeof(void *));
            bptree_node *new_leaf = create_leaf(tree);
            if (!new_leaf) {
                tree->free_fn(temp_keys);
                tree->free_fn(temp_items);
                return result;
            }
            new_leaf->num_keys = total - split;
            memcpy(new_leaf->keys, &temp_keys[split], (total - split) * sizeof(void *));
            memcpy(new_leaf->ptr.leaf.items, &temp_items[split], (total - split) * sizeof(void *));
            new_leaf->ptr.leaf.next = node->ptr.leaf.next;
            node->ptr.leaf.next = new_leaf;
            result.promoted_key = new_leaf->keys[0];
            result.new_child = new_leaf;
            result.status = BPTREE_OK;
            tree->free_fn(temp_keys);
            tree->free_fn(temp_items);
            return result;
        }
    } else {
        int pos = internal_node_search(tree, node->keys, node->num_keys, item);
        insert_result child_result = insert_recursive(tree, node->ptr.internal.children[pos], item);
        if (child_result.status == BPTREE_DUPLICATE) return child_result;
        if (child_result.status != BPTREE_OK) return child_result;
        if (child_result.promoted_key == NULL) return child_result;
        if (node->num_keys < tree->max_keys) {
            memmove(&node->keys[pos + 1], &node->keys[pos],
                    (node->num_keys - pos) * sizeof(void *));
            memmove(&node->ptr.internal.children[pos + 2], &node->ptr.internal.children[pos + 1],
                    (node->num_keys - pos) * sizeof(bptree_node *));
            node->keys[pos] = child_result.promoted_key;
            node->ptr.internal.children[pos + 1] = child_result.new_child;
            node->num_keys++;
            result.status = BPTREE_OK;
            return result;
        } else {
            return split_internal(tree, node, child_result.promoted_key, child_result.new_child,
                                  pos);
        }
    }
}

bptree_status bptree_insert(bptree *tree, void *item) {
    insert_result result = insert_recursive(tree, tree->root, item);
    if (result.status == BPTREE_DUPLICATE) return BPTREE_DUPLICATE;
    if (result.status != BPTREE_OK) return result.status;
    if (result.promoted_key == NULL) {
        tree->count++;
        return BPTREE_OK;
    }
    bptree_node *new_root = create_internal(tree);
    if (!new_root) return BPTREE_ALLOCATION_ERROR;
    new_root->num_keys = 1;
    new_root->keys[0] = result.promoted_key;
    new_root->ptr.internal.children[0] = tree->root;
    new_root->ptr.internal.children[1] = result.new_child;
    tree->root = new_root;
    tree->height++;
    tree->count++;
    return BPTREE_OK;
}

void *bptree_search(const bptree *tree, const void *key) {
    bptree_node *node = tree->root;
    while (!node->is_leaf) {
        int pos = internal_node_search(tree, node->keys, node->num_keys, key);
        node = node->ptr.internal.children[pos];
    }
    int pos = leaf_node_search(tree, node->keys, node->num_keys, key);
    if (pos < node->num_keys && tree->compare(key, node->keys[pos], tree->udata) == 0)
        return node->ptr.leaf.items[pos];
    return NULL;
}

typedef struct {
    bptree_node *node;
    int pos;
} delete_stack_item;

bptree_status bptree_delete(bptree *tree, const void *key) {
    if (!tree || !tree->root) return BPTREE_ERROR;
    int stack_capacity = 16;
    int depth = 0;
    delete_stack_item *stack = tree->malloc_fn(stack_capacity * sizeof(delete_stack_item));
    if (!stack) return BPTREE_ALLOCATION_ERROR;
    bptree_node *node = tree->root;
    while (!node->is_leaf) {
        int pos = internal_node_search(tree, node->keys, node->num_keys, key);
        if (depth >= stack_capacity) {
            int new_capacity = stack_capacity * 2;
            delete_stack_item *new_stack =
                tree->malloc_fn(new_capacity * sizeof(delete_stack_item));
            if (!new_stack) {
                tree->free_fn(stack);
                return BPTREE_ALLOCATION_ERROR;
            }
            memcpy(new_stack, stack, depth * sizeof(delete_stack_item));
            tree->free_fn(stack);
            stack = new_stack;
            stack_capacity = new_capacity;
        }
        stack[depth].node = node;
        stack[depth].pos = pos;
        depth++;
        node = node->ptr.internal.children[pos];
    }
    int pos = leaf_node_search(tree, node->keys, node->num_keys, key);
    if (pos >= node->num_keys || tree->compare(key, node->keys[pos], tree->udata) != 0) {
        tree->free_fn(stack);
        return BPTREE_NOT_FOUND;
    }
    memmove(&node->ptr.leaf.items[pos], &node->ptr.leaf.items[pos + 1],
            (node->num_keys - pos - 1) * sizeof(void *));
    memmove(&node->keys[pos], &node->keys[pos + 1], (node->num_keys - pos - 1) * sizeof(void *));
    node->num_keys--;
    bool underflow = (node != tree->root && node->num_keys < tree->min_keys);
    while (underflow && depth > 0) {
        depth--;
        bptree_node *parent = stack[depth].node;
        int child_index = stack[depth].pos;
        bptree_node *child = parent->ptr.internal.children[child_index];
        bptree_node *left =
            (child_index > 0 ? parent->ptr.internal.children[child_index - 1] : NULL);
        bptree_node *right =
            (child_index < parent->num_keys ? parent->ptr.internal.children[child_index + 1]
                                            : NULL);
        BPTREE_LOG_DEBUG(tree,
                         "Iterative deletion at depth %d: parent num_keys=%d, child index=%d "
                         "(is_leaf=%d, num_keys=%d)",
                         depth, parent->num_keys, child_index, child->is_leaf, child->num_keys);
        bool merged = false;
        if (left && left->num_keys > tree->min_keys) {
            if (child->is_leaf) {
                memmove(&child->ptr.leaf.items[1], child->ptr.leaf.items,
                        child->num_keys * sizeof(void *));
                memmove(&child->keys[1], child->keys, child->num_keys * sizeof(void *));
                child->ptr.leaf.items[0] = left->ptr.leaf.items[left->num_keys - 1];
                child->keys[0] = left->keys[left->num_keys - 1];
                left->num_keys--;
                child->num_keys++;
                parent->keys[child_index - 1] = child->keys[0];
            } else {
                memmove(&child->keys[1], child->keys, child->num_keys * sizeof(void *));
                memmove(&child->ptr.internal.children[1], child->ptr.internal.children,
                        (child->num_keys + 1) * sizeof(bptree_node *));
                child->keys[0] = parent->keys[child_index - 1];
                parent->keys[child_index - 1] = left->keys[left->num_keys - 1];
                child->ptr.internal.children[0] = left->ptr.internal.children[left->num_keys];
                left->num_keys--;
                child->num_keys++;
            }
            merged = true;
        } else if (right && right->num_keys > tree->min_keys) {
            if (child->is_leaf) {
                child->ptr.leaf.items[child->num_keys] = right->ptr.leaf.items[0];
                child->keys[child->num_keys] = right->keys[0];
                memmove(&right->ptr.leaf.items[0], &right->ptr.leaf.items[1],
                        (right->num_keys - 1) * sizeof(void *));
                memmove(&right->keys[0], &right->keys[1], (right->num_keys - 1) * sizeof(void *));
                right->num_keys--;
                parent->keys[child_index] = right->keys[0];
                child->num_keys++;
            } else {
                child->keys[child->num_keys] = parent->keys[child_index];
                child->ptr.internal.children[child->num_keys + 1] = right->ptr.internal.children[0];
                parent->keys[child_index] = right->keys[0];
                memmove(&right->keys[0], &right->keys[1], (right->num_keys - 1) * sizeof(void *));
                memmove(&right->ptr.internal.children[0], &right->ptr.internal.children[1],
                        (right->num_keys - 1) * sizeof(bptree_node *));
                right->num_keys--;
                child->num_keys++;
            }
            merged = true;
        } else {
            if (left) {
                BPTREE_LOG_DEBUG(tree, "Merging child index %d with left sibling", child_index);
                if (child->is_leaf) {
                    memcpy(&left->ptr.leaf.items[left->num_keys], child->ptr.leaf.items,
                           child->num_keys * sizeof(void *));
                    memcpy(&left->keys[left->num_keys], child->keys,
                           child->num_keys * sizeof(void *));
                    left->num_keys += child->num_keys;
                    left->ptr.leaf.next = child->ptr.leaf.next;
                } else {
                    left->keys[left->num_keys] = parent->keys[child_index - 1];
                    left->num_keys++;
                    memcpy(&left->keys[left->num_keys], child->keys,
                           child->num_keys * sizeof(void *));
                    memcpy(&left->ptr.internal.children[left->num_keys],
                           child->ptr.internal.children,
                           (child->num_keys + 1) * sizeof(bptree_node *));
                    left->num_keys += child->num_keys;
                }
                int old_children = parent->num_keys + 1;
                memmove(&parent->ptr.internal.children[child_index],
                        &parent->ptr.internal.children[child_index + 1],
                        (old_children - child_index - 1) * sizeof(bptree_node *));
                parent->ptr.internal.children[old_children - 1] = NULL;
                int old_keys = parent->num_keys;
                memmove(&parent->keys[child_index - 1], &parent->keys[child_index],
                        (old_keys - child_index) * sizeof(void *));
                parent->num_keys = old_keys - 1;
                free_node(tree, child);
                merged = true;
                break;
            } else if (right) {
                BPTREE_LOG_DEBUG(tree, "Merging child index %d with right sibling", child_index);
                if (child->is_leaf) {
                    memcpy(&child->ptr.leaf.items[child->num_keys], right->ptr.leaf.items,
                           right->num_keys * sizeof(void *));
                    memcpy(&child->keys[child->num_keys], right->keys,
                           right->num_keys * sizeof(void *));
                    child->num_keys += right->num_keys;
                    child->ptr.leaf.next = right->ptr.leaf.next;
                } else {
                    child->keys[child->num_keys] = parent->keys[child_index];
                    child->num_keys++;
                    memcpy(&child->keys[child->num_keys], right->keys,
                           right->num_keys * sizeof(void *));
                    memcpy(&child->ptr.internal.children[child->num_keys],
                           right->ptr.internal.children,
                           (right->num_keys + 1) * sizeof(bptree_node *));
                    child->num_keys += right->num_keys;
                }
                int old_children = parent->num_keys + 1;
                memmove(&parent->ptr.internal.children[child_index + 1],
                        &parent->ptr.internal.children[child_index + 2],
                        (old_children - child_index - 2) * sizeof(bptree_node *));
                parent->ptr.internal.children[old_children - 1] = NULL;
                int old_keys = parent->num_keys;
                memmove(&parent->keys[child_index], &parent->keys[child_index + 1],
                        (old_keys - child_index - 1) * sizeof(void *));
                parent->num_keys = old_keys - 1;
                free_node(tree, right);
                merged = true;
                break;
            }
        }
        if (merged) {
            child = parent->ptr.internal.children[child_index];
            underflow = (child != tree->root && child->num_keys < tree->min_keys);
        } else {
            underflow = false;
        }
    }
    if (tree->root->num_keys == 0 && !tree->root->is_leaf) {
        bptree_node *old_root = tree->root;
        tree->root = tree->root->ptr.internal.children[0];
        tree->free_fn(old_root->keys);
        tree->free_fn(old_root->ptr.internal.children);
        tree->free_fn(old_root);
        tree->height--;
    }
    tree->count--;
    tree->free_fn(stack);
    return BPTREE_OK;
}

bptree *bptree_new(int max_keys, int (*compare)(const void *a, const void *b, const void *udata),
                   void *udata, bptree_malloc_t malloc_fn, bptree_free_t free_fn,
                   bool debug_enabled) {
    if (max_keys < 3) max_keys = 3;
    if (!malloc_fn) malloc_fn = default_malloc;
    if (!free_fn) free_fn = default_free;
    bptree *tree = malloc_fn(sizeof(bptree));
    if (!tree) return NULL;
    tree->max_keys = max_keys;
    tree->min_keys = (max_keys + 1) / 2;
    tree->height = 1;
    tree->count = 0;
    tree->compare = compare;
    tree->udata = udata;
    tree->malloc_fn = malloc_fn;
    tree->free_fn = free_fn;
    tree->debug_enabled = debug_enabled;
    BPTREE_LOG_DEBUG(tree, "B+tree created (max_keys=%d)", tree->max_keys);
    tree->root = create_leaf(tree);
    if (!tree->root) {
        tree->free_fn(tree);
        return NULL;
    }
    return tree;
}

void bptree_free(bptree *tree) {
    if (!tree) return;
    free_node(tree, tree->root);
    tree->free_fn(tree);
}

/* Range search API:
   Returns an array of items whose keys are between start_key and end_key (inclusive).
   The number of items found is stored in *count.
   The returned array is allocated via the tree's allocator and must be freed by the caller.
*/
void **bptree_range_search(const bptree *tree, const void *start_key, const void *end_key,
                           int *count) {
    *count = 0;
    if (!tree || !tree->root) return NULL;
    bptree_node *node = tree->root;
    while (!node->is_leaf) {
        int pos = internal_node_search(tree, node->keys, node->num_keys, start_key);
        node = node->ptr.internal.children[pos];
    }
    int capacity = 16;
    void **results = tree->malloc_fn(capacity * sizeof(void *));
    if (!results) return NULL;
    while (node) {
        for (int i = 0; i < node->num_keys; i++) {
            if (tree->compare(node->keys[i], start_key, tree->udata) >= 0 &&
                tree->compare(node->keys[i], end_key, tree->udata) <= 0) {
                if (*count >= capacity) {
                    capacity *= 2;
                    void **temp = tree->malloc_fn(capacity * sizeof(void *));
                    if (!temp) {
                        tree->free_fn(results);
                        return NULL;
                    }
                    memcpy(temp, results, (*count) * sizeof(void *));
                    tree->free_fn(results);
                    results = temp;
                }
                results[(*count)++] = node->ptr.leaf.items[i];
            } else if (tree->compare(node->keys[i], end_key, tree->udata) > 0) {
                return results;
            }
        }
        node = node->ptr.leaf.next;
    }
    return results;
}

#endif /* BPTREE_IMPLEMENTATION */
#endif /* BPTREE_H */
