/**
 * parallel_make
 * CS 341 - Fall 2022
 */
#include <stdbool.h>

#include "format.h"
#include "graph.h"
#include "parmake.h"
#include "parser.h"
#include "set.h"

static const int CYCLE_ERR = 2;
//static const int ERR = 1;

// Forward declarations
void build_goal_rule(graph * g, char * goal_rule);
void _build_recursive(graph * g, set * rec_set, set * visited, void * cur);

int parmake(char *makefile, size_t num_threads, char **targets) {
    graph * dep_graph = parser_parse_makefile(makefile, targets);
    vector * goal_names = graph_neighbors(dep_graph, "");
    VECTOR_FOR_EACH(goal_names, goal, {
        build_goal_rule(dep_graph, goal);
        rule_t * goal_rule = graph_get_vertex_value(dep_graph, goal);
        if (goal_rule->state == CYCLE_ERR) {
            print_cycle_failure(goal);
        }
    });

    vector_destroy(goal_names);
    graph_destroy(dep_graph);
    return 0;
}


// Helper Functions
void build_goal_rule(graph * g, char * goal_rule) {
    set * rec_set = shallow_set_create();
    set * visited = shallow_set_create();

    _build_recursive(g, rec_set, visited, (void *)goal_rule);

    set_destroy(rec_set);
    set_destroy(visited);

    return;
}

void _build_recursive(graph * g, set * rec_set, set * visited, void * cur) {
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
    VECTOR_FOR_EACH(deps, dep, {
        _build_recursive(g, rec_set, visited, dep);
        rule_t * dep_rule = graph_get_vertex_value(g, dep);
        if (dep_rule->state != 0) {
            rule_t * cur_rule = graph_get_vertex_value(g, cur);

            // Set state to highest value (0 -> good, 1 -> general fail, 2 -> cycle fail)
            cur_rule->state = cur_rule->state < dep_rule->state ? dep_rule->state : cur_rule->state;
        }
    });
    set_remove(rec_set, cur); // Done with cur in the recursive stack, remove

    return;
}

