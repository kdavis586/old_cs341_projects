/**
 * parallel_make
 * CS 341 - Fall 2022
 */
#include <stdbool.h>
#include <unistd.h>
#include <stdio.h>
#include <sys/stat.h>
#include <time.h>
#include <pthread.h>
#include <stdbool.h>

#include "format.h"
#include "graph.h"
#include "parmake.h"
#include "parser.h"
#include "set.h"
#include "queue.h"

static const int CYCLE_ERR = 2;
static const int ERR = 1;
static const int WAITING = 0;
static const int OK = -1;

static pthread_mutex_t THREAD_LOCK = PTHREAD_MUTEX_INITIALIZER;
static pthread_cond_t COND = PTHREAD_COND_INITIALIZER;
static size_t NUM_THREADS;

// thread struct
typedef struct _t_info {
    graph * g;
    queue * job_queue;
    size_t queue_size;
} t_info;

// Forward declarations
void cycle_detect(graph * g, char * goal_rule);
void _cycle_detect_recursive(graph * g, set * rec_set, set * visited, void * cur);
void build_goal_rule(graph * g, char * goal_rule, t_info ** infos);
void _build_recursive(graph * g, set * visited, void * cur, t_info ** infos);
void run_commands(rule_t * cur_rule);
void * t_run_commands(void * arg);
int deps_status(vector * deps, graph * g);

int parmake(char *makefile, size_t num_threads, char **targets) {
    NUM_THREADS = num_threads;
    graph * dep_graph = parser_parse_makefile(makefile, targets);
    vector * goal_names = graph_neighbors(dep_graph, "");

    pthread_t tids[NUM_THREADS];
    t_info * infos[NUM_THREADS];
    size_t i;
    // init thread info and start up the threads
    for (i = 0; i < NUM_THREADS; i++) {
        infos[i] = malloc(sizeof(t_info));
        infos[i]->job_queue = queue_create(-1);
        infos[i]->queue_size = 0;
        infos[i]->g = dep_graph;

        pthread_create(tids + i, NULL, t_run_commands, (void *)(infos[i]));
    }

    // cycle detect, the ||ly process rules
    VECTOR_FOR_EACH(goal_names, goal, {
        cycle_detect(dep_graph, goal);
        rule_t * goal_rule = graph_get_vertex_value(dep_graph, goal);
        if (goal_rule->state == CYCLE_ERR) {
            print_cycle_failure(goal);
        } else {
            build_goal_rule(dep_graph, goal, infos);
        }
    });

    // join all threads
    for (i = 0; i < NUM_THREADS; i++) {
        // add null to job queue, thread will exit after
        queue_push(infos[i]->job_queue, NULL);
    }

    for (i = 0; i < NUM_THREADS; i++) {
        // add null to job queue, thread will exit after
        pthread_join(tids[i], NULL);
        queue_destroy(infos[i]->job_queue);
        free(infos[i]);
    }

    vector_destroy(goal_names);
    graph_destroy(dep_graph);
    return 0;
}


// Helper Functions
void cycle_detect(graph * g, char * goal_rule) {
    set * rec_set = shallow_set_create();
    set * visited = shallow_set_create();
    
    _cycle_detect_recursive(g, rec_set, visited, (void *)goal_rule);

    set_destroy(rec_set);
    set_destroy(visited);

    return;
}

void _cycle_detect_recursive(graph * g, set * rec_set, set * visited, void * cur) {
    if (set_contains(rec_set, cur)) {
        pthread_mutex_lock(&THREAD_LOCK);
        rule_t * rule = graph_get_vertex_value(g, cur);
        rule->state = CYCLE_ERR;
        pthread_mutex_unlock(&THREAD_LOCK);
        return;
    } else if (set_contains(visited, cur)) {
        return;
    }

    set_add(rec_set, cur);
    set_add(visited, cur);
    pthread_mutex_lock(&THREAD_LOCK);
    vector * deps = graph_neighbors(g, cur);
    rule_t * cur_rule = graph_get_vertex_value(g, cur);
    pthread_mutex_unlock(&THREAD_LOCK);

    // TODO: maybe ||ize here
    VECTOR_FOR_EACH(deps, dep, {
        _cycle_detect_recursive(g, rec_set, visited, dep);
        pthread_mutex_lock(&THREAD_LOCK);
        rule_t * dep_rule = graph_get_vertex_value(g, dep);
        if (dep_rule->state != OK) {
            // in cycle_detect, only non-OK state is CYCLE-ERR, assign and break;
            cur_rule->state = dep_rule->state;
            pthread_mutex_unlock(&THREAD_LOCK);
            break;
        }
        pthread_mutex_unlock(&THREAD_LOCK);
    });

    set_remove(rec_set, cur); // Done with cur in the recursive stack, remove
    vector_destroy(deps);
    return;
}

// DOES NOT CHECK FOR CYCLES WHILE RUNNING
void build_goal_rule(graph * g, char * goal_rule, t_info ** infos) {
    set * visited = shallow_set_create();
    _build_recursive(g, visited, (void *)goal_rule, infos);
    set_destroy(visited);
    return;
}

