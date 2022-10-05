/**
 * deadlock_demolition
 * CS 341 - Fall 2022
 */
#include "graph.h"
#include "libdrm.h"
#include "set.h"
#include <pthread.h>

static pthread_mutex_t graph_lock = PTHREAD_MUTEX_INITIALIZER;

static graph * g;
static int id = 0;

struct drm_t {
    // TODO: Maybe add something here
};

drm_t *drm_init() {
    // Allocate and init drm_t
    drm_t * new_drm = (drm_t *) malloc(sizeof(drm_t));
    if (!new_drm) {
        // Malloc failed
        return NULL;
    }
    // Add any drm_t value init here...

    // Create (if needed) and update graph
    pthread_mutex_lock(&graph_lock):
    if (!g) {
        g = shallow_graph_create();
    }
    graph_add_vertex(g, new_drm);
    pthread_mutex_unlock(&graph_lock);

    return new_drm;
}

int drm_post(drm_t *drm, pthread_t *thread_id) {
    /* Your code here */
    return 0;
}

int drm_wait(drm_t *drm, pthread_t *thread_id) {
    /* Your code here */
    return 0;
}

void drm_destroy(drm_t *drm) {
    /* Your code here */
    pthread_mutex_lock(&graph_lock);
    graph_remove_vertex(g, drm);
    if (!graph_vertex_count(g)) {
        // No more vertices (therefore no more edges) in graph, free it
        free(g);
        g = NULL;
    }
    ptherad_mutuex_unlock(&graph_lock);

    free(drm);
    drm = NULL;
    return;
}
