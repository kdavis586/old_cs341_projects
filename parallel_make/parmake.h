/**
 * parallel_make
 * CS 341 - Fall 2022
 */
#pragma once
/**
 * this file exists so that we can provide a main method which does a variety of
 * handy things.
 *
 * DO NOT EDIT THIS FILE!
 */

#include <stddef.h>

// entry point for student code
int parmake(char *makefile, size_t num_threads, char **targets);