void _build_recursive(graph * g, set * visited, void * cur, t_info ** infos) {
    pthread_mutex_lock(&THREAD_LOCK);
    rule_t * cur_rule = graph_get_vertex_value(g, cur);
    pthread_mutex_unlock(&THREAD_LOCK);
    if (set_contains(visited, cur)) {
        return;
    }

    set_add(visited, cur);
    pthread_mutex_lock(&THREAD_LOCK);
    vector * deps = graph_neighbors(g, cur);
    pthread_mutex_unlock(&THREAD_LOCK);

    // TODO: maybe ||ize here
    // take care of all dependencies
    VECTOR_FOR_EACH(deps, dep, {
        _build_recursive(g, visited, dep, infos);
    });
    // pthread_mutex_lock(&THREAD_LOCK);
    // cur_rule->state = OK;
    // pthread_mutex_unlock(&THREAD_LOCK);
    // pthread_cond_broadcast(&COND);

    // Run rule commands if no failure from dependencies
    
    struct stat buf;
    if (stat((char *)cur, &buf) == 0) {
        // cur is a file on disk
        time_t cur_mod_time = buf.st_mtime;

        // TODO: maybe ||ize here
        VECTOR_FOR_EACH(deps, dep, {
            struct stat dep_buf;
            if (stat((char *)dep, &dep_buf) == 0 && cur_mod_time < dep_buf.st_mtime) {
                size_t i;
                size_t min_idx = 0;
                for (i = 0; i < NUM_THREADS; i++) {
                    if ((infos[i])->queue_size < (infos[min_idx])->queue_size) {
                        min_idx = i;
                    }
                }
                
                queue_push(infos[min_idx]->job_queue, (void *)cur_rule);
                
                pthread_mutex_lock(&THREAD_LOCK);
                (infos[min_idx])->queue_size++;
                pthread_mutex_unlock(&THREAD_LOCK);
                
                break;
            }
        });

        cur_rule->state = OK;
    } else {
        // cur is /not/ a file on disk and all dependencies should have run
        size_t i;
        size_t min_idx = 0;
        for (i = 0; i < NUM_THREADS; i++) {
            if (infos[i]->queue_size < infos[min_idx]->queue_size) {
                min_idx = i;
            }
        }
        
        queue_push(infos[min_idx]->job_queue, (void *)cur_rule);
        pthread_mutex_lock(&THREAD_LOCK);
        infos[min_idx]->queue_size++;
        pthread_mutex_unlock(&THREAD_LOCK);
    }
    
    vector_destroy(deps);
    return;
}

void run_commands(rule_t * cur_rule) {
    
    int success = 0;
    pthread_mutex_lock(&THREAD_LOCK);
    VECTOR_FOR_EACH(cur_rule->commands, command, {
        pthread_mutex_unlock(&THREAD_LOCK);
        if ((success = system((char *)command)) != 0) {
            pthread_mutex_lock(&THREAD_LOCK);
            cur_rule->state = ERR;
            pthread_mutex_unlock(&THREAD_LOCK);
            
            break;
        };
    });
    if (cur_rule->state == WAITING) {
        // didn't fail
        cur_rule->state = OK;
    }
    pthread_mutex_unlock(&THREAD_LOCK);
    pthread_cond_broadcast(&COND);
}

void * t_run_commands(void * arg) {
    t_info * worker_info = (t_info *)arg;
    
    rule_t * job_rule = NULL;
    while ((job_rule = (rule_t *)queue_pull(worker_info->job_queue))) {
        fprintf(stderr, "Thread %zu about to process: %s\n", pthread_self(), job_rule->target);
        pthread_mutex_lock(&THREAD_LOCK);
        worker_info->queue_size--;
        vector * deps = graph_neighbors(worker_info->g, job_rule->target);
        int dep_overall_state = -1;
        while ((dep_overall_state = deps_status(deps, worker_info->g)) == WAITING) {
            //fprintf(stderr, "Thread %zu is going to sleep\n", pthread_self());
            pthread_cond_wait(&COND, &THREAD_LOCK);
            //fprintf(stderr, "Thread %zu woke up!\n", pthread_self());
        }
        // Check to see if the current rule has already failed;
        if (dep_overall_state != OK) {
            job_rule->state = dep_overall_state;
            pthread_mutex_unlock(&THREAD_LOCK);
            continue;
        }
        pthread_mutex_unlock(&THREAD_LOCK);

        // Process queued rule
        run_commands(job_rule);
        vector_destroy(deps);
    }
    return NULL;
}

int deps_status(vector * deps, graph * g) {
    if (vector_size(deps) == 0) {
        return OK;
    }
    int status = -1;

    VECTOR_FOR_EACH(deps, dep, {
        rule_t * dep_rule = graph_get_vertex_value(g, dep);

        if (dep_rule->state > status) {
            status = dep_rule->state;
        }
    });

    return status;
}



