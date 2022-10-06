/**
 * deadlock_demolition
 * CS 341 - Fall 2022
 */
#include "graph.h"
#include "libdrm.h"
#include "set.h"
#include "queue.h"
#include "vector.h"
#include <pthread.h>
#include <stdio.h>

bool _cycle_present(graph * g, pthread_t * thread_id);

static pthread_mutex_t graph_lock = PTHREAD_MUTEX_INITIALIZER;
static graph * g;

struct drm_t {
    pthread_mutex_t lock;
    pthread_cond_t cv;
    bool in_use;
};

drm_t *drm_init() {
    // Allocate and init drm_t
    drm_t * new_drm = (drm_t *) malloc(sizeof(drm_t));
    if (!new_drm) {
        // Malloc failed
        return NULL;
    }
    pthread_mutex_init(&(new_drm->lock), NULL);
    pthread_cond_init(&(new_drm->cv), NULL);
    new_drm->in_use = false;

    // Create (if needed) and update graph
    pthread_mutex_lock(&graph_lock);
    if (!g) {
        g = shallow_graph_create();
    }
    graph_add_vertex(g, new_drm);
    pthread_mutex_unlock(&graph_lock);

    return new_drm;
}

int drm_post(drm_t *drm, pthread_t *thread_id) {
    /* Your code here */
    int retval = 0;
    pthread_mutex_lock(&(drm->lock));
    pthread_mutex_lock(&graph_lock);
    if (graph_contains_vertex(g, thread_id) && \
        graph_contains_vertex(g, drm) && graph_adjacent(g, drm, thread_id)) {
        graph_remove_edge(g, drm, thread_id);
        if (!graph_vertex_degree(g, thread_id) && !graph_vertex_antidegree(g, thread_id)) {
            graph_remove_vertex(g, thread_id);
        }
        drm->in_use = false;
        pthread_cond_signal(&(drm->cv)); // Wake next waiting thread
        retval = 1;
    }
    pthread_mutex_unlock(&graph_lock);
    pthread_mutex_unlock(&(drm->lock));
    return retval;
}

int drm_wait(drm_t *drm, pthread_t *thread_id) {
    int retval = 0;
    pthread_mutex_lock(&(drm->lock));
    
    // Edit the graph and check for cycles
    pthread_mutex_lock(&graph_lock);
    // See if we need to add vertex
    if (!graph_contains_vertex(g, thread_id)) {
        graph_add_vertex(g, thread_id);
    }

    if (drm->in_use) {
        // drm is currently in use, are we allowed to wait without causing deadlock?
        graph_add_edge(g, thread_id, drm);

        if (_cycle_present(g, thread_id)) {
            graph_remove_edge(g, thread_id, drm);
            retval = 0;
        } else {
            // no cycle go ahead and wait on the drm
            pthread_mutex_unlock(&graph_lock);
            while (drm->in_use) {
                pthread_cond_wait(&(drm->cv), &(drm->lock));
            }
            pthread_mutex_lock(&graph_lock);
            graph_remove_edge(g, thread_id, drm); // thread now has the resource, change edge direction
            graph_add_edge(g, drm, thread_id);
            retval = 1; 
        }
    } else {
        // drm is not in use, no worry about deadlock
        graph_add_edge(g, drm, thread_id);
        drm->in_use = true;
        retval = 1;
    }
    pthread_mutex_unlock(&graph_lock);
    pthread_mutex_unlock(&(drm->lock));
    return retval;
}

void drm_destroy(drm_t *drm) {
    /* Your code here */
    pthread_mutex_lock(&graph_lock);
    if (!drm) {
        // drm is NULL;
        pthread_mutex_unlock(&graph_lock);
        return;
    }

    
    if (g) {
        graph_remove_vertex(g, drm);
        if (!graph_edge_count(g)) {
            // No more vertices (therefore no more edges) in graph, free it
            pthread_mutex_destroy(&graph_lock);
            graph_destroy(g);
            g = NULL;
        }
    }
    
    pthread_mutex_destroy(&(drm->lock));
    pthread_cond_destroy(&(drm->cv));
    free(drm);
    drm = NULL;
    
    pthread_mutex_unlock(&graph_lock);
    return;
}

// Returns true if there is a cycle, false otherwise. NOT THREAD SAFE
bool _cycle_present(graph * g, pthread_t * thread_id) {
    // BFS on graph, no cross edges so we can do niave directed BFS
    queue * q = queue_create(graph_vertex_count(g));
    set * s = shallow_set_create();

    // Setup queue
    bool cycle = false;
    size_t q_size = 1;
    queue_push(q, thread_id);

    while (q_size) {
        void * key = queue_pull(q);
        q_size--;

        if (set_contains(s, key)) {
            cycle = true;
            break;
        }

        set_add(s, key); // Add to visited list
        vector * dir_neighbors = graph_neighbors(g, key);
        VECTOR_FOR_EACH(dir_neighbors, neighbor, {
            queue_push(q, neighbor);
            q_size++;
        });
        vector_destroy(dir_neighbors);
    }

    queue_destroy(q);
    set_destroy(s);
    return cycle;
}
