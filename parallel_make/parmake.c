/**
 * parallel_make
 * CS 341 - Fall 2022
 */
#include <stdbool.h>
#include <unistd.h>
#include <stdio.h>
#include <sys/stat.h>
#include <time.h>

#include "format.h"
#include "graph.h"
#include "parmake.h"
#include "parser.h"
#include "set.h"

static const int CYCLE_ERR = 2;
static const int ERR = 1;
static const int OK = 0;

// Forward declarations
void cycle_detect(graph * g, char * goal_rule);
void _cycle_detect_recursive(graph * g, set * rec_set, set * visited, void * cur);
void build_goal_rule(graph * g, char * goal_rule);
void _build_recursive(graph * g, set * visited, void * cur);
void run_commands(rule_t * cur_rule);

int parmake(char *makefile, size_t num_threads, char **targets) {
    graph * dep_graph = parser_parse_makefile(makefile, targets);
    vector * goal_names = graph_neighbors(dep_graph, "");

    VECTOR_FOR_EACH(goal_names, goal, {
        cycle_detect(dep_graph, goal);
        rule_t * goal_rule = graph_get_vertex_value(dep_graph, goal);
        if (goal_rule->state == CYCLE_ERR) {
            print_cycle_failure(goal);
        } else {
            build_goal_rule(dep_graph, goal);
        }
    });

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
        rule_t * rule = graph_get_vertex_value(g, cur);
        rule->state = CYCLE_ERR;
        return;
    } else if (set_contains(visited, cur)) {
        return;
    }

    set_add(rec_set, cur);
    set_add(visited, cur);
    vector * deps = graph_neighbors(g, cur);
    rule_t * cur_rule = graph_get_vertex_value(g, cur);
    VECTOR_FOR_EACH(deps, dep, {
        _cycle_detect_recursive(g, rec_set, visited, dep);
        rule_t * dep_rule = graph_get_vertex_value(g, dep);
        if (dep_rule->state != OK) {
            // in cycle_detect, only non-OK state is CYCLE-ERR, assign and break;
            cur_rule->state = dep_rule->state;
            break;
        }
    });

    set_remove(rec_set, cur); // Done with cur in the recursive stack, remove
    vector_destroy(deps);
    return;
}

// DOES NOT CHECK FOR CYCLES WHILE RUNNING
void build_goal_rule(graph * g, char * goal_rule) {
    set * visited = shallow_set_create();
    _build_recursive(g, visited, (void *)goal_rule);
    set_destroy(visited);
    return;
}

void _build_recursive(graph * g, set * visited, void * cur) {
    rule_t * cur_rule = graph_get_vertex_value(g, cur);
    if (set_contains(visited, cur)) {
        return;
    }

    set_add(visited, cur);
    vector * deps = graph_neighbors(g, cur);
    VECTOR_FOR_EACH(deps, dep, {
        _build_recursive(g, visited, dep);
        rule_t * dep_rule = graph_get_vertex_value(g, dep);
        if (dep_rule->state != OK) {
            // Set state to highest value (0 -> good, 1 -> general fail, 2 -> cycle fail)
            cur_rule->state = dep_rule->state;
        }
    });

    // Run rule commands if no failure from dependencies
    if (cur_rule->state == OK) {
        struct stat buf;
        if (stat((char *)cur, &buf) == 0) {
            // cur is a file on disk
            time_t cur_mod_time = buf.st_mtime;
            VECTOR_FOR_EACH(deps, dep, {
                struct stat dep_buf;
                if (stat((char *)dep, &dep_buf) == 0 && cur_mod_time < dep_buf.st_mtime) {
                    run_commands(cur_rule);
                    break;
                }
            });
        } else {
            // cur is /not/ a file on disk and all dependencies should have run
            run_commands(cur_rule);
        }
    }
    
    vector_destroy(deps);
    return;
}

void run_commands(rule_t * cur_rule) {
    int success = 0;
    VECTOR_FOR_EACH(cur_rule->commands, command, {
        if ((success = system((char *)command)) != 0) {
            fprintf(stderr, "Command: %s\n", (char *)command);
            cur_rule->state = ERR;
            break;
        };
    });
}

