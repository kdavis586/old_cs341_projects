/**
 * deadlock_demolition
 * CS 341 - Fall 2022
 */
#include "graph.h"
#include "libdrm.h"
#include "set.h"
#include <limits.h>
#include <pthread.h>

static graph * g;
static int id = 0;

struct drm_t {

};

drm_t *drm_init() {
    /* Your code here */
    if (!g) {
        g = shallow_graph_create();
    }

    drm_t * new_drm = (drm_t *) malloc(sizeof(drm_t));
    if (!new_drm) {
        // Malloc failed
        return NULL;
    }

    // Add any drm_t value init here...

    
    graph_add_vertex(g, new_drm);
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
    return;
}
