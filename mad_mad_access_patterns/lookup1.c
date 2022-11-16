/**
 * mad_mad_access_patterns
 * CS 341 - Fall 2022
 */
#include "tree.h"
#include "utils.h"
#include <stdio.h>
#include <string.h>
#include <assert.h>
#include <stdlib.h>
/*
  Look up a few nodes in the tree and print the info they contain.
  This version uses fseek() and fread() to access the data.

  ./lookup1 <data_file> <word> [<word> ...]
*/

int main(int argc, char **argv) {
    if (argc < 3) {
      printArgumentUsage();
      return 1;
    }
    char * file_name = argv[1];
    // Open file
    FILE * data_file = fopen(file_name, "r");
    if (!data_file) {
      // fopen failed
      openFail(file_name);
      if (data_file) {
        fclose(data_file);
      }
      return 2;
    }

    // Validate the file starts with "BTRE"
    char start_bytes[BINTREE_ROOT_NODE_OFFSET + 1];
    memset(start_bytes, 0, BINTREE_ROOT_NODE_OFFSET + 1);
    if (!fread(&start_bytes, BINTREE_ROOT_NODE_OFFSET, 1, data_file)) {
      fclose(data_file);
      return 1;
    }
    if (strcmp(start_bytes, BINTREE_HEADER_STRING)) {
      formatFail(file_name);
      fclose(data_file);
      return 2;
    }

    // Search for the word information of all the requested words in argv
    size_t target_word_idx = 2;
    for (; target_word_idx < (size_t)argc; target_word_idx++) {
      // Seek to offset of 4 bytes to start at file data
      if (fseek(data_file, BINTREE_ROOT_NODE_OFFSET, SEEK_SET) == -1) {
        fclose(data_file);
        return 1;
      }
      char * target_word = argv[target_word_idx];

      while (1) {
        // Populate node with base data
        long cur_offset = ftell(data_file);
        // fprintf(stderr, "%ld\n", cur_offset);
        BinaryTreeNode cur_node;
        memset(&cur_node, 0, sizeof(BinaryTreeNode));
        if (!fread(&cur_node, sizeof(BinaryTreeNode), 1, data_file)) {
          fclose(data_file);
          return 1;
        }
        
        // Get to end of word string
        while ((unsigned char)fgetc(data_file) != 0) {}
        long end_word_offset = ftell(data_file);
        size_t total_read_bytes = (size_t)(end_word_offset - cur_offset);

        // Re-seek to current offset and re-read
        if (fseek(data_file, cur_offset, SEEK_SET) == -1) {
          fclose(data_file);
          return 1;
        }
        if (!fread(&cur_node, total_read_bytes, 1, data_file)) {
          fclose(data_file); 
          return 1;
        }

        int compare_res = strcmp(target_word, cur_node.word);
        if (compare_res == 0) {
          printFound(target_word, cur_node.count, cur_node.price);
          break;
        } else if (compare_res > 0) {
          if (!cur_node.right_child) {
            printNotFound(target_word);
            break;
          } else if (fseek(data_file, cur_node.right_child, SEEK_SET) == -1) {
            fclose(data_file);
            return 1;
          }
        } else {
          if (!cur_node.left_child) {
            printNotFound(target_word);
            break;
          } else if (fseek(data_file, cur_node.left_child, SEEK_SET) == -1) {
            fclose(data_file);
            return 1;
          }
        }
      }
    }

    fclose(data_file);
    return 0;
}
