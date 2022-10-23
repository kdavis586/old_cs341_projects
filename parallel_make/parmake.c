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

// Forward declarations
bool cycle_detect(graph * g, char * goal_rule);
bool _cycle_detect_recursive(graph * g, set * rec_set, set * visited, void * cur);

int parmake(char *makefile, size_t num_threads, char **targets) {
    graph * dep_graph = parser_parse_makefile(makefile, targets);
    vector * goal_rules = graph_neighbors(dep_graph, "");
    VECTOR_FOR_EACH(goal_rules, rule, {
        if (cycle_detect(dep_graph, rule)) {
            print_cycle_failure(rule);
        }
    });

    vector_destroy(goal_rules);
    return 0;
}


// Helper Functions
bool cycle_detect(graph * g, char * goal_rule) {
    set * rec_set = shallow_set_create();
    set * visited = shallow_set_create();

    bool is_cycle = _cycle_detect_recursive(g, rec_set, visited, (void *)goal_rule);

    set_destroy(rec_set);
    set_destroy(visited);

    return is_cycle;
}

bool _cycle_detect_recursive(graph * g, set * rec_set, set * visited, void * cur) {
    if (set_contains(rec_set, cur)) {
        return true;
    } else if (set_contains(visited, cur)) {
        return false;
    }

    set_add(rec_set, cur);
    set_add(visited, cur);
    vector * deps = graph_neighbors(g, cur);
    bool is_cycle = false;
    VECTOR_FOR_EACH(deps, dep, {
        if(_cycle_detect_recursive(g, rec_set, visited, dep)) {
            is_cycle = true;
            break;
        }
    });
    set_remove(rec_set, cur); // Done with cur in the recursive stack, remove

    return is_cycle;
}

