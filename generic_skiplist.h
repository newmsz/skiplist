/*
* Author: Sungsoo Moon (newms@newms.org)
* Modified version of skiplist.h
*/

#ifndef __GENERIC_SKIPLIST_H
#define __GENERIC_SKIPLIST_H

/* skiplist_node_compare_t returns:
 *      > 0 if a > b
 *      = 0 if a = b
 *      < 0 if a < b
 */
typedef int skiplist_node_compare_t (void* a, void* b);

typedef struct sk_link {
    struct sk_link* prev;
    struct sk_link* next;
} sk_link_t;

static inline void
__skiplist_link_init(sk_link_t* link) {
    link->prev = link;
    link->next = link;
}

static inline void
__skiplist_link_add(sk_link_t* link, sk_link_t* prev, sk_link_t* next) {
    link->next = next;
    link->prev = prev;
    next->prev = link;
    prev->next = link;
}

static inline void
__skiplist_link_del(sk_link_t* link) {
    link->prev->next = link->next;
    link->next->prev = link->prev;
    __skiplist_link_init(link);
}

static inline int
__skiplist_list_empty(sk_link_t* link) {
    return link->next == link;
}

#define list_entry(ptr, type, member)           ((type *)((char *)(ptr) - (size_t)(&((type *)0)->member)))
#define skiplist_foreach(pos, end)              for (; pos != end; pos = pos->next)
#define skiplist_foreach_safe(pos, n, end)      for (n = pos->next; pos != end; pos = n, n = pos->next)

#define MAX_LEVEL 32  /* Should be enough for 2^32 elements */

typedef struct {
    int level;
    int count;
    skiplist_node_compare_t* comp;
    sk_link_t head[MAX_LEVEL];
} skiplist_t;

typedef struct {
    void* key;
    void* value;
    sk_link_t link[0];
} skipnode_t;

static skipnode_t*
skipnode_new(int level, void* key, void* value) {
    skipnode_t *node = malloc(sizeof(skipnode_t) + level*sizeof(sk_link_t));

    if (node != NULL) {
        node->key = key;
        node->value = value;

        for(int i=0; i<level; i++)
            __skiplist_link_init(node->link+i);
    }

    return node;
}

static void
skipnode_free(skipnode_t *node) {
    free(node);
}

static skiplist_t*
skiplist_new(skiplist_node_compare_t* compf) {
    skiplist_t *list = malloc(sizeof(skiplist_t));

    if (list != NULL) {
        list->level = 1;
        list->count = 0;
        list->comp = compf;
        for (int i = 0; i < MAX_LEVEL; i++) {
            __skiplist_link_init(&list->head[i]);
        }
    }

    return list;
}

static void
skiplist_free(skiplist_t* list) {
    sk_link_t* n;
    sk_link_t* pos = list->head[0].next;
    skiplist_foreach_safe(pos, n, &list->head[0]) {
        skipnode_t *node = list_entry(pos, skipnode_t, link[0]);
        skipnode_free(node);
    }
    free(list);
}

static inline int
__skiplist_random_level(void) {
    const double p = 0.5;
    int level = 1;
    while ((random() & 0xffff) < 0xffff * p) level++;
    return level > MAX_LEVEL ? MAX_LEVEL : level;
}

static skipnode_t*
skiplist_search(skiplist_t* list, void* key) {
    int i = list->level - 1;
    sk_link_t* pos = &list->head[i];
    sk_link_t* end = &list->head[i];
    skipnode_t *node;

    for (; i >= 0; i--) {
        pos = pos->next;
        skiplist_foreach(pos, end) {
            node = list_entry(pos, skipnode_t, link[i]);
            if (list->comp(node->key, key) >= 0) {
                end = &node->link[i];
                break;
            }
        }

        if (list->comp(node->key, key) == 0) {
            return node;
        }

        pos = end->prev;
        pos--;
        end--;
    }

    return NULL;
}

static skipnode_t*
skiplist_insert(skiplist_t* list, void* key, void* value) {
    int level = __skiplist_random_level();
    if (level > list->level) {
        list->level = level;
    }

    skipnode_t *node = skipnode_new(level, key, value);
    if (node != NULL) {
        int i = list->level - 1;

        sk_link_t* pos = &list->head[i];
        sk_link_t* end = &list->head[i];

        for (; i >= 0; i--) {
            pos = pos->next;
            for (; pos != end; pos = pos->next) {
                skipnode_t *nd = list_entry(pos, skipnode_t, link[i]);
                if (list->comp(nd->key, key) >= 0) {
                    end = &nd->link[i];
                    break;
                }
            }
            pos = end->prev;
            if (i < level) {
                __skiplist_link_add(&node->link[i], pos, end);
            }
            pos--;
            end--;
        }

        list->count++;
    }
    return node;
}

static int
skiplist_size (skiplist_t* list) {
    return list->count;
}

static inline void
__skiplist_remove_node(skiplist_t* list, skipnode_t *node, int level) {
    for (int i = 0; i < level; i++) {
        __skiplist_link_del(&node->link[i]);
        if (__skiplist_list_empty(&list->head[i])) {
            list->level--;
        }
    }

    skipnode_free(node);
    list->count--;
}

static void
skiplist_remove(skiplist_t* list, void* key) {
    int i = list->level - 1;
    sk_link_t* pos = &list->head[i];
    sk_link_t* end = &list->head[i];
    sk_link_t* n;

    for (; i >= 0; i--) {
        pos = pos->next;
        skiplist_foreach_safe(pos, n, end) {
            skipnode_t* node = list_entry(pos, skipnode_t, link[i]);
            if (list->comp(node->key, key) > 0) {
                    end = &node->link[i];
                    break;
            }

            if (list->comp(node->key, key) == 0) {
                // Here's no break statement because we allow nodes with same key.
                __skiplist_remove_node(list, node, i + 1);
            }
        }
        pos = end->prev;
        pos--;
        end--;
    }

}

static void
skiplist_dump(skiplist_t* list) {
    int i = list->level - 1;
    sk_link_t* pos = &list->head[i];
    sk_link_t* end = &list->head[i];

    printf("\nTotal %d nodes: \n", list->count);
    for (; i >= 0; i--) {
        pos = pos->next;
        for (; pos != end; pos = pos->next) {
            // ((type *)((char *)(ptr) - (size_t)(&((type *)0)->member)))
            skipnode_t *node = list_entry(pos, skipnode_t, link[i]);
            printf("level:%d key:0x%s value:0x%p\n", i + 1, node->key, node->value);
        }
        pos = &list->head[i];
        pos--;
        end--;
    }
}

#endif /* __GENERIC_SKIPLIST_H */
