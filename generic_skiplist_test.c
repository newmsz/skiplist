/*
* Author: Sungsoo Moon (newms@newms.org)
* Modified version of skiplist_test.c
*/

#include <stdio.h>
#include <stdlib.h>
#if defined(__MACH__) && !defined(CLOCK_REALTIME)
#include <sys/time.h>

#define CLOCK_REALTIME 0
#define CLOCK_MONOTONIC 1

int clock_gettime(int clk_id, struct timespec* t) {
    struct timeval now;
    int rv = gettimeofday(&now, NULL);
    if (rv) return rv;
    t->tv_sec  = now.tv_sec;
    t->tv_nsec = now.tv_usec * 1000;
    return 0;
}
#else
#include <time.h>
#endif

#include "generic_skiplist.h"

#define N 1024 * 1024
// #define SKIPLIST_DEBUG
#define KEY_BUFSIZE 100

static inline skiplist_node_compare_t skiplist_custom_compare;

static inline int
skiplist_custom_compare (void* a, void* b) {
    return atoi((char*)a) - atoi((char*)b);
}

int
main(void) {
    struct timespec start, end;

    char **keys = malloc(N*sizeof(char*));
    if (keys == NULL) exit(-1);

    for(int i=0; i<N; i++) {
        int value = 0;
        while((value = (int)random()) < 100) { };

        *(keys+i) = malloc(KEY_BUFSIZE);
        snprintf(*(keys+i), KEY_BUFSIZE, "%d", value);
        if(*(keys+i) == NULL) exit(-1);
    }

    skiplist_t* list = skiplist_new(&skiplist_custom_compare);
    if (list == NULL) exit(-1);

    printf("Test start!\n");
    printf("Add %d nodes...\n", N);

    /* Insert test */
    srandom(time(NULL));
    clock_gettime(CLOCK_MONOTONIC, &start);
    for (int i = 0; i < N; i++) {
        skiplist_insert(list, *(keys+i), *(keys+i));
    }

    if(N != skiplist_size(list)) {
        printf("Some insert dropped");
    }

    clock_gettime(CLOCK_MONOTONIC, &end);
    printf("time span: %ldms\n", (end.tv_sec - start.tv_sec)*1000 + (end.tv_nsec - start.tv_nsec)/1000000);

#ifdef SKIPLIST_DEBUG
    skiplist_dump(list);
#endif

    /* Search test */
    printf("Now search each node...\n");
    clock_gettime(CLOCK_MONOTONIC, &start);
    for (int i = 0; i < N; i++) {
        skipnode_t *node = skiplist_search(list, *(keys+i));

        if (node != NULL) {
#ifdef SKIPLIST_DEBUG
            printf("key:0x%s value:0x%s\n", node->key, node->value);
#endif
        } else {
            printf("Not found:0x%s\n", keys[i]);
        }
    }
    clock_gettime(CLOCK_MONOTONIC, &end);
    printf("time span: %ldms\n", (end.tv_sec - start.tv_sec)*1000 + (end.tv_nsec - start.tv_nsec)/1000000);

    printf("Add %d nodes (key=1)...\n", N);
    for (int i=0; i<N; i++) {
        skiplist_insert(list, "1", "1");
    }

    if(skiplist_search(list, "1") == NULL) printf("node (key=1) is not found");
    skiplist_remove(list, "1");
    if(skiplist_search(list, "1") != NULL) printf("node (key=1) is not removed");

    if(N != skiplist_size(list)) {
        printf("Some nodes(key=1) are not dropped");
    }

    /* Delete test */
    printf("Now remove all nodes...\n");
    clock_gettime(CLOCK_MONOTONIC, &start);
    for (int i = N-1; i >= 0; i--) {
        skiplist_remove(list, keys[i]);
    }

    if(0 != skiplist_size(list)) {
        printf("Some nodes are not dropped");
    }

    clock_gettime(CLOCK_MONOTONIC, &end);
    printf("time span: %ldms\n", (end.tv_sec - start.tv_sec)*1000 + (end.tv_nsec - start.tv_nsec)/1000000);

#ifdef SKIPLIST_DEBUG
    skiplist_dump(list);
#endif

    printf("End of Test.\n");
    skiplist_free(list);

    for(int i=0; i<N; i++) {
        free(*(keys+i));
    }
    free(keys);

    return 0;
}
